#ifndef MINING_WORKER_H
#define MINING_WORKER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mining worker management
class MiningWorker {
private:
    char worker_name[16];
    int worker_id;
    uint32_t current_nonce_start;
    uint32_t current_nonce_end;

public:
    // Constructor
    MiningWorker(const char* name);

    // Initialize worker
    bool initialize();

    // Main mining loop
    void mineLoop();

    // Process mining range
    bool processMiningRange(uint32_t start_nonce, uint32_t end_nonce);

    // Get worker statistics
    String getStats();

    // Get worker name
    const char* getName() const { return worker_name; }

    // Get worker ID
    int getWorkerId() const { return worker_id; }
};

// Task functions for FreeRTOS
void runOptimizedWorker(void *name);
void runMonitor(void *name);

// Monitor functions
class MiningMonitor {
public:
    // Start monitoring
    static void start();

    // Update statistics display
    static void updateDisplay();

    // Get mining statistics
    static String getStatistics();
};

#ifdef __cplusplus
}
#endif

#endif // MINING_WORKER_H