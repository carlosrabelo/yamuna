#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "configs.h"
#include "webconfig.h"
#include "sha256_esp32.h"
#include "mining_utils.h"
#include "pool_connection.h"
#include "mining_worker.h"
#include "esp_task_wdt.h"

// System initialization functions
bool initializeHardware() {
    Serial.begin(115200);
    delay(500);

    // Disable watchdog timers for mining cores
    disableCore0WDT();
    disableCore1WDT();

    // Set CPU frequency to maximum for better performance
    setCpuFrequencyMhz(240);

    // Configure task watchdog for recovery
    esp_task_wdt_init(120, false);

    if (VERBOSE) {
        Serial.println("Hardware initialized successfully");
    }
    return true;
}

bool initializeWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(config.wifi_ssid, config.wifi_password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi connection failed - starting configuration portal");
        startConfigAP();
        while (true) {
            handleConfigWeb();
            delay(100);
        }
        return false;
    }

    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());

    return true;
}

bool configureDevice() {
    // Initialize web configuration
    initWebConfig();

    // Check if configured
    bool configured = isConfigured();
    Serial.printf("Device configured: %s\n", configured ? "YES" : "NO");

    if (!configured) {
        Serial.println("First time setup - starting configuration portal");
        startConfigAP();

        while (!isConfigured()) {
            handleConfigWeb();
            delay(100);
        }
    } else {
        Serial.println("Device already configured");
    }

    return true;
}

void displayConfiguration() {
    if (VERBOSE) {
        Serial.println("\n=== YAMUNA Miner Configuration ===");
        Serial.println("Bitcoin mining powered by ESP32 with modular architecture");
        Serial.printf("Pool: %s:%d\n", config.pool_url, config.pool_port);
        Serial.printf("Address: %s\n", config.btc_address);
        Serial.println("===================================\n");
    } else {
        Serial.printf("Pool: %s:%d\n", config.pool_url, config.pool_port);
        Serial.println("Starting miner...");
    }
}

bool runBenchmarks() {
    if (VERBOSE) {
        Serial.println("Running system benchmarks...");
    }

    // SHA-256 benchmark
    sha256_benchmark_t benchmark;
    sha256_esp32_benchmark(&benchmark);

    if (VERBOSE) {
        Serial.printf("SHA-256 Performance: %u H/s (optimized)\n", benchmark.optimized_hps);

        // Pool connection test
        if (PoolConnection::ensureConnection()) {
            Serial.println("Pool connection test: PASSED");
            Serial.println(PoolConnection::getStatus());
        } else {
            Serial.println("Pool connection test: FAILED");
            return false;
        }
    } else {
        // Just ensure connection without verbose output
        if (!PoolConnection::ensureConnection()) {
            Serial.println("Pool connection failed!");
            return false;
        }
    }

    return true;
}

bool startMiningTasks() {
    if (VERBOSE) {
        Serial.println("Starting modular mining tasks...");
    }

    // Auto-detect number of cores and start mining workers (max 2)
    int num_workers = min((int)ESP.getChipCores(), 2);
    if (num_workers == 1) {
        Serial.println("Single-core ESP32 detected, starting 1 worker task.");
    } else {
        Serial.printf("%d-core ESP32 detected, starting %d worker tasks.\n", num_workers, num_workers);
    }
    char worker_names[num_workers][16];
    TaskHandle_t worker_handles[num_workers];

    for (int i = 0; i < num_workers; i++) {
        snprintf(worker_names[i], sizeof(worker_names[i]), "Worker[%d]", i);
        BaseType_t res = xTaskCreate(
            runOptimizedWorker,
            worker_names[i],
            12288, // Optimized stack size
            (void*)worker_names[i],
            2, // High priority for mining
            &worker_handles[i]
        );

        if (res == pdPASS) {
            if (VERBOSE) {
                Serial.printf("Started modular %s successfully on core %d\n",
                             worker_names[i], i % 2);
            }
        } else {
            Serial.printf("Failed to start %s!\n", worker_names[i]);
            return false;
        }
    }

    // Start monitor task
    TaskHandle_t monitor_handle;
    BaseType_t monitor_res = xTaskCreate(runMonitor, "Monitor", 4096, NULL, 1, &monitor_handle);
    if (monitor_res == pdPASS) {
        if (VERBOSE) {
            Serial.println("Monitor task started successfully");
        }
    } else {
        Serial.println("Failed to start monitor task!");
        return false;
    }

    return true;
}

void setup() {
    if (VERBOSE) {
        Serial.println("\n=== YAMUNA Miner - Modular Architecture ===");
    } else {
        Serial.println("\nYAMUNA Miner v1.0");
    }

    // Step 1: Initialize hardware
    if (!initializeHardware()) {
        Serial.println("Hardware initialization failed!");
        ESP.restart();
    }

    // Step 2: Configure device
    if (!configureDevice()) {
        Serial.println("Device configuration failed!");
        ESP.restart();
    }

    // List SPIFFS files for diagnostics
    listSpiffsFiles();

    // Step 3: Initialize WiFi
    if (!initializeWiFi()) {
        Serial.println("WiFi initialization failed!");
        ESP.restart();
    }

    // Step 4: Initialize pool connection system
    if (!PoolConnection::initialize()) {
        Serial.println("Pool connection initialization failed!");
        ESP.restart();
    }

    // Step 5: Initialize adaptive difficulty system
    initAdaptiveDifficulty();

    // Step 6: Display configuration
    displayConfiguration();

    // Step 7: Run benchmarks
    if (!runBenchmarks()) {
        Serial.println("Benchmark failed - continuing anyway...");
    }

    // Step 8: Start mining tasks
    if (!startMiningTasks()) {
        Serial.println("Failed to start mining tasks!");
        ESP.restart();
    }

    Serial.println("=== System fully initialized ===");
    Serial.println("Mining operations started successfully!");
}

void loop() {
    // Main loop kept minimal - all work done in tasks
    esp_task_wdt_reset();

    // Optional: Display periodic status updates
    static unsigned long last_status = 0;
    if (millis() - last_status > 30000) { // Every 30 seconds
        if (VERBOSE) {
            Serial.println("\n--- System Status ---");
            Serial.println(PoolConnection::getStatus());
            Serial.println(MiningMonitor::getStatistics());
            Serial.println("-------------------\n");
        }
        last_status = millis();
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // 10 second delay
}