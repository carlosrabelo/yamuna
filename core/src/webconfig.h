// YAMUNA Web Configuration Interface
// Simple web-based configuration like NerdMiner

#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Configuration structure
struct YamunaConfig {
    char wifi_ssid[32];
    char wifi_password[64];
    char pool_url[64];
    int pool_port;
    char btc_address[64];
    char pool_password[64];
    bool configured;
    bool use_yuma;          // Use YUMA proxy instead of traditional pool
    char yuma_ip[16];       // Discovered YUMA IP address
    int yuma_port;          // YUMA port (default 3334)
};

// Global config instance
extern YamunaConfig config;
extern WebServer server;
extern Preferences preferences;

// Function declarations
void initWebConfig();
void startConfigAP();
void handleConfigWeb();
void listSpiffsFiles();
void saveConfig();
void loadConfig();
bool isConfigured();
void resetConfig();

// Web page templates
const char* getConfigPage();
const char* getSuccessPage();

#endif // WEBCONFIG_H