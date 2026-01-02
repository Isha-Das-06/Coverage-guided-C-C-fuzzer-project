#!/bin/bash

set -e

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  Coverage-Guided Fuzzer - Configuration Script       ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

ERRORS=0

echo "[*] Checking build dependencies..."
echo ""

# Check for required commands
check_command() {
    if ! command -v "$1" &> /dev/null; then
        echo "[✗] ERROR: $1 is not installed"
        echo "    Install with: sudo apt-get install $2"
        ERRORS=$((ERRORS + 1))
    else
        echo "[✓] Found: $1"
    fi
}

# Check for make
if ! command -v make &> /dev/null && ! command -v gmake &> /dev/null; then
    echo "[✗] ERROR: make is not installed"
    echo "    Install with: sudo apt-get install make"
    ERRORS=$((ERRORS + 1))
else
    echo "[✓] Found: make"
fi

check_command gcc "build-essential"
check_command cc "build-essential"

echo ""

if [ $ERRORS -gt 0 ]; then
    echo "[!] Build prerequisites not met"
    echo ""
    echo "On Ubuntu/Debian:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install build-essential"
    echo ""
    echo "On macOS:"
    echo "  xcode-select --install"
    echo ""
    echo "On Windows:"
    echo "  1. Install WSL 2: https://docs.microsoft.com/windows/wsl/"
    echo "  2. Inside WSL, run: sudo apt-get install build-essential"
    echo ""
    exit 1
fi

echo "[✓] All dependencies found!"
echo ""
echo "You can now run:"
echo "  make clean && make all"
echo ""
