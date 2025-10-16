// YAMUNA Configuration
// Default values - will be configurable via web interface

// SHA-256 Implementation
// 0 = Pure C, 1 = ESP32 Hardware Acceleration
#define USE_HW_SHA256 1

// Debug and Verbose Flags
#define DEBUG 0
#define VERBOSE 0  // 0 = clean output (cpuminer style), 1 = show detailed messages

// WiFi Configuration (defaults - configurable via web)
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""

// Mining Configuration
// #define THREADS 1 // Now auto-detected
#define MAX_NONCE 0xFFFFFFFF  // Use full 32-bit range for better share finding

// Share Difficulty Configuration (for pool visibility)
#define SHARE_DIFFICULTY_LEVEL 2  // 1=easy (2 zeros), 2=medium (3 zeros), 3=hard (4 zeros), 4=very hard (5 zeros), 5=extreme (6 zeros)
#define TARGET_SHARE_INTERVAL 120000  // Target interval between shares in milliseconds (2 minutes)
#define ADAPTIVE_DIFFICULTY 1  // Enable automatic difficulty adjustment based on hash rate

// Default Pool Configuration (configurable via web)
#define DEFAULT_POOL_URL "public-pool.io"
#define DEFAULT_POOL_PORT 21496
#define DEFAULT_ADDRESS "bc1qw2raw7urfuu2032uyyx9k5pryan5gu6gmz6exm.worker"
#define DEFAULT_POOL_PASSWORD "x"

// Web Configuration Portal
#define AP_SSID "YAMUNA"
#define AP_PASSWORD "yamuna123"
#define WEB_PORT 80

// Configuration Storage
#define CONFIG_VERSION 1