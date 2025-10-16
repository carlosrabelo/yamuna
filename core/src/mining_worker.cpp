#include "mining_worker.h"
#include "mining_utils.h"
#include "pool_connection.h"
#include "sha256_esp32.h"
#include "configs.h"
#include "esp_task_wdt.h"
#include <ArduinoJson.h>

// MiningWorker implementation
MiningWorker::MiningWorker(const char* name) {
    strlcpy(worker_name, name, sizeof(worker_name));
    worker_id = worker_name[7] - '0'; // Extract number from "Worker[N]"
    current_nonce_start = 0;
    current_nonce_end = 0;
}

bool MiningWorker::initialize() {
    if (DEBUG) {
        Serial.printf("\nInitializing %s on core %d\n", worker_name, xPortGetCoreID());
    }

    // Stagger worker startup to avoid resource conflicts
    delay(worker_id * 2000);

    return true;
}

void MiningWorker::mineLoop() {
    while(true) {
        esp_task_wdt_reset();

        // Check pool connection
        if (!PoolConnection::ensureConnection()) {
            Serial.printf("%s: Pool connection failed, retrying in 10 seconds...\n", worker_name);
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        // Set mining range for this worker
        uint32_t range_size = 1000000; // 1M nonces per cycle
        current_nonce_start = worker_id * range_size;
        current_nonce_end = current_nonce_start + range_size;

        if (VERBOSE) {
            Serial.printf("%s: Mining range %u - %u\n", worker_name,
                         current_nonce_start, current_nonce_end);
        }

        // Update adaptive difficulty
        updateAdaptiveDifficulty();

        // Process mining range
        if (processMiningRange(current_nonce_start, current_nonce_end)) {
            Serial.printf("%s: Completed mining cycle\n", worker_name);
        }

        // Brief pause before next cycle
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool MiningWorker::processMiningRange(uint32_t start_nonce, uint32_t end_nonce) {
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    char line[2048];

    for(uint32_t nonce = start_nonce; nonce < end_nonce; nonce++) {
        // Simulate block header with nonce
        uint8_t block_header[80];
        memset(block_header, 0, 80);
        *(uint32_t*)(block_header + 76) = nonce;

        // Use optimized ESP32 SHA-256
        uint8_t hash_result[32];
        sha256_esp32_bitcoin_hash(block_header, hash_result);

        // Check results
        if(checkShare(hash_result)) {
            if (VERBOSE) {
                Serial.printf("%s: Share found! nonce: %u\n", worker_name, nonce);
            }
            shares++;

            // Could submit share to pool here if needed
            // PoolConnection::sendMessage(share_payload);

        } else if(checkHalfShare(hash_result)) {
            if (VERBOSE) {
                Serial.printf("%s: Half share, nonce: %u\n", worker_name, nonce);
            }
            halfshares++;
        }

        hashes++;

        // Reset watchdog periodically
        if ((nonce % (NONCE_BATCH_SIZE * 16)) == 0) {
            esp_task_wdt_reset();
            vTaskDelay(1);

            // Show progress for debugging
            if ((nonce % 65536) == 0 && VERBOSE) {
                Serial.printf("%s: Processed %u hashes\n", worker_name,
                             nonce - start_nonce);
            }
        }
    }

    return true;
}

String MiningWorker::getStats() {
    String stats = String(worker_name) + ": ";
    stats += "Range " + String(current_nonce_start) + "-" + String(current_nonce_end);
    return stats;
}

// FreeRTOS task wrapper
void runOptimizedWorker(void *name) {
    MiningWorker worker((char*)name);

    if (!worker.initialize()) {
        Serial.printf("Failed to initialize %s\n", (char*)name);
        vTaskDelete(NULL);
        return;
    }

    worker.mineLoop();
}

// MiningMonitor implementation
void MiningMonitor::start() {
    if (DEBUG) {
        Serial.println("Starting mining monitor");
    }
}

void MiningMonitor::updateDisplay() {
    static unsigned long start = millis();
    static unsigned long last_report = start;
    static long last_hashes = 0;

    unsigned long now = millis();
    unsigned long elapsed = now - start;
    unsigned long interval = now - last_report;

    if (interval >= 5000) { // Update every 5 seconds
        long hash_diff = hashes - last_hashes;
        float instant_rate = (interval > 100) ? (hash_diff * 1000.0) / interval / 1000.0 : 0;
        float avg_rate = (elapsed > 1000) ? (hashes * 1000.0) / elapsed / 1000.0 : 0;

        if (VERBOSE) {
            // Detailed output when VERBOSE=1
            Serial.printf(">>> Shares: %d | Hashes: %ld | Avg: %.2f KH/s | Current: %.2f KH/s | Temp: %.1f°C | Diff: %d\n",
                         shares, hashes, avg_rate, instant_rate, temperatureRead(), getCurrentDifficultyLevel());
        } else {
            // Clean cpuminer-style output when VERBOSE=0
            Serial.printf("[%.2f KH/s] %d shares, %.1f°C, diff %d\n",
                         instant_rate, shares, temperatureRead(), getCurrentDifficultyLevel());
        }

        last_hashes = hashes;
        last_report = now;
    }
}

String MiningMonitor::getStatistics() {
    unsigned long uptime = millis() / 1000;
    float avg_rate = (uptime > 0) ? (hashes * 1.0) / uptime / 1000.0 : 0;

    String stats = "Mining Statistics:\n";
    stats += "  Uptime: " + String(uptime) + "s\n";
    stats += "  Total Hashes: " + String(hashes) + "\n";
    stats += "  Shares Found: " + String(shares) + "\n";
    stats += "  Half-Shares: " + String(halfshares) + "\n";
    stats += "  Average Rate: " + String(avg_rate, 2) + " KH/s\n";
    stats += "  Temperature: " + String(temperatureRead(), 1) + "°C\n";
    stats += "  Difficulty Level: " + String(getCurrentDifficultyLevel()) + "\n";

    unsigned long time_since_share = millis() - last_share_time;
    stats += "  Time Since Last Share: " + String(time_since_share / 1000) + "s\n";

    return stats;
}

// FreeRTOS monitor task
void runMonitor(void *name) {
    MiningMonitor::start();

    while (1) {
        MiningMonitor::updateDisplay();
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}