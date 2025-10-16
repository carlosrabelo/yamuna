#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "configs.h"
#include "webconfig.h"
#include "esp_task_wdt.h"


// Performance optimizations
#define NONCE_BATCH_SIZE 256
#define JSON_BUFFER_SIZE 2048
volatile long hashes = 0;
volatile int halfshares = 0;
volatile int shares = 0;
volatile int valids = 0;

bool checkHalfShare(unsigned char* hash) {
  bool valid = true;
  for(uint8_t i=31; i>31-2; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  if (valid) {
    Serial.print("Half-share found! Hash: ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x", hash[i]);
    Serial.println();
  }
  return valid;
}

bool checkShare(unsigned char* hash) {
  // Share detection: look for hashes with at least 6 leading zero bytes
  // This is easier than pool difficulty but harder than half-shares
  bool valid = true;
  for(uint8_t i=31; i>31-6; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  if (valid) {
    Serial.print("SHARE FOUND! Hash: ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x", hash[i]);
    Serial.println();
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

#define CONNECTION_TIMEOUT 300000 // 5 minutes

// Worker-specific connection data
struct WorkerConnection {
  WiFiClient* client;
  unsigned long last_connection_time;
  char worker_name[16];
};

bool ensureConnection(WorkerConnection* conn) {
  if (conn->client == nullptr) {
    conn->client = new WiFiClient();
  }

  if (!conn->client->connected() || (millis() - conn->last_connection_time > CONNECTION_TIMEOUT)) {
    conn->client->stop();
    delay(1000); // Give time for cleanup

    if (VERBOSE) {
      Serial.printf("%s: Attempting connection to %s:%d\n", conn->worker_name, config.pool_url, config.pool_port);
    }

    // Set longer connection timeout
    conn->client->setTimeout(30000); // 30 second timeout

    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
      if (DEBUG) Serial.printf("%s: WiFi not connected, cannot connect to pool\n", conn->worker_name);
      return false;
    }

    // Try to resolve hostname first
    IPAddress serverIP;
    if (WiFi.hostByName(config.pool_url, serverIP)) {
      if (VERBOSE) {
        Serial.printf("%s: Resolved %s to %s\n", conn->worker_name, config.pool_url, serverIP.toString().c_str());
      }

      if (conn->client->connect(serverIP, config.pool_port)) {
        conn->last_connection_time = millis();
        if (VERBOSE) Serial.printf("%s: Pool connection established\n", conn->worker_name);
        return true;
      } else {
        if (DEBUG) Serial.printf("%s: Failed to connect to %s (%s:%d)\n",
                                conn->worker_name, config.pool_url, serverIP.toString().c_str(), config.pool_port);
      }
    } else {
      if (DEBUG) Serial.printf("%s: Failed to resolve hostname: %s\n", conn->worker_name, config.pool_url);

      // Fallback to direct connection attempt
      if (conn->client->connect(config.pool_url, config.pool_port)) {
        conn->last_connection_time = millis();
        if (DEBUG) Serial.printf("%s: Pool connection established (direct)\n", conn->worker_name);
        return true;
      } else {
        if (DEBUG) Serial.printf("%s: Direct connection failed to %s:%d\n", conn->worker_name, config.pool_url, config.pool_port);
      }
    }

    return false;
  }
  return true;
}

void runWorker(void *name) {
  if (DEBUG) Serial.printf("\nRunning %s on core %d\n", (char *)name, xPortGetCoreID());

  // Create worker-specific connection
  WorkerConnection conn;
  conn.client = nullptr;
  conn.last_connection_time = 0;
  strlcpy(conn.worker_name, (char *)name, sizeof(conn.worker_name));

  // Ensure cleanup on exit
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);

  // Stagger worker startup to avoid simultaneous connections
  int worker_id = ((char *)name)[7] - '0'; // Extract number from "Worker[N]"
  delay(worker_id * 2000); // 0s, 2s, 4s, etc. delay between workers

  while(true) {
    esp_task_wdt_reset(); // Reset watchdog before connection attempt
    if (!ensureConnection(&conn)) {
      Serial.printf("%s: Connection failed, retrying in 10 seconds...\n", (char *)name);
      // Progressive backoff delay: start with 10 seconds, increase if repeated failures
      static int retry_count = 0;
      retry_count++;
      int delay_seconds = min(10 + (retry_count * 5), 60); // Max 60 seconds

      // Cleanup any existing connection before retry
      if (conn.client) {
        conn.client->stop();
        delete conn.client;
        conn.client = nullptr;
      }

      for(int i = 0; i < (delay_seconds * 10); i++) { // Reset watchdog during longer delay
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_task_wdt_reset();
      }
      continue;
    }

    // Reset retry count on successful connection
    static int retry_count = 0;
    retry_count = 0;

    WiFiClient& client = *conn.client;
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    char payload[256];
    char line[2048];

    // Pre-allocate buffers to avoid repeated allocations
    char sub_details[64];
    char extranonce1[64];
    char method[32];
    char job_id[64];
    char prevhash[128];
    char coinb1[512];  // Increased for longer coinbase1
    char coinb2[512];  // Increased for longer coinbase2
    char version[16];
    char nbits[16];
    char ntime[16];
    JsonArray merkle_branch;
    bool clean_jobs;
    
    snprintf(payload, sizeof(payload), "{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": []}\n");
    if (DEBUG) { Serial.print("Sending  : "); Serial.println(payload); }
    client.print(payload);
    
    // Add timeout for reading response
    unsigned long timeout = millis() + 10000; // 10 second timeout
    while(!client.available() && millis() < timeout) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      esp_task_wdt_reset();
    }
    
    if(!client.available()) {
      Serial.println("Timeout waiting for subscribe response");
      continue;
    }
    
    client.readStringUntil('\n').toCharArray(line, sizeof(line));
    if (VERBOSE) { Serial.print("Subscribe response: "); Serial.println(line); }
    if (DEBUG) { Serial.print("Receiving: "); Serial.println(line); }
    DeserializationError error = deserializeJson(doc, line);
    if(error) {
      Serial.printf("JSON deserialize error: %s\n", error.c_str());
      continue;
    }
    
    if(!doc.containsKey("result")) {
      Serial.println("Invalid subscribe response - missing result");
      continue;
    }

    // Safely access nested JSON arrays with bounds checking
    JsonArray result = doc["result"];
    if(result.size() < 3) {
      Serial.println("Invalid subscribe result - insufficient elements");
      continue;
    }

    JsonArray sub_array = result[0];
    if(sub_array.size() < 1 || !sub_array[0].is<JsonArray>()) {
      Serial.println("Invalid subscribe result - malformed subscription details");
      continue;
    }

    JsonArray sub_details_array = sub_array[0];
    if(sub_details_array.size() < 2) {
      Serial.println("Invalid subscribe result - malformed subscription details array");
      continue;
    }

    strlcpy(sub_details, sub_details_array[1], sizeof(sub_details));
    strlcpy(extranonce1, result[1], sizeof(extranonce1));
    int extranonce2_size = result[2];
    
    if (DEBUG) {
      Serial.print("sub_details: "); Serial.println(sub_details);
      Serial.print("extranonce1: "); Serial.println(extranonce1);
      Serial.print("extranonce2_size: "); Serial.println(extranonce2_size);
    }
    
    // Read additional lines after subscribe (like in original code)
    line[0] = '\0';
    if(client.available()) {
      client.readStringUntil('\n').toCharArray(line, sizeof(line));
      if (VERBOSE) { Serial.print("Extra after subscribe: "); Serial.println(line); }
    }

    // Now send authorization
    snprintf(payload, sizeof(payload), "{\"params\": [\"%s\", \"%s\"], \"id\": 2, \"method\": \"mining.authorize\"}\n", config.btc_address, config.pool_password);
    if (DEBUG) { Serial.print("Sending  : "); Serial.println(payload); }
    client.print(payload);
    
    esp_task_wdt_reset();
    timeout = millis() + 10000; // 10 second timeout for authorize
    while(!client.available() && millis() < timeout) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      esp_task_wdt_reset();
    }
    
    if(!client.available()) {
      Serial.println("Timeout waiting for authorize response");
      continue;
    }
    
    // Read authorization response, but handle out-of-order messages
    bool auth_received = false;
    timeout = millis() + 10000; // 10 second timeout for authorize

    while(millis() < timeout && !auth_received) {
      if(client.available()) {
        String auth_response = client.readStringUntil('\n');
        if (VERBOSE) { Serial.print("Auth response: "); Serial.println(auth_response); }

        error = deserializeJson(doc, auth_response);
        if(error) {
          Serial.printf("JSON deserialize error in authorize: %s\n", error.c_str());
          continue;
        }

        // Check if this is an authorization response (has id: 2)
        if(doc.containsKey("id") && doc["id"] == 2) {
          if(doc.containsKey("result") && doc["result"]) {
            Serial.println("Authorization successful");
            auth_received = true;
          } else {
            Serial.println("Authorization failed");
            break;
          }
        } else if(doc.containsKey("method")) {
          // This is a notification, store for later processing
          if (VERBOSE) Serial.printf("Received notification: %s\n", doc["method"].as<const char*>());
        }
      } else {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        esp_task_wdt_reset();
      }
    }

    if(!auth_received) {
      Serial.println("Authorization timeout or failed");
      continue;
    }
    
    Serial.println("Authorization successful - reading job");
    
    // Wait for mining.notify message with job data
    esp_task_wdt_reset();
    timeout = millis() + 30000; // 30 second timeout for first job
    bool job_received = false;
    
    while(millis() < timeout && !job_received) {
      if(client.available()) {
        String message = client.readStringUntil('\n');
        if (VERBOSE) { Serial.print("Pool message: "); Serial.println(message); }
        
        // Check for mining.notify
        if(message.indexOf("\"method\":\"mining.notify\"") != -1) {
          error = deserializeJson(doc, message);
          if(!error && doc.containsKey("method") && doc.containsKey("params") &&
             strcmp(doc["method"], "mining.notify") == 0 &&
             doc["params"].is<JsonArray>()) {

            JsonArray params = doc["params"];
            if(params.size() >= 9) {
              // Validate string lengths before copying
              const char* job_id_str = params[0];
              const char* prevhash_str = params[1];
              const char* coinb1_str = params[2];
              const char* coinb2_str = params[3];

              // Null pointer checks
              if (!job_id_str || !prevhash_str || !coinb1_str || !coinb2_str) {
                Serial.println("Null pointer in job parameters, skipping");
                continue;
              }

              size_t job_id_len = strnlen(job_id_str, sizeof(job_id));
              size_t prevhash_len = strnlen(prevhash_str, sizeof(prevhash));
              size_t coinb1_len = strnlen(coinb1_str, sizeof(coinb1));
              size_t coinb2_len = strnlen(coinb2_str, sizeof(coinb2));

              if (DEBUG) {
                Serial.printf("Job param lengths: job_id=%d, prevhash=%d, coinb1=%d, coinb2=%d\n",
                             job_id_len, prevhash_len, coinb1_len, coinb2_len);
                Serial.printf("Buffer sizes: job_id=%d, prevhash=%d, coinb1=%d, coinb2=%d\n",
                             sizeof(job_id), sizeof(prevhash), sizeof(coinb1), sizeof(coinb2));
              }

              if(job_id_len >= sizeof(job_id) ||
                 prevhash_len >= sizeof(prevhash) ||
                 coinb1_len >= sizeof(coinb1) ||
                 coinb2_len >= sizeof(coinb2)) {
                Serial.printf("Job parameters too long: job_id=%d/%d, prevhash=%d/%d, coinb1=%d/%d, coinb2=%d/%d\n",
                             job_id_len, sizeof(job_id), prevhash_len, sizeof(prevhash),
                             coinb1_len, sizeof(coinb1), coinb2_len, sizeof(coinb2));
                continue;
              }

              strlcpy(job_id, job_id_str, sizeof(job_id));
              strlcpy(prevhash, prevhash_str, sizeof(prevhash));
              strlcpy(coinb1, coinb1_str, sizeof(coinb1));
              strlcpy(coinb2, coinb2_str, sizeof(coinb2));

              // Safely access merkle branch array
              if(params[4].is<JsonArray>()) {
                merkle_branch = params[4];
              } else {
                Serial.println("Invalid merkle branch in job");
                continue;
              }

              const char* version_str = params[5];
              const char* nbits_str = params[6];
              const char* ntime_str = params[7];

              // Null pointer checks for timing parameters
              if (!version_str || !nbits_str || !ntime_str) {
                Serial.println("Null pointer in timing parameters, skipping");
                continue;
              }

              if(strnlen(version_str, sizeof(version)) >= sizeof(version) ||
                 strnlen(nbits_str, sizeof(nbits)) >= sizeof(nbits) ||
                 strnlen(ntime_str, sizeof(ntime)) >= sizeof(ntime)) {
                Serial.println("Job timing parameters too long, skipping");
                continue;
              }

              strlcpy(version, version_str, sizeof(version));
              strlcpy(nbits, nbits_str, sizeof(nbits));
              strlcpy(ntime, ntime_str, sizeof(ntime));
              clean_jobs = params[8];

              Serial.println("Job received - starting mining");
              job_received = true;
              break;
            } else {
              Serial.printf("Invalid job params - only %d elements\n", params.size());
            }
          }
        } else if(message.indexOf("\"method\":\"mining.set_difficulty\"") != -1) {
          error = deserializeJson(doc, message);
          if(!error && doc.containsKey("params") && doc["params"].is<JsonArray>() && doc["params"].size() > 0) {
            double pool_difficulty = doc["params"][0];
            Serial.printf("%s: Pool difficulty set to: %.0f\n", conn.worker_name, pool_difficulty);
          } else {
            if (VERBOSE) Serial.println("Set difficulty message received");
          }
        } else if(message.indexOf("\"id\":") != -1 && message.indexOf("\"result\"") != -1) {
          // This is a response, not a notification - ignore
          if (VERBOSE) Serial.println("Response message received (not job)");
        }
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
      esp_task_wdt_reset();
    }
    
    if(!job_received) {
      Serial.println("Timeout waiting for job");
      continue;
    }
    
    if (VERBOSE) {
      Serial.printf("%s: Job details - ID: %s, Version: %s, Bits: %s, Time: %s\n",
                   conn.worker_name, job_id, version, nbits, ntime);
    }
    
    // Clear any additional messages before starting mining
    esp_task_wdt_reset();
    while(client.available()) {
      String extra_msg = client.readStringUntil('\n');
      if (VERBOSE) Serial.printf("Extra message: %s\n", extra_msg.c_str());
      delay(10);
    }

    // Optimized target calculation using direct byte operations
    char target[65];
    memset(target, '0', 64);
    target[64] = '\0';

    // Parse nbits more efficiently
    uint8_t exp = ((nbits[0] >= 'a') ? (nbits[0] - 'a' + 10) : (nbits[0] - '0')) << 4;
    exp |= (nbits[1] >= 'a') ? (nbits[1] - 'a' + 10) : (nbits[1] - '0');
    int zeros = exp - 3;

    // Direct copy without strlen
    int nbits_len = 6; // nbits is always 8 chars, we want last 6
    int target_pos = 64 - nbits_len - (zeros * 2);
    if (target_pos >= 0) {
      memcpy(target + target_pos, nbits + 2, nbits_len);
    }
    if (DEBUG) {
      Serial.printf("%s: Target: %s\n", conn.worker_name, target);
    }

    uint8_t bytearray_target[32];
    size_t size_target = to_byte_array(target, 32, bytearray_target);

    if (DEBUG) {
      Serial.printf("%s: Target bytes: ", conn.worker_name);
      for (size_t i = 0; i < 8; i++) Serial.printf("%02x", bytearray_target[i]);
      Serial.println("...");
    }
    uint8_t buf;
    for (size_t j = 0; j < 16; j++) {
        buf = bytearray_target[j];
        bytearray_target[j] = bytearray_target[size_target - 1 - j];
        bytearray_target[size_target - 1 - j] = buf;
    }


    uint32_t extranonce2_a_bin = esp_random();
    uint32_t extranonce2_b_bin = esp_random();
    char extranonce2[16];
    snprintf(extranonce2, sizeof(extranonce2), "%08x%08x", extranonce2_a_bin, extranonce2_b_bin);
    if (DEBUG) { Serial.print("extranonce2: "); Serial.println(extranonce2); }

    // Optimized coinbase construction
    char coinbase[1024];  // Increased to accommodate coinb1+extranonces+coinb2
    size_t coinb1_len = strlen(coinb1);
    size_t extranonce1_len = strlen(extranonce1);
    size_t extranonce2_len = strlen(extranonce2);
    size_t coinb2_len = strlen(coinb2);

    // Use memcpy for better performance
    char* pos = coinbase;
    memcpy(pos, coinb1, coinb1_len); pos += coinb1_len;
    memcpy(pos, extranonce1, extranonce1_len); pos += extranonce1_len;
    memcpy(pos, extranonce2, extranonce2_len); pos += extranonce2_len;
    memcpy(pos, coinb2, coinb2_len); pos += coinb2_len;
    *pos = '\0';
    if (DEBUG) { Serial.print("coinbase: "); Serial.println(coinbase); }
    
    size_t str_len = strlen(coinbase)/2;
    uint8_t bytearray[str_len];
    size_t res = to_byte_array(coinbase, strlen(coinbase), bytearray);
    if (DEBUG) {
      Serial.print("coinbase bytes - size ");
      Serial.println(res);
      for (size_t i = 0; i < res; i++)
          Serial.printf("%02x ", bytearray[i]);
      Serial.println("---");
    }

    byte interResult[32];
    byte shaResult[32];
  
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, bytearray, str_len);
    mbedtls_md_finish(&ctx, interResult);

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, interResult, 32);
    mbedtls_md_finish(&ctx, shaResult);

    if (DEBUG) {
      Serial.print("coinbase double sha: ");
      for (size_t i = 0; i < 32; i++)
          Serial.printf("%02x ", shaResult[i]);
      Serial.println("");
    }

    byte merkle_result[32];
    for (size_t i = 0; i < 32; i++)
      merkle_result[i] = shaResult[i];
    
    byte merkle_concatenated[64];
    for (size_t k=0; k<merkle_branch.size(); k++) {
        const char* merkle_element = (const char*) merkle_branch[k];
        uint8_t bytearray_merkle[32];
        size_t res_merkle = to_byte_array(merkle_element, 64, bytearray_merkle);

        if (DEBUG) {
          Serial.print("\tmerkle element    "); Serial.print(k); Serial.print(": "); Serial.println(merkle_element);
        }
        
        for (size_t i = 0; i < 32; i++) {
          merkle_concatenated[i] = merkle_result[i];
          merkle_concatenated[32 + i] = bytearray_merkle[i];
        }
        
        if (DEBUG) {
          Serial.print("\tmerkle concatenated: ");
          for (size_t i = 0; i < 64; i++)
              Serial.printf("%02x", merkle_concatenated[i]);
          Serial.println("");
        }
            
        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, merkle_concatenated, 64);
        mbedtls_md_finish(&ctx, interResult);

        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, interResult, 32);
        mbedtls_md_finish(&ctx, merkle_result);

        if (DEBUG) {
          Serial.print("\tmerkle sha         : ");
          for (size_t i = 0; i < 32; i++)
              Serial.printf("%02x", merkle_result[i]);
          Serial.println("");
        }
    }
    
    char merkle_root[64];
    for (int i = 0; i < 32; i++) {
      snprintf(merkle_root + (i*2), 3, "%02x", merkle_result[i]);
    }

    // Optimized blockheader construction
    char blockheader[161]; // 160 + null terminator
    char* bh_pos = blockheader;

    // Use memcpy instead of snprintf for better performance
    memcpy(bh_pos, version, 8); bh_pos += 8;
    memcpy(bh_pos, prevhash, 64); bh_pos += 64;
    memcpy(bh_pos, merkle_root, 64); bh_pos += 64;
    memcpy(bh_pos, nbits, 8); bh_pos += 8;
    memcpy(bh_pos, ntime, 8); bh_pos += 8;
    memcpy(bh_pos, "00000000", 8); bh_pos += 8;
    *bh_pos = '\0';
    
    if (DEBUG) Serial.println("blockheader bytes");
    str_len = strlen(blockheader)/2;
    uint8_t bytearray_blockheader[str_len];
    res = to_byte_array(blockheader, strlen(blockheader), bytearray_blockheader);
    
    uint8_t buff;
    size_t bsize, boffset;
    
    boffset = 0; bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }
    
    boffset = 36; bsize = 32;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }
    
    boffset = 72; bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }

    uint32_t nonce = 0;
    // Remove unused timing variable

    // Optimized mining loop with batch processing
    if (VERBOSE) Serial.printf("%s: Starting mining loop with job %s\n", conn.worker_name, job_id);
    while(true) {
      // Process nonces in batches for better performance
      uint32_t batch_end = min(nonce + NONCE_BATCH_SIZE, (uint32_t)MAX_NONCE);
      for(; nonce < batch_end; nonce++) {
        // Inline nonce update for speed
        *(uint32_t*)(bytearray_blockheader + 76) = nonce;

        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, bytearray_blockheader, 80);
        mbedtls_md_finish(&ctx, interResult);

        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, interResult, 32);
        mbedtls_md_finish(&ctx, shaResult);

        // Check results (optimized order: valid first as it's most important)
        if(checkValid(shaResult, bytearray_target)) {
          Serial.printf("%s: Valid found! nonce: %u | 0x%x\n",
                       (char *)name, xPortGetCoreID(), nonce, nonce);
          valids++;
          snprintf(payload, sizeof(payload),
                   "{\"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%08x\"], \"id\": 1, \"method\": \"mining.submit\"}\n",
                   config.btc_address, job_id, extranonce2, ntime, nonce);
          if (DEBUG) { Serial.print("Sending  : "); Serial.println(payload); }
          client.print(payload);
          client.readStringUntil('\n').toCharArray(line, sizeof(line));
          if (DEBUG) { Serial.print("Receiving: "); Serial.println(line); }
          goto exit_mining;
        }

        // For share detection, use a simpler threshold
        // Pool shares are typically much easier than the full target
        if(checkShare(shaResult)) {
          Serial.printf("%s: Share found! nonce: %u\n", (char *)name, nonce);
          shares++;

          // Submit share to pool
          snprintf(payload, sizeof(payload),
                   "{\"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%08x\"], \"id\": 1, \"method\": \"mining.submit\"}\n",
                   config.btc_address, job_id, extranonce2, ntime, nonce);
          if (VERBOSE) { Serial.print("Submitting share: "); Serial.println(payload); }
          client.print(payload);

          // Read response
          timeout = millis() + 5000; // 5 second timeout for submit response
          while(!client.available() && millis() < timeout) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            esp_task_wdt_reset();
          }
          if(client.available()) {
            client.readStringUntil('\n').toCharArray(line, sizeof(line));
            if (VERBOSE) { Serial.print("Share response: "); Serial.println(line); }
          }
        } else if(checkHalfShare(shaResult)) {
          if (VERBOSE) Serial.printf("%s: Half share, nonce: %u\n", (char *)name, nonce);
          halfshares++;
        }
      }

      hashes += (nonce - (batch_end - NONCE_BATCH_SIZE));

      if (nonce >= MAX_NONCE) {
        Serial.printf("%s: Reached MAX_NONCE (%d), restarting job\n", conn.worker_name, MAX_NONCE);
        break;
      }

      // Reset watchdog and yield occasionally
      if ((nonce % (NONCE_BATCH_SIZE * 16)) == 0) { // More frequent resets
        esp_task_wdt_reset();
        vTaskDelay(1);

        // Show progress every 64K hashes
        if ((nonce % 65536) == 0 && VERBOSE) {
          Serial.printf("%s: Processed %u hashes\n", conn.worker_name, nonce);
        }
      }
    }

    exit_mining:
    // This label is reached when mining completes, but we also need cleanup for all exit paths
    break; // Exit the main loop
  }

  // Cleanup resources - this ensures cleanup happens for all exit paths
  if (conn.client) {
    conn.client->stop();
    delete conn.client;
    conn.client = nullptr;
  }
  mbedtls_md_free(&ctx);

  Serial.printf("%s: Worker task exiting\n", conn.worker_name);
}

