#!/bin/bash
# ESP32 Board Detection Script for YAMUNA

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}YAMUNA Board Detection${NC}"
echo "======================="

# Check for USB devices
echo "Scanning for ESP32 boards..."

# Common ESP32 USB IDs
ESP32_VENDORS=("10c4:ea60" "1a86:7523" "0403:6001" "303a:1001")
ESP32_NAMES=("Silicon Labs CP210x" "CH340 Serial" "FTDI Serial" "Espressif ESP32")

found_boards=()

if command -v lsusb >/dev/null 2>&1; then
    usb_devices=$(lsusb 2>/dev/null)

    for i in "${!ESP32_VENDORS[@]}"; do
        vendor_id="${ESP32_VENDORS[$i]}"
        board_name="${ESP32_NAMES[$i]}"

        if echo "$usb_devices" | grep -qi "$vendor_id"; then
            found_boards+=("$board_name")
            echo -e "${GREEN}Found: $board_name${NC}"
        fi
    done
else
    echo -e "${YELLOW}lsusb not available, checking serial ports...${NC}"
fi

# Check serial ports
echo ""
echo "Available serial ports:"
ports_found=false

for port in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART*; do
    if [ -c "$port" 2>/dev/null ]; then
        echo -e "${GREEN}$port${NC}"
        ports_found=true
    fi
done

if [ "$ports_found" = false ]; then
    echo -e "${YELLOW}No serial ports found${NC}"
fi

echo ""
echo "Board Suggestions:"
echo "=================="

if [ ${#found_boards[@]} -gt 0 ]; then
    echo -e "${GREEN}Detected ESP32-compatible boards!${NC}"
    echo ""
    echo "Recommended make commands:"
    echo "  make BOARD=esp32 build    # For standard ESP32"
    echo "  make BOARD=m5stack build  # For M5Stack boards"
    echo ""
    echo "Available environments in platformio.ini:"
    echo "  - esp32-release  (optimized)"
    echo "  - esp32-debug    (with debug info)"
    echo "  - m5stack-release"
    echo "  - m5stack-debug"
else
    echo -e "${YELLOW}No ESP32 boards detected${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "1. Make sure ESP32 is connected via USB"
    echo "2. Check USB cable (some are power-only)"
    echo "3. Install USB drivers if needed"
    echo "4. Try a different USB port"
fi

echo ""
echo "Current Makefile configuration:"
echo "==============================="
echo "Default board: esp32"
echo "Change with: make BOARD=m5stack build"
echo ""
echo "Note: This detection doesn't modify any files."
echo "Use environment variables or command-line options to override settings."