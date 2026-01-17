#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Minimal Arduino-style String implementation for unit tests
class String {
public:
    String() = default;
    String(const char* s) : value_(s ? s : "") {}
    String(const std::string& s) : value_(s) {}
    String(int v) : value_(std::to_string(v)) {}
    String(unsigned int v) : value_(std::to_string(v)) {}
    String(long v) : value_(std::to_string(v)) {}
    String(unsigned long v) : value_(std::to_string(v)) {}

    String(const String&) = default;
    String(String&&) noexcept = default;
    String& operator=(const String&) = default;
    String& operator=(String&&) noexcept = default;

    size_t length() const { return value_.length(); }
    bool isEmpty() const { return value_.empty(); }
    const char* c_str() const { return value_.c_str(); }
    const char* data() const { return value_.data(); }

    String substring(size_t begin, size_t end) const {
        if (begin >= value_.size()) return String();
        size_t len = (end > value_.size()) ? value_.size() - begin : end - begin;
        return String(value_.substr(begin, len));
    }

    void replace(const String& from, const String& to) {
        if (from.value_.empty()) return;
        size_t pos = 0;
        while ((pos = value_.find(from.value_, pos)) != std::string::npos) {
            value_.replace(pos, from.value_.length(), to.value_);
            pos += to.value_.length();
        }
    }

    int toInt() const {
        if (value_.empty()) return 0;
        return std::atoi(value_.c_str());
    }

    float toFloat() const {
        if (value_.empty()) return 0.0f;
        return std::strtof(value_.c_str(), nullptr);
    }

    String& operator+=(const String& other) {
        value_ += other.value_;
        return *this;
    }

    bool operator==(const String& other) const { return value_ == other.value_; }
    bool operator!=(const String& other) const { return !(*this == other); }
    bool operator==(const char* other) const { return value_ == (other ? other : ""); }
    bool operator!=(const char* other) const { return !(*this == other); }

    char operator[](size_t idx) const { return value_[idx]; }

    std::string std() const { return value_; }

private:
    std::string value_;
};

inline String operator+(const String& lhs, const String& rhs) {
    return String(lhs.std() + rhs.std());
}

inline String operator+(const char* lhs, const String& rhs) {
    return String(std::string(lhs ? lhs : "") + rhs.std());
}

inline String operator+(const String& lhs, const char* rhs) {
    return String(lhs.std() + std::string(rhs ? rhs : ""));
}

// Serial stub
class SerialClass {
public:
    void begin(unsigned long) {}
    void println(const String&) {}
    void println(const char*) {}
    void println(int) {}
    void println() {}
    void print(const String&) {}
    void print(const char*) {}
    void printf(const char* fmt, ...) {
        (void)fmt;
    }
};

inline SerialClass Serial;

inline void delay(int) {}
inline unsigned long millis() {
    static unsigned long counter = 0;
    counter += 1;
    return counter;
}

// File & SPIFFS stubs
class File {
public:
    File() : valid_(false) {}
    explicit File(const String& data) : data_(data), pos_(0), valid_(true) {}

    bool operator!() const { return !valid_; }
    explicit operator bool() const { return valid_; }

    String readString() {
        return data_;
    }

    size_t size() const { return data_.length(); }
    void close() { valid_ = false; }

private:
    String data_;
    size_t pos_{0};
    bool valid_;
};

class SPIFFSClass {
public:
    bool begin(bool) { return true; }
    File open(const char* path, const char* mode = "r") {
        (void)mode;
        auto it = files_.find(path ? path : "");
        if (it == files_.end()) {
            return File();
        }
        return File(it->second);
    }

    void addFile(const String& path, const String& content) {
        files_[path.std()] = content;
    }

private:
    std::unordered_map<std::string, String> files_;
};

inline SPIFFSClass SPIFFS;

// Preferences stub
class Preferences {
public:
    bool begin(const char*, bool) { return true; }
    void end() {}
    void clear() {
        strings_.clear();
        ints_.clear();
        bools_.clear();
    }

    bool putString(const char* key, const char* value) {
        strings_[key ? key : ""] = value ? value : "";
        return true;
    }

    bool putString(const char* key, const String& value) {
        strings_[key ? key : ""] = value.std();
        return true;
    }

    String getString(const char* key, const char* default_value = "") const {
        auto it = strings_.find(key ? key : "");
        if (it == strings_.end()) {
            return String(default_value ? default_value : "");
        }
        return String(it->second);
    }

    bool putInt(const char* key, int value) {
        ints_[key ? key : ""] = value;
        return true;
    }

    int getInt(const char* key, int default_value = 0) const {
        auto it = ints_.find(key ? key : "");
        if (it == ints_.end()) {
            return default_value;
        }
        return it->second;
    }

    bool putBool(const char* key, bool value) {
        bools_[key ? key : ""] = value;
        return true;
    }

    bool getBool(const char* key, bool default_value = false) const {
        auto it = bools_.find(key ? key : "");
        if (it == bools_.end()) {
            return default_value;
        }
        return it->second;
    }

private:
    std::unordered_map<std::string, std::string> strings_;
    std::unordered_map<std::string, int> ints_;
    std::unordered_map<std::string, bool> bools_;
};

// WebServer stub
const int HTTP_GET = 0;
const int HTTP_POST = 1;

class WebServer {
public:
    explicit WebServer(int) {}

    void on(const char*, int, void (*)()) {}
    void begin() {}
    void handleClient() {}

    void send(int, const char*, const char*) {}
    void send(int, const char*, const String&) {}

    String arg(const String& name) const {
        auto it = args_.find(name.std());
        if (it == args_.end()) {
            return String();
        }
        return String(it->second);
    }

    void setArg(const String& name, const String& value) {
        args_[name.std()] = value.std();
    }

private:
    std::unordered_map<std::string, std::string> args_;
};

// WiFi stub
const int WIFI_AP = 2;

class IPAddress {
public:
    String toString() const { return String("192.168.4.1"); }
};

class WiFiClass {
public:
    void disconnect(bool) {}
    void mode(int) {}
    void softAP(const char*, const char*) {}
    IPAddress softAPIP() const { return IPAddress(); }
};

inline WiFiClass WiFi;

// ESP stub
class ESPClass {
public:
    void restart() {}
};

inline ESPClass ESP;
