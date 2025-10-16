#!/bin/bash
# PlatformIO Check Script for YAMUNA

PLATFORM="pio"
PYTHON_VENV="$HOME/.platformio/penv"

check_pio() {
    if command -v $PLATFORM >/dev/null 2>&1; then
        echo "PlatformIO found"
        return 0
    elif [ -f "$PYTHON_VENV/bin/$PLATFORM" ]; then
        echo "PlatformIO found in virtual environment"
        return 0
    else
        echo "✗ PlatformIO not found. Install with:"
        echo "  pip install platformio"
        return 1
    fi
}

run_pio() {
    if command -v $PLATFORM >/dev/null 2>&1; then
        $PLATFORM "$@"
    elif [ -f "$PYTHON_VENV/bin/$PLATFORM" ]; then
        "$PYTHON_VENV/bin/$PLATFORM" "$@"
    else
        echo "✗ PlatformIO not available"
        return 1
    fi
}

case "${1:-check}" in
    check)
        check_pio
        ;;
    run)
        shift
        run_pio "$@"
        ;;
    *)
        echo "Usage: $0 {check|run}"
        exit 1
        ;;
esac