void runMonitor(void *name) {
  unsigned long start = millis();
  unsigned long last_report = start;
  long last_hashes = 0;

  while (1) {
    unsigned long now = millis();
    unsigned long elapsed = now - start;
    unsigned long interval = now - last_report;

    if (interval >= 5000) {
      long hash_diff = hashes - last_hashes;
      float instant_rate = (interval > 100) ? (hash_diff * 1000.0) / interval / 1000.0 : 0;
      float avg_rate = (elapsed > 1000) ? (hashes * 1000.0) / elapsed / 1000.0 : 0;

      Serial.printf(">>> Shares: %d | Hashes: %ld | Avg: %.2f KH/s | Current: %.2f KH/s | Temp: %.1f°C\n",
                   shares, hashes, avg_rate, instant_rate, temperatureRead());

      last_hashes = hashes;
      last_report = now;
    }

    esp_task_wdt_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup(){
  Serial.begin(115200);
  delay(500);

  // Disable watchdog timers for mining cores
  disableCore0WDT();
  disableCore1WDT();

  // Set CPU frequency to maximum for better performance
  setCpuFrequencyMhz(240);

  // Configure task watchdog for recovery
  esp_task_wdt_init(120, false); // 120 second timeout, no panic (reset instead)

  Serial.println("\nYAMUNA Miner");
  Serial.println("Bitcoin mining powered by ESP32");

  // Initialize web configuration
  initWebConfig();
  
  // Configuration reset removed for normal operation

  // Check if configured
  bool configured = isConfigured();
  Serial.printf("Device configured: %s\n", configured ? "YES" : "NO");
  
  if (!configured) {
    Serial.println("First time setup - starting configuration portal");
    startConfigAP();

    // Wait for configuration
    while (!isConfigured()) {
      handleConfigWeb();
      delay(100);
    }
  } else {
    Serial.println("Device already configured, connecting to WiFi...");
  }

  Serial.println("Connecting to WiFi...");
  WiFi.begin(config.wifi_ssid, config.wifi_password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed - starting configuration portal");
    startConfigAP();
    while (true) {
      handleConfigWeb();
      delay(100);
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("Pool: %s:%d\n", config.pool_url, config.pool_port);
  Serial.printf("Address: %s\n", config.btc_address);
  Serial.printf("Password: %s\n", config.pool_password);
  Serial.printf("Threads: %d\n", config.threads);

  // Test DNS resolution
  Serial.println("Testing DNS resolution...");
  IPAddress testIP;
  if (WiFi.hostByName(config.pool_url, testIP)) {
    Serial.printf("Successfully resolved %s to %s\n", config.pool_url, testIP.toString().c_str());
  } else {
    Serial.printf("Failed to resolve %s\n", config.pool_url);
  }

  Serial.println("Starting mining tasks...");

  // Start worker tasks with proper error handling
  char worker_names[config.threads][16];
  TaskHandle_t worker_handles[config.threads];

  for (size_t i = 0; i < config.threads; i++) {
    snprintf(worker_names[i], sizeof(worker_names[i]), "Worker[%d]", i);
    BaseType_t res = xTaskCreate(
      runWorker,
      worker_names[i],
      16384, // Further increased stack size for JSON parsing
      (void*)worker_names[i],
      2, // Higher priority for mining
      &worker_handles[i]
    );

    if (res == pdPASS) {
      Serial.printf("Started %s successfully on core %d\n", worker_names[i], i % 2);
      // Don't add to watchdog for now to avoid timeouts
      // esp_task_wdt_add(worker_handles[i]);
    } else {
      Serial.printf("Failed to start %s!\n", worker_names[i]);
    }
  }

  TaskHandle_t monitor_handle;
  BaseType_t monitor_res = xTaskCreate(runMonitor, "Monitor", 4096, NULL, 1, &monitor_handle);
  if (monitor_res == pdPASS) {
    // Don't add monitor to watchdog either
    // esp_task_wdt_add(monitor_handle);
    Serial.println("Monitor task started");
  }
}

void loop(){
  // Main loop kept minimal - all work done in tasks
  esp_task_wdt_reset();
  vTaskDelay(10000 / portTICK_PERIOD_MS); // 10 second delay
}