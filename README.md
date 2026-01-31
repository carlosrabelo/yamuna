# YAMUNA Bitcoin Miner

Educational Bitcoin mining implementation for ESP32 microcontrollers

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP32](https://img.shields.io/badge/platform-ESP32-green.svg)](https://espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)

## Highlights

- Multi-core mining using both ESP32 cores for maximum hash rate
- Stratum protocol compatibility with standard mining pools
- Real-time hash rate, temperature, and share statistics every 5 seconds
- SHA-256 midstate cache — first hash half computed once per job
- Automatic valid share detection and submission to the pool
- Adaptive difficulty adjustment based on share frequency
- Browser-based configuration portal for WiFi and pool settings
- Smart WiFi handling with automatic fallback AP mode
- Multiple mining pool presets with custom pool support
- Watchdog timer and automatic error recovery for reliable operation
- Native ESP32-WROOM-32 and M5Stack Core support with hardware auto-detection

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Usage](#usage)
- [Configuration](#configuration)
- [Project Layout](#project-layout)
- [Development](#development)
- [License](#license)

## Overview

YAMUNA is Bitcoin mining firmware for ESP32 microcontrollers. It is not economically viable for profit; it exists as an educational tool for understanding Bitcoin mining protocols, ESP32 development, and cryptocurrency fundamentals.

## Prerequisites

### Hardware

- **ESP32 Development Board** (ESP32-WROOM-32 recommended)
- **WiFi Network** for pool connectivity
- **USB Cable** for programming and power
- **Optional**: Heat sink for thermal management

### Software

- **PlatformIO** (recommended) or Arduino IDE
- **Python** 3.6+ (for PlatformIO)

## Installation

### Build from Source

```bash
git clone https://github.com/carlosrabelo/yamuna.git
cd yamuna
make deps
make flash
```

Upload web assets to SPIFFS (required for the configuration portal):

```bash
make upload-fs
```

## Quick Start

```bash
git clone https://github.com/carlosrabelo/yamuna.git
cd yamuna
make deps
make flash
make upload-fs
make monitor
```

### Initial Configuration

1. Connect to the setup network `YAMUNA` (password: `yamuna123`)
2. Open `http://192.168.4.1` and configure WiFi, Bitcoin address/username, pool, and pool password (default: `x`)
3. The device restarts, joins your WiFi, and starts mining on all available cores

## Usage

### Normal Operation

`VERBOSE=0` (default):

```
YAMUNA Miner v1.0
...
[  24.32 KH/s] 3 shares, 56.2C, diff 1, job a3f92b
yay!!! Share found!
```

`VERBOSE=1`:

```
=== YAMUNA Miner - Modular Architecture ===
...
Pool: public-pool.io:21496
Address: bc1qexample...
...
>>> Shares: 3 | Hashes: 2847296 | Avg: 24.32 KH/s | Current: 25.1 KH/s | Temp: 56.2°C | Stratum Diff: 1 | Job: a3f92b
Worker[0]: VALID SHARE! nonce: 1847263, difficulty: 1
```

### Performance Monitoring

The system prints statistics every 5 seconds:

- **Hash Rate**: Instantaneous and average KH/s
- **Shares**: Total valid shares submitted to the pool
- **Temperature**: ESP32 internal temperature
- **Stratum Diff**: Current difficulty assigned by the pool
- **Job**: Current job ID from the pool

## Configuration

### Supported Mining Pools

| Pool | URL | Port | Type |
|------|-----|------|------|
| **Public Pool** (Recommended) | `public-pool.io` | `21496` | Public |
| **Solo CK Pool** | `solo.ckpool.org` | `3333` | Solo |
| **Custom** | Your pool URL | Your port | Custom |

### Performance Profiles

| Configuration | Hash Rate | Power | Temperature | Stability |
|---------------|-----------|-------|-------------|-----------|
| **Single Thread** | 13-15 KH/s | ~1.5W | 45-55°C | Excellent |
| **Dual Thread** | 24-26 KH/s | ~2.5W | 55-65°C | Good |

### Adaptive Difficulty

YAMUNA automatically adjusts local share difficulty based on how frequently shares are found. Controlled via `src/configs.h`:

```cpp
#define SHARE_DIFFICULTY_LEVEL 2      // Initial level: 1 (easiest) to 5 (hardest)
#define MAX_DIFFICULTY_LEVEL 5
#define TARGET_SHARE_INTERVAL 120000  // Target: one share every 2 minutes
#define ADAPTIVE_DIFFICULTY 1         // 0=fixed, 1=auto-adjust
#define DIFFICULTY_ADJUST_INTERVAL_MS 60000  // Minimum time between adjustments
```

### Debug Configuration

Control output verbosity in `src/configs.h`:

```cpp
#define DEBUG 0    // 0=off, 1=development mode
#define VERBOSE 0  // 0=clean output, 1=detailed messages
```

- `VERBOSE=0`: Clean production output (cpuminer style)
- `VERBOSE=1`: Detailed pool communication and operational messages
- `DEBUG=1`: Full development debugging with technical details

Serial ports and speeds can be overridden in `.env` (see `.env.example`):

```bash
UPLOAD_PORT=/dev/ttyUSB0
MONITOR_PORT=/dev/ttyUSB0
MONITOR_SPEED=115200
UPLOAD_SPEED=921600
```

## Project Layout

```
src/                 # Firmware source (PlatformIO)
data/                # SPIFFS web assets (config portal HTML)
test/                # Unity unit tests
.make/               # PlatformIO helper scripts
platformio.ini       # PlatformIO environments and deps
Makefile             # Build orchestration (BOARD=esp32|m5stack)
```

## Development

```bash
make build           # Compile firmware (BOARD=esp32|m5stack)
make upload          # Upload firmware to device
make flash           # Build and upload
make upload-fs       # Upload filesystem image
make monitor         # Open serial monitor
make test            # Run unit tests
make check           # Run static analysis
make clean           # Remove build artifacts
make deps            # Install dependencies
make detect-port     # Auto-detect board USB port into .env
make erase           # Erase device flash memory
make install-pio     # Install PlatformIO
make help            # Show available targets
```

## Security Features

### Built-in Protections

- Buffer overflow protection with bounds-checked string handling
- Automatic memory cleanup and leak prevention
- Input and parameter validation
- Null pointer checks against invalid memory access
- Watchdog timer for automatic recovery from system hangs

### Network Security

- Connection timeouts to prevent hanging TCP sessions
- Automatic reconnection with fixed retry intervals on pool or WiFi failure
- DNS validation before direct connection attempts

## Important Disclaimers

### Educational Purpose Only

ESP32 mining is **not** economically viable. Modern ASIC miners are millions of times more efficient. This project is designed for:

- Learning Bitcoin mining algorithms and the Stratum protocol
- Exploring ESP32 programming and IoT concepts
- Hands-on cryptocurrency and blockchain education
- Academic and experimental research

### Hardware Considerations

- Continuous operation draws about 1.5–2.5W
- Ensure adequate cooling and ventilation
- Extended operation may reduce hardware lifespan
- Use a quality USB power source for stability

### Economic Reality

- ESP32 mining will generate negligible Bitcoin rewards
- Electricity costs will exceed any potential earnings

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feat/description`
3. Commit with Conventional Commits: `git commit -m "feat: add X"`
4. Push and open a pull request

## Acknowledgments

- Satoshi Nakamoto — for creating Bitcoin and inspiring this project
- Espressif Systems — for the ESP32 platform
- Bitcoin Community — for open protocols and educational resources
- Contributors — everyone who helps improve this project

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
