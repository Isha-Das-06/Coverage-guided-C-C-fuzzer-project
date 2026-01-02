#!/bin/bash

set -e

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  Coverage-Guided Fuzzer - Test Script                ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "[*] Building project..."
make clean && make all > /dev/null 2>&1

echo "[*] Setting up test environment..."
TEST_INPUT_DIR="test_inputs"
TEST_OUTPUT_DIR="test_outputs"

rm -rf "$TEST_INPUT_DIR" "$TEST_OUTPUT_DIR"
mkdir -p "$TEST_INPUT_DIR"
mkdir -p "$TEST_OUTPUT_DIR"

echo "[*] Creating seed corpus..."
echo "A" > "$TEST_INPUT_DIR/seed_a.txt"
echo "B" > "$TEST_INPUT_DIR/seed_b.txt"
echo "test" > "$TEST_INPUT_DIR/seed_test.txt"
echo "ABCDEFGH" > "$TEST_INPUT_DIR/seed_long.txt"

echo "[*] Running fuzzer on simple_target (5 seconds)..."
timeout 5 ./bin/fuzzer -i "$TEST_INPUT_DIR" -o "$TEST_OUTPUT_DIR" -t ./bin/simple_target -v || true

echo ""
echo "[*] Test Results:"
echo "────────────────────────────────────────────────────────"

if [ -d "$TEST_OUTPUT_DIR/crashes" ]; then
    crash_count=$(find "$TEST_OUTPUT_DIR/crashes" -name "crash_*" -type f | wc -l)
    echo "[+] Crashes found: $crash_count"
    ls -la "$TEST_OUTPUT_DIR/crashes" 2>/dev/null || echo "    (none)"
fi

if [ -d "$TEST_OUTPUT_DIR/corpus" ]; then
    corpus_count=$(find "$TEST_OUTPUT_DIR/corpus" -type f | wc -l)
    echo "[+] Corpus entries discovered: $corpus_count"
fi

if [ -f "$TEST_OUTPUT_DIR/stats.txt" ]; then
    echo "[+] Stats log:"
    tail -3 "$TEST_OUTPUT_DIR/stats.txt" | sed 's/^/    /'
fi

echo ""
echo "[*] Cleanup test files..."
rm -rf "$TEST_INPUT_DIR" "$TEST_OUTPUT_DIR"

echo "[+] Test complete!"
