#ifndef MINING_UTILS_H
#define MINING_UTILS_H

#include <Arduino.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hash validation functions
bool checkHalfShare(unsigned char* hash);
bool checkShare(unsigned char* hash);
bool checkValid(unsigned char* hash, unsigned char* target);

// Adaptive difficulty functions
void initAdaptiveDifficulty();
void updateAdaptiveDifficulty();
uint8_t getCurrentDifficultyLevel();

// Utility functions
uint8_t hex(char ch);
int to_byte_array(const char *in, size_t in_size, uint8_t *out);

// Stratum mining functions
bool buildBlockHeader(uint8_t* header, uint32_t nonce, const String& extranonce2);
String calculateMerkleRoot(const String& coinb1, const String& coinb2, const String& extranonce1, const String& extranonce2, const String merkle_branch[], int merkle_count);
bool checkStratumTarget(const uint8_t* hash, uint32_t difficulty);

// Performance statistics
extern volatile unsigned long hashes;
extern volatile int halfshares;
extern volatile int shares;
extern volatile int valids;

// Adaptive difficulty variables
extern volatile unsigned long last_share_time;
extern volatile uint8_t current_difficulty_level;

// Constants
#define NONCE_BATCH_SIZE 256
#define JSON_BUFFER_SIZE 2048

#ifdef __cplusplus
}
#endif

#endif // MINING_UTILS_H