// YAMUNA Web Configuration Implementation

#include "webconfig.h"
#include "configs.h"

// Global instances
YamunaConfig config;
WebServer server(WEB_PORT);
Preferences preferences;


// Forward declarations
void handleRoot();
void handleSave();
void handleReset();
// Web page HTML templates
const char* CONFIG_PAGE = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>YAMUNA Miner Setup</title>\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
"    <style>\n"
"        body { font-family: Arial; margin: 40px; background: #f0f0f0; }\n"
"        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n"
"        h1 { color: #333; text-align: center; margin-bottom: 30px; }\n"
"        .form-group { margin-bottom: 20px; }\n"
"        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }\n"
"        input, select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; }\n"
"        button { background: #007bff; color: white; padding: 12px 30px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 100%; }\n"
"        button:hover { background: #0056b3; }\n"
"        .info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; border-left: 4px solid #007bff; }\n"
"        .footer { text-align: center; margin-top: 30px; color: #666; font-size: 14px; }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h1>YAMUNA Miner Setup</h1>\n"
"\n"
"        <div class=\"info\">\n"
"            <strong>Welcome!</strong> Configure your YAMUNA Bitcoin miner settings below.\n"
"        </div>\n"
"\n"
"        <form action=\"/save\" method=\"POST\">\n"
"            <div class=\"form-group\">\n"
"                <label>WiFi Network:</label>\n"
"                <input type=\"text\" name=\"wifi_ssid\" value=\"%WIFI_SSID%\" placeholder=\"Your WiFi network name\" required>\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>WiFi Password:</label>\n"
"                <input type=\"password\" name=\"wifi_password\" value=\"%WIFI_PASSWORD%\" placeholder=\"Your WiFi password\" required>\n"
"            </div>\n"
"\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Username (Bitcoin Address):</label>\n"
"                <input type=\"text\" name=\"btc_address\" value=\"%BTC_ADDRESS%\" placeholder=\"bc1q... or your mining username\" required>\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Password:</label>\n"
"                <input type=\"text\" name=\"pool_password\" value=\"%POOL_PASSWORD%\" placeholder=\"Leave blank for most pools\">\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Mining Pool:</label>\n"
"                <select name=\"pool_preset\" onchange=\"updatePool(this.value)\" id=\"pool_preset\">\n"
"                    <option value=\"public-pool\">Public Pool (Recommended)</option>\n"
"                    <option value=\"solo-ck\">Solo CK Pool (Solo Mining)</option>\n"
"                    <option value=\"custom\">Custom Pool</option>\n"
"                </select>\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Pool URL:</label>\n"
"                <input type=\"text\" name=\"pool_url\" id=\"pool_url\" value=\"%POOL_URL%\" placeholder=\"public-pool.io\">\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Pool Port:</label>\n"
"                <input type=\"number\" name=\"pool_port\" id=\"pool_port\" value=\"%POOL_PORT%\" placeholder=\"21496\">\n"
"            </div>\n"
"\n"
"            <div class=\"form-group\">\n"
"                <label>Mining Threads:</label>\n"
"                <select name=\"threads\">\n"
"                    <option value=\"1\" %THREADS_1%>1 Thread (Stable, Lower Power)</option>\n"
"                    <option value=\"2\" %THREADS_2%>2 Threads (Higher Hashrate)</option>\n"
"                </select>\n"
"            </div>\n"
"\n"
"            <button type=\"submit\">Save & Start Mining</button>\n"
"        </form>\n"
"\n"
"        <div class=\"footer\">\n"
"            YAMUNA Bitcoin Miner v1.0<br>\n"
"            Connect to your home WiFi after setup\n"
"        </div>\n"
"    </div>\n"
"\n"
"    <script>\n"
"        function updatePool(preset) {\n"
"            var urlField = document.getElementById(\"pool_url\");\n"
"            var portField = document.getElementById(\"pool_port\");\n"
"\n"
"            if (preset === \"public-pool\") {\n"
"                urlField.value = \"public-pool.io\";\n"
"                portField.value = \"21496\";\n"
"            } else if (preset === \"solo-ck\") {\n"
"                urlField.value = \"solo.ckpool.org\";\n"
"                portField.value = \"3333\";\n"
"            }\n"
"        }\n"
"\n"
"        function detectCurrentPool() {\n"
"            var urlField = document.getElementById(\"pool_url\");\n"
"            var portField = document.getElementById(\"pool_port\");\n"
"            var presetField = document.getElementById(\"pool_preset\");\n"
"\n"
"            var currentUrl = urlField.value;\n"
"            var currentPort = portField.value;\n"
"\n"
"            if (currentUrl === \"public-pool.io\" && currentPort === \"21496\") {\n"
"                presetField.value = \"public-pool\";\n"
"            } else if (currentUrl === \"solo.ckpool.org\" && currentPort === \"3333\") {\n"
"                presetField.value = \"solo-ck\";\n"
"            } else {\n"
"                presetField.value = \"custom\";\n"
"            }\n"
"        }\n"
"\n"
"        // Auto-detect pool when page loads\n"
"        window.onload = function() {\n"
"            detectCurrentPool();\n"
"        };\n"
"    </script>\n"
"</body>\n"
"</html>\n";

