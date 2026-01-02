# Coverage instrumentation requires Clang (LLVM's trace-pc-guard syntax)
# On most Linux systems: clang should be available. If not, install with:
#   Ubuntu/Debian: sudo apt-get install clang
#   macOS: brew install llvm or xcode-select --install
CC = clang
CFLAGS = -Wall -Wextra -g -O1 -fPIC
LDFLAGS = -lm -lrt
MKDIR = mkdir -p

SRC_DIR = src
BIN_DIR = bin
EXAMPLES_DIR = examples

FUZZER_SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/fuzzer.c $(SRC_DIR)/mutator.c \
                 $(SRC_DIR)/coverage.c $(SRC_DIR)/corpus.c $(SRC_DIR)/crash_handler.c

FUZZER_OBJECTS = $(FUZZER_SOURCES:.c=.o)
FUZZER_HEADERS = $(wildcard $(SRC_DIR)/*.h)
COVERAGE_SHARED = $(SRC_DIR)/coverage_shared.o

FUZZER_BIN = $(BIN_DIR)/fuzzer
SIMPLE_TARGET = $(BIN_DIR)/simple_target
VULNERABLE_TARGET = $(BIN_DIR)/vulnerable_target

# Coverage instrumentation flags
COVERAGE_CFLAGS = -fsanitize-coverage=trace-pc-guard -fsanitize=address

.PHONY: all clean help dirs

all: dirs $(FUZZER_BIN) $(SIMPLE_TARGET) $(VULNERABLE_TARGET)

dirs:
	@$(MKDIR) $(BIN_DIR)

$(FUZZER_BIN): $(FUZZER_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[+] Built: $@"

# Depend on headers, otherwise editing a header leaves stale objects behind.
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(FUZZER_HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(COVERAGE_SHARED): $(SRC_DIR)/coverage_shared.c
	$(CC) $(COVERAGE_CFLAGS) $(CFLAGS) -c -o $@ $<

$(SIMPLE_TARGET): $(EXAMPLES_DIR)/simple_target.c $(COVERAGE_SHARED)
	$(CC) $(COVERAGE_CFLAGS) $(CFLAGS) -o $@ $(EXAMPLES_DIR)/simple_target.c $(COVERAGE_SHARED) $(LDFLAGS)
	@echo "[+] Built: $@ (with coverage instrumentation)"

$(VULNERABLE_TARGET): $(EXAMPLES_DIR)/vulnerable.c $(COVERAGE_SHARED)
	$(CC) $(COVERAGE_CFLAGS) $(CFLAGS) -o $@ $(EXAMPLES_DIR)/vulnerable.c $(COVERAGE_SHARED) $(LDFLAGS)
	@echo "[+] Built: $@ (with coverage instrumentation)"

clean:
	rm -f $(SRC_DIR)/*.o $(FUZZER_BIN) $(SIMPLE_TARGET) $(VULNERABLE_TARGET)
	@echo "[*] Clean complete"

help:
	@echo "Coverage-Guided Fuzzer - Build targets:"
	@echo ""
	@echo "  make              - Build fuzzer and example targets"
	@echo "  make clean        - Remove built binaries"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Built binaries:"
	@echo "  $(FUZZER_BIN)           - Main fuzzer"
	@echo "  $(SIMPLE_TARGET)     - Simple vulnerable target (with coverage)"
	@echo "  $(VULNERABLE_TARGET) - Advanced vulnerable target (with coverage)"
	@echo ""
	@echo "Usage example:"
	@echo "  mkdir -p inputs outputs"
	@echo "  echo 'test' > inputs/seed.txt"
	@echo "  ./$(FUZZER_BIN) -i inputs -o outputs -t ./$(SIMPLE_TARGET)"
