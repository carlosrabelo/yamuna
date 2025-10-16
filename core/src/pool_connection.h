#ifndef POOL_CONNECTION_H
#define POOL_CONNECTION_H

#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Connection constants
#define CONNECTION_TIMEOUT 300000 // 5 minutes

// Stratum protocol structures
struct StratumJob {
    String job_id;
    String prevhash;
    String coinb1;
    String coinb2;
    String merkle_branch[16];
    int merkle_count;
    String version;
    String nbits;
    String ntime;
    bool clean_jobs;
};

struct StratumState {
    bool subscribed;
    bool authorized;
    String extranonce1;
    int extranonce2_size;
    uint32_t difficulty;
    String session_id;
    StratumJob current_job;
};

// Pool connection management
class PoolConnection {
private:
    static WiFiClient* shared_pool_client;
    static SemaphoreHandle_t pool_mutex;
    static unsigned long last_pool_activity;

public:
    // Initialize pool connection system
    static bool initialize();

    // Cleanup pool connection system
    static void cleanup();

    // Ensure shared connection is available
    static bool ensureConnection();

    // Send message to pool (thread-safe)
    static bool sendMessage(const char* message);

    // Read response from pool (thread-safe)
    static String readResponse(unsigned long timeout_ms = 10000);

    // Check if connected
    static bool isConnected();

    // Get connection status
    static String getStatus();

    // Submit share to pool
    static bool submitShare(uint32_t nonce, const char* worker_name);

    // Stratum protocol functions
    static bool performStratumHandshake();
    static bool subscribeToPool();
    static bool authorizeWorker();
    static bool processStratumMessage(const String& message);
    static bool handleMiningNotify(const String& params);
    static bool submitStratumShare(uint32_t nonce, const String& extranonce2, const String& ntime);

    // Get current Stratum state
    static StratumState* getStratumState();
    static bool hasValidJob();
    static String getCurrentJobId();
    static uint32_t getCurrentDifficulty();
};

#ifdef __cplusplus
}
#endif

#endif // POOL_CONNECTION_H