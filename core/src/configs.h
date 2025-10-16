// YAMUNA Configuration
// Default values - will be configurable via web interface

// Debug and Verbose Flags
#define DEBUG 0
#define VERBOSE 0  // 0 = clean output, 1 = show detailed messages

// WiFi Configuration (defaults - configurable via web)
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""

// Mining Configuration
#define THREADS 1
#define MAX_NONCE 0xFFFFFFFF  // Use full 32-bit range for better share finding

// Default Pool Configuration (configurable via web)
#define DEFAULT_POOL_URL "public-pool.io"
#define DEFAULT_POOL_PORT 21496
#define DEFAULT_ADDRESS "bc1qw2raw7urfuu2032uyyx9k5pryan5gu6gmz6exm.worker"

// Web Configuration Portal
#define AP_SSID "YAMUNA"
#define AP_PASSWORD "yamuna123"
#define WEB_PORT 80

// Configuration Storage
#define CONFIG_VERSION 1