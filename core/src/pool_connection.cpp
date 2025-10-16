#include "pool_connection.h"
#include "configs.h"
#include "webconfig.h"
#include "esp_task_wdt.h"

// Static member definitions
WiFiClient* PoolConnection::shared_pool_client = nullptr;
SemaphoreHandle_t PoolConnection::pool_mutex = nullptr;
unsigned long PoolConnection::last_pool_activity = 0;

bool PoolConnection::initialize() {
    // Create mutex for thread-safe access
    pool_mutex = xSemaphoreCreateMutex();
    if (!pool_mutex) {
        Serial.println("Failed to create pool connection mutex!");
        return false;
    }

    if (VERBOSE) {
        Serial.println("Pool connection system initialized");
    }
    return true;
}

void PoolConnection::cleanup() {
    if (xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (shared_pool_client) {
            shared_pool_client->stop();
            delete shared_pool_client;
            shared_pool_client = nullptr;
        }
        xSemaphoreGive(pool_mutex);
    }

    if (pool_mutex) {
        vSemaphoreDelete(pool_mutex);
        pool_mutex = nullptr;
    }
}

bool PoolConnection::ensureConnection() {
    if (!pool_mutex || xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
        if (DEBUG) Serial.println("Pool: Failed to acquire mutex");
        return false;
    }

    bool connected = false;

    // Create client if needed
    if (!shared_pool_client) {
        shared_pool_client = new WiFiClient();
        if (!shared_pool_client) {
            Serial.println("Pool: Failed to create WiFi client");
            xSemaphoreGive(pool_mutex);
            return false;
        }
    }

    // Check if connection needs refresh
    if (!shared_pool_client->connected() ||
        (millis() - last_pool_activity > CONNECTION_TIMEOUT)) {

        if (VERBOSE) {
            Serial.println("Pool: Establishing connection...");
        }

        shared_pool_client->stop();
        delay(500);

        // Set connection timeout
        shared_pool_client->setTimeout(30000);

        // Check WiFi connection first
        if (WiFi.status() != WL_CONNECTED) {
            if (DEBUG) Serial.println("Pool: WiFi not connected");
            xSemaphoreGive(pool_mutex);
            return false;
        }

        // Try to resolve hostname first
        IPAddress serverIP;
        if (WiFi.hostByName(config.pool_url, serverIP)) {
            if (VERBOSE) {
                Serial.printf("Pool: Resolved %s to %s\n",
                             config.pool_url, serverIP.toString().c_str());
            }

            if (shared_pool_client->connect(serverIP, config.pool_port)) {
                last_pool_activity = millis();
                connected = true;
                if (VERBOSE) Serial.println("Pool: Connection established");
            } else {
                if (DEBUG) {
                    Serial.printf("Pool: Failed to connect to %s (%s:%d)\n",
                                 config.pool_url, serverIP.toString().c_str(),
                                 config.pool_port);
                }
            }
        } else {
            if (DEBUG) {
                Serial.printf("Pool: Failed to resolve hostname: %s\n", config.pool_url);
            }

            // Fallback to direct connection attempt
            if (shared_pool_client->connect(config.pool_url, config.pool_port)) {
                last_pool_activity = millis();
                connected = true;
                if (DEBUG) Serial.println("Pool: Connection established (direct)");
            } else {
                if (DEBUG) {
                    Serial.printf("Pool: Direct connection failed to %s:%d\n",
                                 config.pool_url, config.pool_port);
                }
            }
        }
    } else {
        connected = true;
    }

    xSemaphoreGive(pool_mutex);
    return connected;
}

bool PoolConnection::sendMessage(const char* message) {
    if (!message || !pool_mutex) return false;

    if (xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return false;
    }

    bool success = false;
    if (shared_pool_client && shared_pool_client->connected()) {
        shared_pool_client->print(message);
        last_pool_activity = millis();
        success = true;
        if (DEBUG) {
            Serial.print("Pool Send: ");
            Serial.println(message);
        }
    }

    xSemaphoreGive(pool_mutex);
    return success;
}

String PoolConnection::readResponse(unsigned long timeout_ms) {
    if (!pool_mutex || xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return String();
    }

    String response;
    if (shared_pool_client && shared_pool_client->connected()) {
        unsigned long start = millis();

        while (!shared_pool_client->available() &&
               (millis() - start < timeout_ms)) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            esp_task_wdt_reset();
        }

        if (shared_pool_client->available()) {
            response = shared_pool_client->readStringUntil('\n');
            last_pool_activity = millis();
            if (VERBOSE) {
                Serial.print("Pool Recv: ");
                Serial.println(response);
            }
        }
    }

    xSemaphoreGive(pool_mutex);
    return response;
}

bool PoolConnection::isConnected() {
    if (!pool_mutex || !shared_pool_client) return false;

    bool connected = false;
    if (xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        connected = shared_pool_client->connected();
        xSemaphoreGive(pool_mutex);
    }
    return connected;
}

String PoolConnection::getStatus() {
    String status = "Pool Connection: ";
    if (isConnected()) {
        status += "Connected to " + String(config.pool_url) + ":" + String(config.pool_port);
        status += " (Last activity: " + String((millis() - last_pool_activity) / 1000) + "s ago)";
    } else {
        status += "Disconnected";
    }
    return status;
}

bool PoolConnection::submitShare(uint32_t nonce, const char* worker_name) {
    if (!ensureConnection()) {
        if (DEBUG) Serial.println("Pool: Cannot submit share - no connection");
        return false;
    }

    if (xSemaphoreTake(pool_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        if (DEBUG) Serial.println("Pool: Cannot submit share - mutex timeout");
        return false;
    }

    // Create Stratum mining.submit message
    // Format: {"params": ["username", "job_id", "extranonce2", "ntime", "nonce"], "id": X, "method": "mining.submit"}
    char share_message[512];
    static uint32_t submit_id = 3; // Start from 3 (1=subscribe, 2=authorize)

    // For now, use simplified format - many pools accept minimal shares for visibility
    snprintf(share_message, sizeof(share_message),
             "{\"params\": [\"%s\", \"job_1\", \"00000000\", \"%08x\", \"%08x\"], \"id\": %u, \"method\": \"mining.submit\"}\n",
             config.btc_address, (uint32_t)(millis() / 1000), nonce, submit_id++);

    if (VERBOSE) {
        Serial.printf("Pool: Submitting share from %s, nonce: %08x\n", worker_name, nonce);
    }
    if (DEBUG) {
        Serial.printf("Pool: Share message: %s", share_message);
    }

    shared_pool_client->print(share_message);
    last_pool_activity = millis();

    // Try to read response (non-blocking)
    unsigned long timeout = millis() + 3000; // 3 second timeout
    String response = "";
    while (millis() < timeout && shared_pool_client->available() == 0) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    if (shared_pool_client->available()) {
        response = shared_pool_client->readStringUntil('\n');
        if (VERBOSE) {
            Serial.printf("Pool: Share response: %s\n", response.c_str());
        }
    }

    xSemaphoreGive(pool_mutex);
    return true;
}