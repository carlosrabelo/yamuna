#ifndef SHA256_ESP32_H
#define SHA256_ESP32_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// SHA-256 context structure optimized for ESP32
typedef struct {
    uint32_t state[8];          // Current hash state
    uint8_t buffer[64];         // Input buffer
    uint32_t buffer_len;        // Length of data in buffer
    uint64_t total_len;         // Total length of processed data
    bool use_hardware;          // Flag to use ESP32 hardware acceleration
} sha256_esp32_ctx_t;

// Initialize SHA-256 context with ESP32 optimizations
void sha256_esp32_init(sha256_esp32_ctx_t *ctx);

// Update hash with new data (optimized for ESP32)
void sha256_esp32_update(sha256_esp32_ctx_t *ctx, const uint8_t *data, size_t len);

// Finalize hash computation and return result
void sha256_esp32_final(sha256_esp32_ctx_t *ctx, uint8_t *hash);

// Single-call hash function optimized for ESP32
void sha256_esp32_hash(const uint8_t *data, size_t len, uint8_t *hash);

// Double SHA-256 (Bitcoin standard) optimized for ESP32
void sha256_esp32_double(const uint8_t *data, size_t len, uint8_t *hash);

// Specialized function for Bitcoin block header hashing
void sha256_esp32_bitcoin_hash(const uint8_t *block_header, uint8_t *hash);

// Check if ESP32 hardware acceleration is available
bool sha256_esp32_hw_available(void);

// Benchmark different SHA-256 implementations
typedef struct {
    uint32_t software_hps;      // Hashes per second (software)
    uint32_t hardware_hps;      // Hashes per second (hardware)
    uint32_t optimized_hps;     // Hashes per second (optimized)
} sha256_benchmark_t;

void sha256_esp32_benchmark(sha256_benchmark_t *result);

#ifdef __cplusplus
}
#endif

#endif // SHA256_ESP32_H