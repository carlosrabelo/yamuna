#include <Arduino.h>
#include <unity.h>
#include "../src/sha256_optimized.h"

void setUp(void) {
    // set up operations if needed
}

void tearDown(void) {
    // clean up operations if needed
}

void test_sha256_performance(void) {
    sha256_benchmark_t result;
    
    sha256_esp32_benchmark(&result);

#if USE_HW_SHA256
    TEST_ASSERT_GREATER_THAN_UINT32(result.software_hps, result.hardware_hps);
    TEST_ASSERT_GREATER_THAN_UINT32(10000, result.hardware_hps);
#else
    TEST_ASSERT_GREATER_THAN_UINT32(1000, result.software_hps);
#endif
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
