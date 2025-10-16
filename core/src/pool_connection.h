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
};

#ifdef __cplusplus
}
#endif

#endif // POOL_CONNECTION_H