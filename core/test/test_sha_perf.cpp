#include <Arduino.h>
#include <unity.h>
#include "../src/sha256_esp32.h"

void setUp(void) {
    // set up operations if needed
}

void tearDown(void) {
    // clean up operations if needed
}

void test_sha256_performance(void) {
    sha256_benchmark_t result;
    
    // Run the benchmark function
    // Note: The benchmark itself will print to Serial
    sha256_esp32_benchmark(&result);

    // Assert that hardware is significantly faster than software
    // (e.g., at least 10x faster)
    TEST_ASSERT_GREATER_THAN_UINT32(result.software_hps, result.hardware_hps);

    // Assert that the hardware hashrate is above a reasonable minimum
    // For double-SHA256, a 240MHz ESP32 should be well above 10,000 H/s
    TEST_ASSERT_GREATER_THAN_UINT32(10000, result.hardware_hps);
}

void setup() {
    // Wait for a moment to connect the serial monitor
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_sha256_performance);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
