# YAMUNA Bitcoin Miner Makefile

.DEFAULT_GOAL := help

# Board/target selection
SUPPORTED_BOARDS := esp32 m5stack
BOARD ?= esp32

ifeq ($(BOARD),m5stack)
BUILD_ENV := m5stack-release
else ifeq ($(BOARD),esp32)
BUILD_ENV := esp32-release
else
$(error Unsupported BOARD '$(BOARD)'. Supported values: $(SUPPORTED_BOARDS))
endif

# Variables
CORE_DIR = core
VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo "v1.0.0-dev")

.PHONY: build upload monitor clean deps check info help erase detect _run-pio

# Build targets
build: check-pio ## Build firmware for selected board
	@echo "Building YAMUNA $(VERSION) for $(BOARD)..."
	@$(MAKE) --no-print-directory _run-pio ARGS="run --environment $(BUILD_ENV)"

upload: check-pio ## Upload firmware to target board
	@echo "Uploading to $(BOARD)..."
	@$(MAKE) --no-print-directory _run-pio ARGS="run --target upload --environment $(BUILD_ENV)"

monitor: check-pio ## Start serial monitor
	@echo "Starting serial monitor..."
	@$(MAKE) --no-print-directory _run-pio ARGS="device monitor --baud 115200"

flash: build upload ## Build and upload firmware

# Maintenance targets
clean: check-pio ## Clean build artifacts
	@echo "Cleaning build artifacts..."
	@$(MAKE) --no-print-directory _run-pio ARGS="run --target clean"
	@rm -rf $(CORE_DIR)/.pio/build $(CORE_DIR)/.pio/libdeps 2>/dev/null || true

deps: check-pio ## Install project dependencies
	@echo "Installing dependencies..."
	@$(MAKE) --no-print-directory _run-pio ARGS="pkg install"

erase: check-pio ## Erase flash memory completely
	@echo "Erasing flash memory..."
	@$(MAKE) --no-print-directory _run-pio ARGS="run --target erase --environment $(BUILD_ENV)"

# Quality targets
check: check-pio ## Check project configuration
	@$(MAKE) --no-print-directory _run-pio ARGS="check --environment $(BUILD_ENV)"

test: check-pio ## Run unit tests
	@echo "Running unit tests..."
	@$(MAKE) --no-print-directory _run-pio ARGS="test -e test"

detect: ## Auto-detect connected ESP32 boards
	@./scripts/detect_board.sh


# Internal target to run PlatformIO
_run-pio:
	@cd $(CORE_DIR) && ../scripts/pio_check.sh run $(ARGS)

# Information targets
info: ## Show project information
	@echo "YAMUNA Bitcoin Miner"
	@echo "===================="
	@echo "Version: $(VERSION)"
	@echo "Board: $(BOARD)"
	@echo "Environment: $(BUILD_ENV)"
	@echo "Core Directory: $(CORE_DIR)"
	@echo "Supported Boards: $(SUPPORTED_BOARDS)"

help: ## Show this help
	@echo "YAMUNA Bitcoin Miner"
	@echo ""
	@echo "Usage: make [TARGET]"
	@echo ""
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | sort | awk -F':|## ' '{printf " %-20s %s\n", $$1, $$3}'
	@echo ""
	@echo "Examples:"
	@echo "  make build            # Build firmware"
	@echo "  make flash            # Build and upload"
	@echo "  make monitor          # Start serial monitor"
	@echo "  make detect           # Auto-detect ESP32 boards"
	@echo "  make BOARD=m5stack build  # Build for M5Stack"

# Internal targets
check-pio: ## Check if PlatformIO is installed
	@./scripts/pio_check.sh check