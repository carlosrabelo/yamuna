#include "webconfig.h"
#include "configs.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>

// Global instances
YamunaConfig config;
WebServer server(WEB_PORT);
Preferences preferences;

// Forward declarations
void handleRoot();
void handleSave();
void handleReset();
String processor(const String& var);

void initWebConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("!!! SPIFFS mount FAILED !!!");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");
    preferences.begin("yamuna", false);
    loadConfig();
}

void listSpiffsFiles() {
    Serial.println("--- Listing files on SPIFFS ---");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
        Serial.printf("  FILE: %s  SIZE: %d\n", file.name(), file.size());
        file = root.openNextFile();
    }
    Serial.println("---------------------------------");
}

void startConfigAP() {
    Serial.println("Starting Access Point...");
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/reset", HTTP_GET, handleReset);
    server.begin();
    Serial.println("Web server started.");
}

void handleConfigWeb() {
    server.handleClient();
}

String processor(const String& var) {
    if (var == "WIFI_SSID") return config.wifi_ssid;
    if (var == "WIFI_PASSWORD") return "";
    if (var == "BTC_ADDRESS") return config.btc_address;
    if (var == "POOL_URL") return config.pool_url;
    if (var == "POOL_PORT") return String(config.pool_port);
    return String();
}

void handleRoot() {
    Serial.println("-> handleRoot called");
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        Serial.println("!!! handleRoot: FAILED to open /index.html");
        server.send(404, "text/plain", "File Not Found");
        return;
    }
    Serial.printf("handleRoot: /index.html opened successfully, size: %d\n", file.size());

    String page = file.readString();
    file.close();

    page.replace("%WIFI_SSID%", processor("WIFI_SSID"));
    page.replace("%WIFI_PASSWORD%", processor("WIFI_PASSWORD"));
    page.replace("%BTC_ADDRESS%", processor("BTC_ADDRESS"));
    page.replace("%POOL_URL%", processor("POOL_URL"));
    page.replace("%POOL_PORT%", processor("POOL_PORT"));
    page.replace("%THREADS_1%", processor("THREADS_1"));
    page.replace("%THREADS_2%", processor("THREADS_2"));

    server.send(200, "text/html", page);
}

void handleSave() {
    String wifi_ssid = server.arg("wifi_ssid");
    String wifi_password = server.arg("wifi_password");
    String btc_address = server.arg("btc_address");
    String pool_password = server.arg("pool_password");
    String pool_url = server.arg("pool_url");
    int pool_port = server.arg("pool_port").toInt();

    if (wifi_ssid.length() > 0 && btc_address.length() > 0) {
        strncpy(config.wifi_ssid, wifi_ssid.c_str(), sizeof(config.wifi_ssid) - 1);
        if (wifi_password.length() > 0) {
            strncpy(config.wifi_password, wifi_password.c_str(), sizeof(config.wifi_password) - 1);
        }
        strncpy(config.btc_address, btc_address.c_str(), sizeof(config.btc_address) - 1);
        strncpy(config.pool_url, pool_url.c_str(), sizeof(config.pool_url) - 1);
        config.pool_port = pool_port;
        config.configured = true;
        saveConfig();
        server.send_P(200, "text/html", SPIFFS.open("/success.html", "r").readString().c_str());
        delay(3000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Missing required fields");
    }
}

void handleReset() {
    preferences.clear();
    server.send(200, "text/plain", "Configuration reset. Restarting...");
    delay(2000);
    ESP.restart();
}

void saveConfig() {
    preferences.putString("wifi_ssid", config.wifi_ssid);
    preferences.putString("wifi_password", config.wifi_password);
    preferences.putString("btc_address", config.btc_address);
    preferences.putString("pool_url", config.pool_url);
    preferences.putInt("pool_port", config.pool_port);
    preferences.putBool("configured", true);
}

void loadConfig() {
    config.configured = preferences.getBool("configured", false);
    if (config.configured) {
        strncpy(config.wifi_ssid, preferences.getString("wifi_ssid", "").c_str(), sizeof(config.wifi_ssid));
        strncpy(config.wifi_password, preferences.getString("wifi_password", "").c_str(), sizeof(config.wifi_password));
        strncpy(config.btc_address, preferences.getString("btc_address", DEFAULT_ADDRESS).c_str(), sizeof(config.btc_address));
        strncpy(config.pool_url, preferences.getString("pool_url", DEFAULT_POOL_URL).c_str(), sizeof(config.pool_url));
        config.pool_port = preferences.getInt("pool_port", 0);
    }
}

bool isConfigured() {
    return config.configured;
}
