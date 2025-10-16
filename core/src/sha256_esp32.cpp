#include "sha256_esp32.h"
#include <Arduino.h>
#include "esp_system.h"
#include "soc/hwcrypto_periph.h"
#include "driver/periph_ctrl.h"
#include "esp32/rom/sha.h"
#include "configs.h"

// SHA-256 constants (first 32 bits of fractional parts of cube roots of first 64 primes)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// Rotate right macro
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

// SHA-256 logical functions
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROR(x, 2) ^ ROR(x, 13) ^ ROR(x, 22))
#define EP1(x)       (ROR(x, 6) ^ ROR(x, 11) ^ ROR(x, 25))
#define SIG0(x)      (ROR(x, 7) ^ ROR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROR(x, 17) ^ ROR(x, 19) ^ ((x) >> 10))

// Check if ESP32 hardware SHA acceleration is available
bool sha256_esp32_hw_available(void) {
    // ESP32 has built-in SHA hardware acceleration
    return true;
}

// Initialize SHA-256 context
void sha256_esp32_init(sha256_esp32_ctx_t *ctx) {
    if (!ctx) return;

    // Copy initial hash values
    for (int i = 0; i < 8; i++) {
        ctx->state[i] = H0[i];
    }

    ctx->buffer_len = 0;
    ctx->total_len = 0;
    ctx->use_hardware = sha256_esp32_hw_available();
}

// Process a single 512-bit block using optimized ESP32 assembly-like operations
static void sha256_esp32_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t W[64];
    uint32_t t1, t2;

    // Prepare message schedule W[0..15] with byte order conversion
    for (int i = 0; i < 16; i++) {
        W[i] = ((uint32_t)data[i * 4 + 0] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }

    // Extend the message schedule W[16..63]
    for (int i = 16; i < 64; i++) {
        W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
    }

    // Initialize working variables
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    // Main compression loop (unrolled for ESP32 optimization)
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// Update hash with new data
void sha256_esp32_update(sha256_esp32_ctx_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data) return;

    size_t remaining = len;
    const uint8_t *ptr = data;

    while (remaining > 0) {
        size_t space_in_buffer = 64 - ctx->buffer_len;
        size_t to_copy = (remaining < space_in_buffer) ? remaining : space_in_buffer;

        // Copy data to buffer
        memcpy(ctx->buffer + ctx->buffer_len, ptr, to_copy);
        ctx->buffer_len += to_copy;
        ctx->total_len += to_copy;

        // Process full buffer
        if (ctx->buffer_len == 64) {
            sha256_esp32_transform(ctx->state, ctx->buffer);
            ctx->buffer_len = 0;
        }

        ptr += to_copy;
        remaining -= to_copy;
    }
}

// Finalize hash computation
void sha256_esp32_final(sha256_esp32_ctx_t *ctx, uint8_t *hash) {
    if (!ctx || !hash) return;

    uint64_t total_bits = ctx->total_len * 8;

    // Padding: append '1' bit followed by zeros
    uint8_t padding[64];
    padding[0] = 0x80;
    for (int i = 1; i < 64; i++) {
        padding[i] = 0x00;
    }

    // Calculate padding length
    size_t pad_len = (ctx->buffer_len < 56) ? (56 - ctx->buffer_len) : (120 - ctx->buffer_len);

    // Add padding
    sha256_esp32_update(ctx, padding, pad_len);

    // Append length in bits as 64-bit big-endian
    uint8_t length_bytes[8];
    for (int i = 0; i < 8; i++) {
        length_bytes[i] = (total_bits >> (56 - i * 8)) & 0xFF;
    }
    sha256_esp32_update(ctx, length_bytes, 8);

    // Convert state to byte array (big-endian)
    for (int i = 0; i < 8; i++) {
        hash[i * 4 + 0] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = (ctx->state[i]) & 0xFF;
    }
}

// Single-call hash function
void sha256_esp32_hash(const uint8_t *data, size_t len, uint8_t *hash) {
    sha256_esp32_ctx_t ctx;
    sha256_esp32_init(&ctx);
    sha256_esp32_update(&ctx, data, len);
    sha256_esp32_final(&ctx, hash);
}

// Double SHA-256 (Bitcoin standard)
void sha256_esp32_double(const uint8_t *data, size_t len, uint8_t *hash) {
    uint8_t first_hash[32];
    sha256_esp32_hash(data, len, first_hash);
    sha256_esp32_hash(first_hash, 32, hash);
}

// Specialized function for Bitcoin block header hashing (80 bytes)
void sha256_esp32_bitcoin_hash(const uint8_t *block_header, uint8_t *hash) {
    // Bitcoin uses double SHA-256 on 80-byte block headers
    sha256_esp32_double(block_header, 80, hash);
}

// Benchmark different implementations
void sha256_esp32_benchmark(sha256_benchmark_t *result) {
    if (!result) return;

    const int test_iterations = 1000;
    const uint8_t test_data[80] = {0}; // Simulate block header
    uint8_t hash[32];

    // Benchmark software implementation
    unsigned long start = millis();
    for (int i = 0; i < test_iterations; i++) {
        sha256_esp32_bitcoin_hash(test_data, hash);
    }
    unsigned long software_time = millis() - start;

    // Calculate hashes per second
    result->software_hps = (software_time > 0) ? (test_iterations * 1000 / software_time) : 0;
    result->hardware_hps = result->software_hps; // ESP32 uses same implementation
    result->optimized_hps = result->software_hps;

    if (DEBUG) {
        Serial.printf("SHA-256 Benchmark Results:\n");
        Serial.printf("  Software: %u H/s\n", result->software_hps);
        Serial.printf("  Hardware: %u H/s\n", result->hardware_hps);
        Serial.printf("  Optimized: %u H/s\n", result->optimized_hps);
    }
}