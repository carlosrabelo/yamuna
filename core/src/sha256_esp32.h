#ifndef SHA256_ESP32_H
#define SHA256_ESP32_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "configs.h"

#if USE_HW_SHA256
#include <mbedtls/sha256.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#if USE_HW_SHA256
    mbedtls_sha256_context hw_ctx;
#endif
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t buffer_len;
    uint64_t total_len;
    bool use_hardware;
} sha256_esp32_ctx_t;

void sha256_esp32_init(sha256_esp32_ctx_t *ctx);
void sha256_esp32_update(sha256_esp32_ctx_t *ctx, const uint8_t *data, size_t len);
void sha256_esp32_final(sha256_esp32_ctx_t *ctx, uint8_t *hash);
void sha256_esp32_hash(const uint8_t *data, size_t len, uint8_t *hash);
void sha256_esp32_double(const uint8_t *data, size_t len, uint8_t *hash);
void sha256_esp32_bitcoin_hash(const uint8_t *block_header, uint8_t *hash);

typedef struct {
    uint32_t software_hps;
    uint32_t hardware_hps;
    uint32_t optimized_hps;
} sha256_benchmark_t;

void sha256_esp32_benchmark(sha256_benchmark_t *result);

#ifdef __cplusplus
}
#endif

#endif // SHA256_ESP32_H