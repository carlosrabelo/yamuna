#ifndef SHA256_OPTIMIZED_H
#define SHA256_OPTIMIZED_H

#include <stdint.h>
#include <stddef.h>
#include "configs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[8];
    uint8_t buffer[64];
    uint32_t buffer_len;
    uint64_t total_len;
    bool use_hardware;
} sha256_opt_ctx_t;

typedef struct {
    uint32_t software_hps;
    uint32_t hardware_hps;
    uint32_t optimized_hps;
} sha256_benchmark_t;

void sha256_esp32_init(sha256_opt_ctx_t *ctx);
void sha256_esp32_update(sha256_opt_ctx_t *ctx, const uint8_t *data, size_t len);
void sha256_esp32_final(sha256_opt_ctx_t *ctx, uint8_t *hash);
void sha256_esp32_hash(const uint8_t *data, size_t len, uint8_t *hash);
void sha256_esp32_double(const uint8_t *data, size_t len, uint8_t *hash);
void sha256_esp32_bitcoin_hash(const uint8_t *block_header, uint8_t *hash);
void sha256_esp32_benchmark(sha256_benchmark_t *result);

void sha256_compute_midstate(const uint8_t *data, uint32_t len, uint8_t *midstate);
void sha256_bitcoin_hash_fast(const uint8_t *midstate, const uint8_t *midstate2,
                               const uint8_t *tail_data, size_t tail_len, uint8_t *hash_result);
bool sha256_check_fast_reject(const uint8_t *hash, uint8_t min_zeros);

#ifdef __cplusplus
}
#endif

#endif
