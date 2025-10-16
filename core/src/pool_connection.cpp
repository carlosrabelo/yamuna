#include "pool_connection.h"
#include "configs.h"
#include "webconfig.h"
#include "esp_task_wdt.h"
#include <ArduinoJson.h>

// Static member definitions
WiFiClient* PoolConnection::shared_pool_client = nullptr;
SemaphoreHandle_t PoolConnection::pool_mutex = nullptr;
unsigned long PoolConnection::last_pool_activity = 0;
static StratumState stratum_state = {false, false, "", 0, 1, "", {"", "", "", "", {}, 0, "", "", "", false}};
static uint32_t message_id = 1;

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

// Stratum protocol implementation
bool PoolConnection::performStratumHandshake() {
    if (!ensureConnection()) {
        if (DEBUG) Serial.println("Pool: Cannot perform handshake - no connection");
        return false;
    }

    // Reset stratum state
    stratum_state.subscribed = false;
    stratum_state.authorized = false;
    stratum_state.extranonce1 = "";
    stratum_state.extranonce2_size = 0;
    stratum_state.difficulty = 1;
    stratum_state.session_id = "";
    message_id = 1;

    if (VERBOSE) {
        Serial.println("Pool: Starting Stratum handshake...");
    }

    // Step 1: Subscribe
    if (!subscribeToPool()) {
        if (DEBUG) Serial.println("Pool: Subscribe failed");
        return false;
    }

    // Step 2: Authorize
    if (!authorizeWorker()) {
        if (DEBUG) Serial.println("Pool: Authorization failed");
        return false;
    }

    if (VERBOSE) {
        Serial.println("Pool: Stratum handshake completed successfully");
    }

    return true;
}

bool PoolConnection::subscribeToPool() {
    char subscribe_message[256];
    snprintf(subscribe_message, sizeof(subscribe_message),
             "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"%s\"]}\n",
             message_id++, "ESP32Miner/1.0");

    if (!sendMessage(subscribe_message)) {
        return false;
    }

    // Wait for response
    String response = readResponse(10000);
    if (response.length() == 0) {
        if (DEBUG) Serial.println("Pool: No response to subscribe");
        return false;
    }

    // Parse JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        if (DEBUG) Serial.printf("Pool: JSON parse error in subscribe: %s\n", error.c_str());
        return false;
    }

    // Check for error
    if (doc.containsKey("error") && !doc["error"].isNull()) {
        if (DEBUG) Serial.printf("Pool: Subscribe error: %s\n", doc["error"].as<String>().c_str());
        return false;
    }

    // Extract subscription details
    if (doc.containsKey("result") && doc["result"].is<JsonArray>()) {
        JsonArray result = doc["result"];
        if (result.size() >= 3) {
            stratum_state.extranonce1 = result[1].as<String>();
            stratum_state.extranonce2_size = result[2].as<int>();
            stratum_state.subscribed = true;

            if (VERBOSE) {
                Serial.printf("Pool: Subscribed - extranonce1: %s, extranonce2_size: %d\n",
                             stratum_state.extranonce1.c_str(), stratum_state.extranonce2_size);
            }
        }
    }

    return stratum_state.subscribed;
}

bool PoolConnection::authorizeWorker() {
    char auth_message[256];
    snprintf(auth_message, sizeof(auth_message),
             "{\"id\": %u, \"method\": \"mining.authorize\", \"params\": [\"%s\", \"x\"]}\n",
             message_id++, config.btc_address);

    if (!sendMessage(auth_message)) {
        return false;
    }

    // Wait for response
    String response = readResponse(10000);
    if (response.length() == 0) {
        if (DEBUG) Serial.println("Pool: No response to authorize");
        return false;
    }

    // Parse JSON response
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        if (DEBUG) Serial.printf("Pool: JSON parse error in authorize: %s\n", error.c_str());
        return false;
    }

    // Check for error
    if (doc.containsKey("error") && !doc["error"].isNull()) {
        if (DEBUG) Serial.printf("Pool: Authorize error: %s\n", doc["error"].as<String>().c_str());
        return false;
    }

    // Check result
    if (doc.containsKey("result") && doc["result"].as<bool>()) {
        stratum_state.authorized = true;
        if (VERBOSE) {
            Serial.printf("Pool: Authorized worker: %s\n", config.btc_address);
        }
    }

    return stratum_state.authorized;
}

