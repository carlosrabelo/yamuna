#include "mining_utils.h"
#include "configs.h"
#include "pool_connection.h"
#include "sha256_esp32.h"

// Global statistics variables
volatile unsigned long hashes = 0;
volatile int halfshares = 0;
volatile int shares = 0;
volatile int valids = 0;

// Adaptive difficulty variables
volatile unsigned long last_share_time = 0;
volatile uint8_t current_difficulty_level = SHARE_DIFFICULTY_LEVEL;

bool checkHalfShare(unsigned char* hash) {
  bool valid = true;
  for(uint8_t i=31; i>31-2; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  if (valid && VERBOSE) {
    Serial.print("Half-share found! Hash: ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x", hash[i]);
    Serial.println();
  }
  return valid;
}

bool checkShare(unsigned char* hash) {
  // Share detection: configurable difficulty for pool visibility
  // Level 1=2 zeros, 2=3 zeros, 3=4 zeros, 4=5 zeros, 5=6 zeros
  uint8_t required_zeros = current_difficulty_level + 1;

  bool valid = true;
  for(uint8_t i=31; i>31-required_zeros; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  if (valid) {
    // Update last share time for adaptive difficulty
    last_share_time = millis();

    if (VERBOSE) {
      Serial.printf("SHARE FOUND! (difficulty %d) Hash: ", current_difficulty_level);
      for (size_t i = 0; i < 32; i++)
          Serial.printf("%02x", hash[i]);
      Serial.println();
    } else {
      Serial.println("yay!!! Share found!");
    }
  }
  return valid;
}

bool checkValid(unsigned char* hash, unsigned char* target) {
  bool valid = true;
  for(int8_t i=31; i>=0; i--) {
    if(hash[i] > target[i]) {
      valid = false;
      break;
    } else if (hash[i] < target[i]) {
      valid = true;
      break;
    }
  }
  if (valid && DEBUG) {
    Serial.print("\tvalid : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  return valid;
}

uint8_t hex(char ch) {
    uint8_t r = (ch > 57) ? (ch - 55) : (ch - 48);
    return r & 0x0F;
}

int to_byte_array(const char *in, size_t in_size, uint8_t *out) {
    int count = 0;
    if (in_size % 2) {
        while (*in && out) {
            *out = hex(*in++);
            if (!*in)
                return count;
            *out = (*out << 4) | hex(*in++);
            *out++;
            count++;
        }
        return count;
    } else {
        while (*in && out) {
            *out++ = (hex(*in++) << 4) | hex(*in++);
            count++;
        }
        return count;
    }
}

// Adaptive difficulty implementation
void initAdaptiveDifficulty() {
    last_share_time = millis();
    current_difficulty_level = SHARE_DIFFICULTY_LEVEL;

    if (VERBOSE) {
        Serial.printf("Adaptive difficulty initialized at level %d\n", current_difficulty_level);
    }
}

void updateAdaptiveDifficulty() {
    if (!ADAPTIVE_DIFFICULTY) return;

    unsigned long now = millis();
    unsigned long time_since_last_share = now - last_share_time;

    // Only adjust every 60 seconds to avoid frequent changes
    static unsigned long last_adjustment = 0;
    if (now - last_adjustment < 60000) return;

    last_adjustment = now;

    // Minimum difficulty level to prevent going to zero
    uint8_t min_difficulty = max(SHARE_DIFFICULTY_LEVEL, 3);
    uint8_t max_difficulty = 5;

    // If no shares found for too long, decrease difficulty (but not below minimum)
    if (time_since_last_share > TARGET_SHARE_INTERVAL * 2 && current_difficulty_level > min_difficulty) {
        current_difficulty_level--;
        last_share_time = now; // Reset timer to give it a chance at the new difficulty
        if (VERBOSE) {
            Serial.printf("Decreasing difficulty to level %d (no shares for %lu ms)\n",
                         current_difficulty_level, time_since_last_share);
        }
    }
    // If shares coming too fast, increase difficulty
    else if (time_since_last_share < TARGET_SHARE_INTERVAL / 3 && current_difficulty_level < max_difficulty) {
        current_difficulty_level++;
        last_share_time = now; // Reset timer to avoid rapid successive increases
        if (VERBOSE) {
            Serial.printf("Increasing difficulty to level %d (shares too frequent)\n",
                         current_difficulty_level);
        }
    }

    // Reset share time if it's been too long (prevents overflow)
    if (time_since_last_share > TARGET_SHARE_INTERVAL * 10) {
        last_share_time = now;
        if (VERBOSE) {
            Serial.println("Resetting share time timer (too long without shares)");
        }
    }
}

uint8_t getCurrentDifficultyLevel() {
    return current_difficulty_level;
}

// Stratum mining functions implementation
bool buildBlockHeader(uint8_t* header, uint32_t nonce, const String& extranonce2) {
    StratumState* state = PoolConnection::getStratumState();
    if (!state || state->current_job.job_id.isEmpty()) {
        return false;
    }

    // Calculate merkle root
    String merkleRoot = calculateMerkleRoot(
        state->current_job.coinb1,
        state->current_job.coinb2,
        state->extranonce1,
        extranonce2,
        state->current_job.merkle_branch,
        state->current_job.merkle_count
    );

    // Build block header (80 bytes)
    memset(header, 0, 80);

    // Version (4 bytes) - little endian
    uint32_t version = strtoul(state->current_job.version.c_str(), NULL, 16);
    *(uint32_t*)(header + 0) = version;

    // Previous hash (32 bytes) - reverse byte order for little endian
    if (state->current_job.prevhash.length() >= 64) {
        for (int i = 0; i < 32; i++) {
            String byte_str = state->current_job.prevhash.substring((31-i)*2, (31-i)*2 + 2);
            header[4 + i] = strtoul(byte_str.c_str(), NULL, 16);
        }
    }

    // Merkle root (32 bytes) - reverse byte order for little endian
    if (merkleRoot.length() >= 64) {
        for (int i = 0; i < 32; i++) {
            String byte_str = merkleRoot.substring((31-i)*2, (31-i)*2 + 2);
            header[36 + i] = strtoul(byte_str.c_str(), NULL, 16);
        }
    }

    // Timestamp (4 bytes) - little endian
    uint32_t ntime = strtoul(state->current_job.ntime.c_str(), NULL, 16);
    *(uint32_t*)(header + 68) = ntime;

    // Bits/difficulty (4 bytes) - little endian
    uint32_t bits = strtoul(state->current_job.nbits.c_str(), NULL, 16);
    *(uint32_t*)(header + 72) = bits;

    // Nonce (4 bytes) - little endian
    *(uint32_t*)(header + 76) = nonce;

    return true;
}

String calculateMerkleRoot(const String& coinb1, const String& coinb2, const String& extranonce1, const String& extranonce2, const String merkle_branch[], int merkle_count) {
    // Build coinbase transaction
    String coinbase = coinb1 + extranonce1 + extranonce2 + coinb2;

    // Convert to bytes and hash twice (Bitcoin double SHA-256)
    uint8_t coinbase_bytes[coinbase.length()/2];
    to_byte_array(coinbase.c_str(), coinbase.length(), coinbase_bytes);

    uint8_t hash1[32];
    uint8_t hash2[32];
    sha256_esp32_hash(coinbase_bytes, coinbase.length()/2, hash1);
    sha256_esp32_hash(hash1, 32, hash2);

    // Apply merkle branch
    for (int i = 0; i < merkle_count; i++) {
        if (merkle_branch[i].length() >= 64) {
            uint8_t branch_bytes[32];
            for (int j = 0; j < 32; j++) {
                String byte_str = merkle_branch[i].substring(j*2, j*2 + 2);
                branch_bytes[j] = strtoul(byte_str.c_str(), NULL, 16);
            }

            // Concatenate and hash
            uint8_t combined[64];
            memcpy(combined, hash2, 32);
            memcpy(combined + 32, branch_bytes, 32);

            sha256_esp32_hash(combined, 64, hash1);
            sha256_esp32_hash(hash1, 32, hash2);
        }
    }

    // Convert result to hex string
    String result = "";
    for (int i = 0; i < 32; i++) {
        char hex_byte[3];
        sprintf(hex_byte, "%02x", hash2[i]);
        result += hex_byte;
    }

    return result;
}

bool checkStratumTarget(const uint8_t* hash, uint32_t difficulty) {
    // Simple difficulty check - count leading zeros
    uint8_t required_zeros = 0;
    if (difficulty == 1) required_zeros = 8;  // ~32 bits
    else if (difficulty <= 4) required_zeros = 12;  // ~48 bits
    else if (difficulty <= 16) required_zeros = 16;  // ~64 bits
    else if (difficulty <= 64) required_zeros = 20;  // ~80 bits
    else required_zeros = 24;  // ~96 bits

    // Check from the end of hash (little endian)
    for (int i = 31; i > 31 - (required_zeros / 8); i--) {
        if (hash[i] != 0) {
            return false;
        }
    }

    // Check remaining bits if needed
    int remaining_bits = required_zeros % 8;
    if (remaining_bits > 0) {
        int byte_idx = 31 - (required_zeros / 8);
        if (byte_idx >= 0 && (hash[byte_idx] >> (8 - remaining_bits)) != 0) {
            return false;
        }
    }

    return true;
}