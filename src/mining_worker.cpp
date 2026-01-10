#include "mining_worker.h"
#include "mining_utils.h"
#include "pool_connection.h"
#include "sha256_optimized.h"
#include "configs.h"
#include "esp_task_wdt.h"
#include <ArduinoJson.h>

// MiningWorker implementation
MiningWorker::MiningWorker(const char* name) {
    strlcpy(worker_name, name, sizeof(worker_name));
    worker_id = worker_name[7] - '0';
    current_nonce_start = 0;
    current_nonce_end = 0;
    initMidstateCache(&midstate_cache);
}

bool MiningWorker::initialize() {
    if (DEBUG) {
        Serial.printf("\nInitializing %s on core %d\n", worker_name, xPortGetCoreID());
    }

    // Stagger worker startup to avoid resource conflicts
    delay(worker_id * WORKER_STAGGER_MS);

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

        // Perform Stratum handshake if not done
        if (!PoolConnection::hasValidJob()) {
            if (!PoolConnection::performStratumHandshake()) {
                Serial.printf("%s: Stratum handshake failed, retrying...\n", worker_name);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            // Wait for mining.notify
            unsigned long wait_start = millis();
            while (!PoolConnection::hasValidJob() && (millis() - wait_start < JOB_WAIT_TIMEOUT_MS)) {
                String message = PoolConnection::readResponse(1000);
                if (message.length() > 0) {
                    PoolConnection::processStratumMessage(message);
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            if (!PoolConnection::hasValidJob()) {
                Serial.printf("%s: No valid job received, retrying...\n", worker_name);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        // Set mining range for this worker
        current_nonce_start = worker_id * NONCE_RANGE_SIZE;
        current_nonce_end = current_nonce_start + NONCE_RANGE_SIZE;

        if (VERBOSE) {
            Serial.printf("%s: Mining job %s, range %u - %u\n", worker_name,
                         PoolConnection::getCurrentJobId().c_str(), current_nonce_start, current_nonce_end);
        }

        // Update adaptive difficulty
        updateAdaptiveDifficulty();

        // Process mining range
        if (processMiningRange(current_nonce_start, current_nonce_end)) {
            Serial.printf("%s: Completed mining cycle\n", worker_name);
        }

        // No delay here to maximize performance
        vTaskDelay(pdMS_TO_TICKS(1)); // Minimal delay to allow task switching
    }
}

bool MiningWorker::processMiningRange(uint32_t start_nonce, uint32_t end_nonce) {
    // Check if we have a valid Stratum job
    if (!PoolConnection::hasValidJob()) {
        if (DEBUG) Serial.printf("%s: No valid job available\n", worker_name);
        return false;
    }

    // Generate extranonce2 for this worker
    char extranonce2[9];
    snprintf(extranonce2, sizeof(extranonce2), "%08x", worker_id);
    String extranonce2_str = String(extranonce2);
    
    StratumState* state = PoolConnection::getStratumState();
    bool job_changed = !midstate_cache.valid || 
                       midstate_cache.job_id != state->current_job.job_id;
    
    if (job_changed) {
        uint8_t block_header[80];
        if (!buildBlockHeader(block_header, 0, extranonce2_str)) {
            if (DEBUG) Serial.printf("%s: Failed to build block header\n", worker_name);
            return false;
        }
        updateMidstateCache(&midstate_cache, block_header);
        midstate_cache.job_id = state->current_job.job_id;
        
        if (VERBOSE) {
            Serial.printf("%s: Midstate updated for job %s\n", worker_name, 
                         midstate_cache.job_id.c_str());
        }
    }

    // Create tail data with nonce at position 12 (offset in 64-byte block after midstate)
    uint8_t mining_tail[16];
    
    for(uint32_t nonce = start_nonce; nonce < end_nonce; nonce++) {
        // Build mining tail: ntime(4) + bits(4) + nonce(4) + padding(4)
        memcpy(mining_tail, midstate_cache.tail_data, 12);  // ntime + bits
        *(uint32_t*)(mining_tail + 12) = nonce;
        
        // Use optimized midstate SHA-256 (first half of double hash)
        uint8_t hash_result[32];
        sha256_bitcoin_hash_fast(midstate_cache.midstate, midstate_cache.midstate2,
                                  mining_tail, 16, hash_result);
        
        // Fast reject: check first 2 bytes for quick rejection
        if (!sha256_check_fast_reject(hash_result, 2)) {
            // Not even a half-share, continue
            __atomic_fetch_add(&hashes, 1, __ATOMIC_RELAXED);
            
            // Reset watchdog periodically
            if ((nonce % 65536) == 0) {
                esp_task_wdt_reset();
            }
            continue;
        }
        
        // Check results using real Stratum difficulty
        uint32_t pool_difficulty = PoolConnection::getCurrentDifficulty();
        
        if(checkStratumTarget(hash_result, pool_difficulty)) {
            if (VERBOSE) {
                Serial.printf("%s: VALID SHARE! nonce: %u, difficulty: %u\n", worker_name, nonce, pool_difficulty);
            }
            shares++;
            
            // Submit share to pool with proper Stratum format
            StratumState* st = PoolConnection::getStratumState();
            String ntime_str = st->current_job.ntime;
            PoolConnection::submitStratumShare(nonce, extranonce2_str, ntime_str);
        } else if(checkShare(hash_result)) {
            // Local share for statistics (easier difficulty)
            if (VERBOSE) {
                Serial.printf("%s: Half-share found, nonce: %u\n", worker_name, nonce);
            }
            halfshares++;
        }
        
        __atomic_fetch_add(&hashes, 1, __ATOMIC_RELAXED);

        // Reset watchdog periodically and check for new jobs
        if ((nonce % (NONCE_BATCH_SIZE * 16)) == 0) {
            esp_task_wdt_reset();
            vTaskDelay(1);

            // Only worker 0 should check for new jobs to prevent mutex contention
            if (worker_id == 0) {
                String message = PoolConnection::readResponse(10);
                if (message.length() > 0) {
                    PoolConnection::processStratumMessage(message);
                }
            }

            // Show progress for debugging
            if ((nonce % 65536) == 0 && VERBOSE) {
                Serial.printf("%s: Processed %u hashes, job: %s\n", worker_name,
                             nonce - start_nonce, PoolConnection::getCurrentJobId().c_str());
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
    static unsigned long last_hashes = 0;

    unsigned long now = millis();
    unsigned long elapsed = now - start;
    unsigned long interval = now - last_report;

    if (interval >= 5000) { // Update every 5 seconds
        unsigned long hash_diff = __atomic_load_n(&hashes, __ATOMIC_RELAXED) - last_hashes;
        float instant_rate = (interval > 100) ? (hash_diff * 1000.0) / interval / 1000.0 : 0;
        float avg_rate = (elapsed > 1000) ? (hashes * 1000.0) / elapsed / 1000.0 : 0;

        uint32_t stratum_diff = PoolConnection::hasValidJob() ? PoolConnection::getCurrentDifficulty() : 1;
        String current_job = PoolConnection::getCurrentJobId();

        if (PoolConnection::hasValidJob()) {
            if (VERBOSE) {
                // Detailed output when VERBOSE=1
                Serial.printf(">>> Shares: %d | Hashes: %lu | Avg: %.2f KH/s | Current: %.2f KH/s | Temp: %.1f°C | Stratum Diff: %u | Job: %s\n",
                            shares, __atomic_load_n(&hashes, __ATOMIC_RELAXED), avg_rate, instant_rate, temperatureRead(), stratum_diff, current_job.c_str());
            } else {
                // Clean cpuminer-style output when VERBOSE=0
                Serial.printf("[%7.2f KH/s] %d shares, %.1fC, diff %u, job %s\n",
                            instant_rate, shares, temperatureRead(), stratum_diff, current_job.c_str());
            }
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
    stats += "  Local Difficulty Level: " + String(getCurrentDifficultyLevel()) + "\n";
    stats += "  Stratum Difficulty: " + String(PoolConnection::getCurrentDifficulty()) + "\n";
    stats += "  Current Job: " + PoolConnection::getCurrentJobId() + "\n";

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