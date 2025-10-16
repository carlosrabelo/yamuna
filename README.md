# YAMUNA Bitcoin Miner

> Educational Bitcoin mining implementation for ESP32 microcontrollers

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP32](https://img.shields.io/badge/platform-ESP32-green.svg)](https://espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)

**YAMUNA** is a modern Bitcoin mining software designed for ESP32 microcontrollers. While not economically viable for profit, it serves as an excellent educational tool for understanding Bitcoin mining protocols, ESP32 development, and cryptocurrency fundamentals.

---

## Features

### Core Mining
- **Multi-core Processing**: Utilizes both ESP32 cores for optimal performance
- **Stratum Protocol**: Full compatibility with standard mining pools
- **Real-time Monitoring**: Live hash rate, temperature, and share statistics
- **Optimized SHA-256**: Custom implementation with batch processing
- **Share Detection**: Automatic detection and submission of valid shares

### Configuration & Management
- **Web-based Setup**: Intuitive browser interface for configuration
- **WiFi Management**: Smart connection handling with fallback AP mode
- **Pool Management**: Support for multiple mining pools with presets
- **OTA Ready**: Designed for over-the-air firmware updates
- **Robust Error Handling**: Watchdog protection and automatic recovery

### Hardware Support
- **ESP32 Development Boards**: Standard ESP32-WROOM-32 and variants
- **M5Stack**: Native support for M5Stack Core devices
- **Auto-detection**: Automatic hardware detection and optimization

---

## Requirements

### Hardware
- **ESP32 Development Board** (ESP32-WROOM-32 recommended)
- **WiFi Network** for pool connectivity
- **USB Cable** for programming and power
- **Optional**: Heat sink for thermal management

### Software
- **PlatformIO** (recommended) or Arduino IDE
- **Git** for version control
- **Python** 3.6+ (for PlatformIO)

---

## Quick Start

### 1. Clone and Setup
```bash
# Clone the repository
git clone https://github.com/yourusername/yamuna.git
cd yamuna

# Install dependencies (if using PlatformIO directly)
pio pkg install
```

### 2. Build and Flash
```bash
# Build firmware
make build

# Flash to ESP32
make upload

# Monitor serial output
make monitor
```

### 3. Initial Configuration
1. **Connect to Setup Network**
   - Network: `YAMUNA`
   - Password: `yamuna123`

2. **Open Configuration Portal**
   - URL: `http://192.168.4.1`
   - Configure WiFi credentials
   - Set Bitcoin address/username
   - Select mining pool
   - Choose thread count (1-2)

3. **Start Mining**
   - Device restarts automatically
   - Connects to your WiFi
   - Begins mining immediately

---

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

### Debug Configuration

Control output verbosity in `core/src/configs.h`:

```cpp
#define DEBUG 0    // 0=off, 1=development mode
#define VERBOSE 0  // 0=clean, 1=detailed messages
```

**Output Modes:**
- `VERBOSE=0`: Clean production output with essential information
- `VERBOSE=1`: Detailed pool communication and operational messages
- `DEBUG=1`: Full development debugging with technical details

---

## Build System

### Make Commands
```bash
# Core commands
make build          # Compile firmware
make upload         # Flash to ESP32
make monitor        # Start serial monitor
make flash          # Build + upload in one step

# Maintenance
make clean          # Clean build artifacts
make deps           # Install dependencies
make erase          # Erase flash completely

# Development
make check          # Run code analysis
make detect         # Auto-detect ESP32 boards
make info           # Show project information

# Board-specific builds
make BOARD=esp32 build      # ESP32 development board
make BOARD=m5stack build    # M5Stack devices
```

### PlatformIO Environments
```ini
# Release builds (optimized)
esp32-release       # ESP32 development boards
m5stack-release     # M5Stack devices

# Debug builds (with symbols)
esp32-debug         # ESP32 with debugging
m5stack-debug       # M5Stack with debugging
```

---

## Mining Output

### Normal Operation
```
YAMUNA Miner v1.0
Bitcoin mining powered by ESP32

WiFi connected!
IP address: 192.168.1.100
Pool: public-pool.io:21496
Address: bc1qexample...
Threads: 2

Authorization successful
Job received - starting mining

>>> Shares: 3 | Hashes: 2847296 | Avg: 24.32 KH/s | Current: 25.1 KH/s | Temp: 56.2°C
Half-share found! Hash: 8bec30dd792358f1...
SHARE FOUND! Hash: 00000012a4c5f832...
Worker[0]: Share found! nonce: 1847263
```

### Performance Monitoring
The system provides real-time statistics every 5 seconds:
- **Shares**: Total valid shares found
- **Hashes**: Total hash calculations performed
- **Avg Rate**: Average hash rate since startup
- **Current Rate**: Instantaneous hash rate
- **Temperature**: ESP32 internal temperature

---

## Project Structure

```
yamuna/
├── core/                    # Core firmware
│   ├── src/                 # Source code
│   │   ├── main.cpp         # Main mining logic
│   │   ├── webconfig.cpp    # Web configuration
│   │   ├── webconfig.h      # Web interface headers
│   │   └── configs.h        # Configuration constants
│   └── platformio.ini       # PlatformIO configuration
├── scripts/                 # Build automation
│   ├── build.sh             # Build automation
│   ├── detect_board.sh      # Hardware detection
│   └── pio_check.sh         # PlatformIO validation
├── Makefile                 # Build system
├── README.md                # This file
└── README-PT.md             # Portuguese documentation
```

---

## Security Features

### Built-in Protections
- **Buffer Overflow Protection**: Safe string handling with bounds checking
- **Memory Management**: Automatic cleanup and leak prevention
- **Input Validation**: Comprehensive parameter validation
- **Null Pointer Checks**: Protection against invalid memory access
- **Watchdog Timer**: Automatic recovery from system hangs

### Network Security
- **Connection Timeout**: Prevents hanging connections
- **Retry Logic**: Exponential backoff for failed connections
- **DNS Validation**: Secure hostname resolution
- **SSL Ready**: Prepared for encrypted pool connections

---

## Important Disclaimers

### Educational Purpose Only
**ESP32 mining is NOT economically viable.** Modern ASIC miners are millions of times more efficient. This project is designed for:

- **Learning Bitcoin Protocols**: Understand mining algorithms and Stratum protocol
- **ESP32 Development**: Explore microcontroller programming and IoT concepts
- **Cryptocurrency Education**: Hands-on experience with blockchain technology
- **Research Projects**: Academic and experimental use cases

### Hardware Considerations
- **Power Consumption**: Continuous operation at 1.5-2.5W
- **Heat Generation**: Ensure adequate cooling and ventilation
- **Component Stress**: Extended operation may reduce hardware lifespan
- **Power Supply**: Use quality USB power source for stability

### Economic Reality
- **No Profit**: ESP32 mining will generate negligible Bitcoin rewards
- **Electricity Costs**: Power costs will exceed any potential earnings
- **Opportunity Cost**: Hardware could be used for more productive purposes

---

## Contributing

We welcome contributions to improve YAMUNA! Here are some areas of interest:

### High Priority
- Performance optimizations and mining algorithm improvements
- Additional mining pool protocol support
- Enhanced web interface with real-time dashboards
- Power management and thermal optimization

### Medium Priority
- Support for additional ESP32 variants and boards
- Improved error handling and recovery mechanisms
- Advanced configuration options and mining strategies
- Documentation improvements and tutorials

### Low Priority
- Code refactoring and architectural improvements
- Unit tests and automated testing framework
- Internationalization and localization
- Advanced analytics and logging features

### Development Workflow
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with clear, descriptive commits
4. Add tests if applicable
5. Update documentation
6. Submit a pull request with detailed description

---

## License

This project is open source and available under the [MIT License](LICENSE).

---

## Support & Community

### Getting Help
- **Bug Reports**: [GitHub Issues](https://github.com/yourusername/yamuna/issues)
- **Feature Requests**: [GitHub Discussions](https://github.com/yourusername/yamuna/discussions)
- **Documentation**: [Project Wiki](https://github.com/yourusername/yamuna/wiki)

### Community Guidelines
- Be respectful and constructive in all interactions
- Search existing issues before creating new ones
- Provide detailed information when reporting problems
- Follow the code of conduct in all community spaces

---

## Acknowledgments

- **Satoshi Nakamoto** - For creating Bitcoin and inspiring this project
- **Espressif Systems** - For the excellent ESP32 platform
- **Bitcoin Community** - For open protocols and educational resources
- **Contributors** - Everyone who helps improve this project

---

<div align="center">

**Powered by ESP32 | Built for Bitcoin | Made for Learning**

*Happy Mining! (Educationally speaking)*

</div>