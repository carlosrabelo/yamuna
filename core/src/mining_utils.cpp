#include "mining_utils.h"
#include "configs.h"

// Global statistics variables
volatile long hashes = 0;
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

    // Only adjust every 30 seconds to avoid frequent changes
    static unsigned long last_adjustment = 0;
    if (now - last_adjustment < 30000) return;

    last_adjustment = now;

    // If no shares found for too long, decrease difficulty
    if (time_since_last_share > TARGET_SHARE_INTERVAL * 2 && current_difficulty_level > 1) {
        current_difficulty_level--;
        if (VERBOSE) {
            Serial.printf("Decreasing difficulty to level %d (no shares for %lu ms)\n",
                         current_difficulty_level, time_since_last_share);
        }
    }
    // If shares coming too fast, increase difficulty
    else if (time_since_last_share < TARGET_SHARE_INTERVAL / 3 && current_difficulty_level < 5) {
        current_difficulty_level++;
        if (VERBOSE) {
            Serial.printf("Increasing difficulty to level %d (shares too frequent)\n",
                         current_difficulty_level);
        }
    }
}

uint8_t getCurrentDifficultyLevel() {
    return current_difficulty_level;
}