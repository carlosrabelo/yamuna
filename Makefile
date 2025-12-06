MAKEFLAGS += --no-print-directory

.DEFAULT_GOAL := help

-include .env

UPLOAD_PORT      ?= /dev/ttyUSB0
MONITOR_PORT     ?= /dev/ttyUSB0
MONITOR_SPEED    ?= 115200
UPLOAD_SPEED     ?= 921600

SUPPORTED_BOARDS := esp32 m5stack
BOARD            ?= esp32

ifeq ($(BOARD),m5stack)
BUILD_ENV := m5stack-release
else ifeq ($(BOARD),esp32)
BUILD_ENV := esp32-release
else
$(error Unsupported BOARD=$(BOARD). Supported: $(SUPPORTED_BOARDS))
endif

.PHONY: build check check-pio clean deps detect-port erase flash help install-pio monitor test upload upload-fs

build: check-pio ## Compile firmware (BOARD=esp32|m5stack)
	./.make/run-pio.sh run --environment $(BUILD_ENV)

upload: check-pio ## Upload firmware to device
	./.make/run-pio.sh run --environment $(BUILD_ENV) --target upload \
	    --upload-port $(UPLOAD_PORT) \
	    --upload-speed $(UPLOAD_SPEED)

flash: build upload ## Compile and upload

monitor: check-pio ## Open serial monitor
	./.make/run-pio.sh device monitor \
	    --port $(MONITOR_PORT) \
	    --baud $(MONITOR_SPEED)

clean: ## Remove build artifacts
	./.make/clean.sh

deps: check-pio ## Install dependencies
	./.make/run-pio.sh pkg install

check: check-pio ## Run static analysis
	./.make/run-pio.sh check --environment $(BUILD_ENV)

test: check-pio ## Run unit tests
	./.make/run-pio.sh test

erase: check-pio ## Erase device flash memory
	./.make/run-pio.sh run --environment $(BUILD_ENV) --target erase

upload-fs: check-pio ## Upload filesystem image
	./.make/run-pio.sh run --environment $(BUILD_ENV) --target uploadfs

detect-port: ## Auto-detect board USB port and save to .env
	@./.make/detect_board.sh

install-pio: ## Install PlatformIO
	@./.make/install-pio.sh

check-pio: ## Verify PlatformIO is installed
	@./.make/check-pio.sh

help: ## Show available targets
	@echo "YAMUNA - Available targets (BOARD=esp32|m5stack)"
	@echo ""
	@grep -hE '^[a-zA-Z_-]+:.*## ' $(MAKEFILE_LIST) \
		| sort \
		| awk 'BEGIN {FS = ":.*## "} {printf "  %-15s %s\n", $$1, $$2}'