bool PoolConnection::processStratumMessage(const String& message) {
    if (message.length() == 0) return false;

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        if (DEBUG) Serial.printf("Pool: JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Check if it's a method call (notification)
    if (doc.containsKey("method")) {
        String method = doc["method"].as<String>();

        if (method == "mining.notify") {
            if (doc.containsKey("params")) {
                return handleMiningNotify(doc["params"].as<String>());
            }
        } else if (method == "mining.set_difficulty") {
            if (doc.containsKey("params") && doc["params"].is<JsonArray>()) {
                JsonArray params = doc["params"];
                if (params.size() > 0) {
                    stratum_state.difficulty = params[0].as<uint32_t>();
                    if (VERBOSE) {
                        Serial.printf("Pool: Difficulty set to %u\n", stratum_state.difficulty);
                    }
                }
            }
        }
    }

    return true;
}

bool PoolConnection::handleMiningNotify(const String& params) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, params);
    if (error) {
        if (DEBUG) Serial.printf("Pool: JSON parse error in mining.notify: %s\n", error.c_str());
        return false;
    }

    if (!doc.is<JsonArray>()) {
        if (DEBUG) Serial.println("Pool: mining.notify params not an array");
        return false;
    }

    JsonArray job_params = doc.as<JsonArray>();
    if (job_params.size() < 8) {
        if (DEBUG) Serial.println("Pool: mining.notify insufficient parameters");
        return false;
    }

    // Parse job parameters
    stratum_state.current_job.job_id = job_params[0].as<String>();
    stratum_state.current_job.prevhash = job_params[1].as<String>();
    stratum_state.current_job.coinb1 = job_params[2].as<String>();
    stratum_state.current_job.coinb2 = job_params[3].as<String>();

    // Parse merkle branch
    if (job_params[4].is<JsonArray>()) {
        JsonArray merkle = job_params[4];
        stratum_state.current_job.merkle_count = min((int)merkle.size(), 16);
        for (int i = 0; i < stratum_state.current_job.merkle_count; i++) {
            stratum_state.current_job.merkle_branch[i] = merkle[i].as<String>();
        }
    }

    stratum_state.current_job.version = job_params[5].as<String>();
    stratum_state.current_job.nbits = job_params[6].as<String>();
    stratum_state.current_job.ntime = job_params[7].as<String>();

    if (job_params.size() > 8) {
        stratum_state.current_job.clean_jobs = job_params[8].as<bool>();
    }

    if (VERBOSE) {
        Serial.printf("Pool: New job %s received, difficulty %u\n",
                     stratum_state.current_job.job_id.c_str(), stratum_state.difficulty);
    }

    return true;
}

bool PoolConnection::submitStratumShare(uint32_t nonce, const String& extranonce2, const String& ntime) {
    if (!stratum_state.subscribed || !stratum_state.authorized || stratum_state.current_job.job_id.isEmpty()) {
        if (DEBUG) Serial.println("Pool: Cannot submit share - not ready");
        return false;
    }

    char share_message[512];
    snprintf(share_message, sizeof(share_message),
             "{\"id\": %u, \"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%08x\"]}\n",
             message_id++, config.btc_address, stratum_state.current_job.job_id.c_str(),
             extranonce2.c_str(), ntime.c_str(), nonce);

    if (!sendMessage(share_message)) {
        return false;
    }

    if (VERBOSE) {
        Serial.printf("Pool: Submitted share - nonce: %08x\n", nonce);
    }

    return true;
}

StratumState* PoolConnection::getStratumState() {
    return &stratum_state;
}

bool PoolConnection::hasValidJob() {
    return stratum_state.subscribed && stratum_state.authorized && !stratum_state.current_job.job_id.isEmpty();
}

String PoolConnection::getCurrentJobId() {
    return stratum_state.current_job.job_id;
}

uint32_t PoolConnection::getCurrentDifficulty() {
    return stratum_state.difficulty;
}