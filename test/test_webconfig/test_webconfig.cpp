#define UNIT_TEST

#include <cstring>
#include <unity.h>

#include "configs.h"
#include "webconfig.h"

extern YamunaConfig config;
extern Preferences preferences;

static void given_config_with_sample_values() {
    std::memset(&config, 0, sizeof(config));
    config.configured = true;
    std::strncpy(config.wifi_ssid, "TestSSID", sizeof(config.wifi_ssid) - 1);
    std::strncpy(config.wifi_password, "TestPassword", sizeof(config.wifi_password) - 1);
    std::strncpy(config.btc_address, "bc1qsampleaddress", sizeof(config.btc_address) - 1);
    std::strncpy(config.pool_url, "pool.example.org", sizeof(config.pool_url) - 1);
    std::strncpy(config.pool_password, "SuperSecret", sizeof(config.pool_password) - 1);
    config.pool_port = 12345;
}

void setUp() {
    preferences.begin("yamuna", false);
    preferences.clear();
    std::memset(&config, 0, sizeof(config));
}

void tearDown() {}

static void test_save_and_load_persists_all_fields() {
    given_config_with_sample_values();
    saveConfig();

    std::memset(&config, 0, sizeof(config));
    loadConfig();

    TEST_ASSERT_TRUE(config.configured);
    TEST_ASSERT_EQUAL_STRING("TestSSID", config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("TestPassword", config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("bc1qsampleaddress", config.btc_address);
    TEST_ASSERT_EQUAL_STRING("pool.example.org", config.pool_url);
    TEST_ASSERT_EQUAL_INT(12345, config.pool_port);
    TEST_ASSERT_EQUAL_STRING("SuperSecret", config.pool_password);
}

static void test_save_preserves_wifi_password_when_empty_input() {
    given_config_with_sample_values();
    saveConfig();

    // Simulate user saving again updating only pool password
    std::strncpy(config.pool_password, "UpdatedSecret", sizeof(config.pool_password) - 1);
    saveConfig();

    std::memset(&config, 0, sizeof(config));
    loadConfig();

    TEST_ASSERT_EQUAL_STRING("TestPassword", config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("UpdatedSecret", config.pool_password);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_save_and_load_persists_all_fields);
    RUN_TEST(test_save_preserves_wifi_password_when_empty_input);
    return UNITY_END();
}
