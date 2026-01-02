#!/bin/bash

set -e

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  Coverage-Guided Fuzzer - Build Script               ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "[*] Cleaning previous builds..."
make clean 2>/dev/null || true

echo "[*] Building fuzzer and targets..."
make all

echo ""
echo "[+] Build complete!"
echo ""
echo "Binaries ready:"
echo "  - bin/fuzzer           Main fuzzer binary"
echo "  - bin/simple_target    Simple vulnerable program"
echo "  - bin/vulnerable_target Advanced vulnerable program"
echo ""
echo "Next steps:"
echo "  1. Create seed corpus:  mkdir -p inputs && echo 'test' > inputs/seed.txt"
echo "  2. Create output dir:   mkdir -p outputs"
echo "  3. Start fuzzing:       ./bin/fuzzer -i inputs -o outputs -t ./bin/simple_target"
echo ""