const char* SUCCESS_PAGE = R"(
<!DOCTYPE html>
<html>
<head>
    <title>YAMUNA Setup Complete</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="refresh" content="10;url=/">
    <style>
        body { font-family: Arial; margin: 40px; background: #f0f0f0; text-align: center; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .success { color: #28a745; font-size: 48px; margin-bottom: 20px; }
        h1 { color: #333; }
        .info { background: #d4edda; padding: 15px; border-radius: 5px; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="success">Success</div>
        <h1>Setup Complete!</h1>
        <div class="info">
            <strong>Configuration saved successfully!</strong><br><br>
            YAMUNA will now restart and connect to your WiFi network.<br>
            Mining will start automatically.
        </div>
        <p>This page will redirect in 10 seconds...</p>
    </div>
</body>
</html>
)";

void initWebConfig() {
    preferences.begin("yamuna", false);
    loadConfig();
}

void startConfigAP() {
    Serial.println("Starting Access Point mode...");
    
    // Stop any existing WiFi connection
    WiFi.disconnect(true);
    delay(1000);
    
    // Start Access Point
    WiFi.mode(WIFI_AP);
    bool ap_success = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (ap_success) {
        Serial.println("Access Point started successfully!");
        Serial.printf("SSID: %s\n", AP_SSID);
        Serial.printf("Password: %s\n", AP_PASSWORD);
        Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.println("Open http://192.168.4.1 in your browser");
    } else {
        Serial.println("Failed to start Access Point!");
        return;
    }

    // Setup web server routes
    server.on("/", HTTP_GET, []() { 
        Serial.println("Handling root request...");
        handleRoot(); 
    });
    server.on("/save", HTTP_POST, []() { 
        Serial.println("Handling save request...");
        handleSave(); 
    });
    server.on("/reset", HTTP_GET, []() { 
        Serial.println("Handling reset request...");
        handleReset(); 
    });

    server.begin();
    Serial.println("Web server started on port 80");
    Serial.println("Waiting for client connections...");
}

void handleRoot() {
    Serial.println("Serving configuration page...");
    
    String page = String(CONFIG_PAGE);
    Serial.printf("Page length: %d characters\n", page.length());

    // Replace placeholders with current values
    page.replace("%WIFI_SSID%", config.wifi_ssid);
    page.replace("%WIFI_PASSWORD%", ""); // Don't show password
    page.replace("%BTC_ADDRESS%", config.btc_address);
    page.replace("%POOL_URL%", config.pool_url);
    page.replace("%POOL_PORT%", String(config.pool_port));
    page.replace("%POOL_PASSWORD%", config.pool_password);

    // Handle thread selection
    page.replace("%THREADS_1%", config.threads == 1 ? "selected" : "");
    page.replace("%THREADS_2%", config.threads == 2 ? "selected" : "");

    server.send(200, "text/html", page);
    Serial.println("Configuration page sent successfully");
}

void handleSave() {
    Serial.println("=== Saving Configuration ===");
    
    // Get form data
    String wifi_ssid = server.arg("wifi_ssid");
    String wifi_password = server.arg("wifi_password");
    String btc_address = server.arg("btc_address");
    String pool_url = server.arg("pool_url");
    int pool_port = server.arg("pool_port").toInt();
    String pool_password = server.arg("pool_password");
    int threads = server.arg("threads").toInt();

    Serial.printf("WiFi SSID: '%s'\n", wifi_ssid.c_str());
    Serial.printf("WiFi Password length: %d\n", wifi_password.length());
    Serial.printf("BTC Address: '%s'\n", btc_address.c_str());
    Serial.printf("Pool URL: '%s'\n", pool_url.c_str());
    Serial.printf("Pool Port: %d\n", pool_port);
    Serial.printf("Pool Password: '%s'\n", pool_password.c_str());
    Serial.printf("Threads: %d\n", threads);

    // Validate inputs
    if (wifi_ssid.length() == 0 || wifi_password.length() == 0 || btc_address.length() == 0) {
        Serial.println("Validation failed - missing required fields");
        server.send(400, "text/plain", "Missing required fields");
        return;
    }

    // Check string length limits to prevent buffer overflow
    if (wifi_ssid.length() >= sizeof(config.wifi_ssid) ||
        wifi_password.length() >= sizeof(config.wifi_password) ||
        btc_address.length() >= sizeof(config.btc_address) ||
        pool_url.length() >= sizeof(config.pool_url) ||
        pool_password.length() >= sizeof(config.pool_password)) {
        Serial.println("Validation failed - parameters too long");
        server.send(400, "text/plain", "Configuration parameters too long");
        return;
    }

    // Save configuration
    strncpy(config.wifi_ssid, wifi_ssid.c_str(), sizeof(config.wifi_ssid) - 1);
    strncpy(config.wifi_password, wifi_password.c_str(), sizeof(config.wifi_password) - 1);
    strncpy(config.btc_address, btc_address.c_str(), sizeof(config.btc_address) - 1);
    strncpy(config.pool_url, pool_url.c_str(), sizeof(config.pool_url) - 1);
    config.pool_port = pool_port;
    strncpy(config.pool_password, pool_password.c_str(), sizeof(config.pool_password) - 1);
    config.threads = threads;
    config.configured = true;

    Serial.println("Calling saveConfig()...");
    saveConfig();
    Serial.println("saveConfig() completed");

    // Verify save was successful
    Serial.printf("Configured flag after save: %s\n", config.configured ? "TRUE" : "FALSE");
    Serial.printf("WiFi SSID in config: '%s'\n", config.wifi_ssid);

    // Send success page
    server.send(200, "text/html", SUCCESS_PAGE);
    Serial.println("Success page sent");

    // Wait longer before restart
    Serial.println("Waiting 5 seconds before restart...");
    delay(5000);
    Serial.println("Restarting ESP32...");
    ESP.restart();
}

void handleReset() {
    resetConfig();
    server.send(200, "text/plain", "Configuration reset. Restarting...");
    delay(2000);
    ESP.restart();
}

void saveConfig() {
    Serial.println("=== saveConfig() called ===");
    Serial.println("Preferences namespace: yamuna");
    
    // Test if preferences is working
    Serial.printf("Preferences is free: %d entries\n", preferences.freeEntries());
    
    bool success = true;
    
    // Save each value individually with error checking
    bool result;
    
    result = preferences.putString("wifi_ssid", config.wifi_ssid);
    success &= result;
    Serial.printf("Saved wifi_ssid: %s (%s)\n", config.wifi_ssid, result ? "OK" : "FAIL");
    
    result = preferences.putString("wifi_password", config.wifi_password);
    success &= result;
    Serial.printf("Saved wifi_password: length %d (%s)\n", strlen(config.wifi_password), result ? "OK" : "FAIL");
    
    result = preferences.putString("btc_address", config.btc_address);
    success &= result;
    Serial.printf("Saved btc_address: %s (%s)\n", config.btc_address, result ? "OK" : "FAIL");
    
    result = preferences.putString("pool_url", config.pool_url);
    success &= result;
    Serial.printf("Saved pool_url: %s (%s)\n", config.pool_url, result ? "OK" : "FAIL");
    
    result = preferences.putInt("pool_port", config.pool_port);
    success &= result;
    Serial.printf("Saved pool_port: %d (%s)\n", config.pool_port, result ? "OK" : "FAIL");
    
    result = preferences.putString("pool_password", config.pool_password);
    success &= result;
    Serial.printf("Saved pool_password: %s (%s)\n", config.pool_password, result ? "OK" : "FAIL");
    
    result = preferences.putInt("threads", config.threads);
    success &= result;
    Serial.printf("Saved threads: %d (%s)\n", config.threads, result ? "OK" : "FAIL");
    
    result = preferences.putBool("configured", config.configured);
    success &= result;
    Serial.printf("Saved configured: %s (%s)\n", config.configured ? "TRUE" : "FALSE", result ? "OK" : "FAIL");
    
    result = preferences.putInt("version", CONFIG_VERSION);
    success &= result;
    Serial.printf("Saved version: %d (%s)\n", CONFIG_VERSION, result ? "OK" : "FAIL");

    // Force write to flash with longer delay
    Serial.println("Forcing write to flash...");
    preferences.end();
    delay(500);  // Longer delay
    
    // Reopen preferences
    bool reopened = preferences.begin("yamuna", false);
    Serial.printf("Preferences reopened: %s\n", reopened ? "YES" : "NO");
    
    Serial.println("=== saveConfig() completed ===");
    Serial.printf("Overall save success: %s\n", success ? "YES" : "NO");
}

void loadConfig() {
    if (DEBUG) Serial.println("=== loadConfig() called ===");
    if (DEBUG) Serial.println("Preferences namespace: yamuna");
    
    // Load from preferences or use defaults
    String wifi_ssid = preferences.getString("wifi_ssid", DEFAULT_WIFI_SSID);
    String wifi_password = preferences.getString("wifi_password", DEFAULT_WIFI_PASSWORD);
    String btc_address = preferences.getString("btc_address", DEFAULT_ADDRESS);
    String pool_url = preferences.getString("pool_url", DEFAULT_POOL_URL);
    String pool_password = preferences.getString("pool_password", "");

    strncpy(config.wifi_ssid, wifi_ssid.c_str(), sizeof(config.wifi_ssid) - 1);
    strncpy(config.wifi_password, wifi_password.c_str(), sizeof(config.wifi_password) - 1);
    strncpy(config.btc_address, btc_address.c_str(), sizeof(config.btc_address) - 1);
    strncpy(config.pool_url, pool_url.c_str(), sizeof(config.pool_url) - 1);
    strncpy(config.pool_password, pool_password.c_str(), sizeof(config.pool_password) - 1);

    config.pool_port = preferences.getInt("pool_port", DEFAULT_POOL_PORT);
    config.threads = preferences.getInt("threads", THREADS);
    config.configured = preferences.getBool("configured", false);

    if (DEBUG) {
        Serial.printf("Loaded wifi_ssid: '%s'\n", config.wifi_ssid);
        Serial.printf("Loaded wifi_password length: %d\n", strlen(config.wifi_password));
        Serial.printf("Loaded btc_address: '%s'\n", config.btc_address);
        Serial.printf("Loaded pool_url: '%s'\n", config.pool_url);
        Serial.printf("Loaded pool_port: %d\n", config.pool_port);
        Serial.printf("Loaded pool_password: '%s'\n", config.pool_password);
        Serial.printf("Loaded threads: %d\n", config.threads);
        Serial.printf("Loaded configured: %s\n", config.configured ? "TRUE" : "FALSE");
        Serial.println("=== loadConfig() completed ===");
    }
}

bool isConfigured() {
    return config.configured && strlen(config.wifi_ssid) > 0 && strlen(config.wifi_password) > 0;
}

void resetConfig() {
    preferences.clear();

    // Reset to defaults
    strcpy(config.wifi_ssid, DEFAULT_WIFI_SSID);
    strcpy(config.wifi_password, DEFAULT_WIFI_PASSWORD);
    strcpy(config.btc_address, DEFAULT_ADDRESS);
    strcpy(config.pool_url, DEFAULT_POOL_URL);
    config.pool_port = DEFAULT_POOL_PORT;
    strcpy(config.pool_password, "");
    config.threads = THREADS;
    config.configured = false;

    Serial.println("Configuration reset to defaults");
}

void handleConfigWeb() {
    server.handleClient();
}