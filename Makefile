# Makefile for RISC-V RV32I Instruction Decoder
# Targets: all, clean, test, debug, valgrind

# ─── Compiler and flags ───────────────────────────────────────────
CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99     # Enable all warnings; use C99 standard
DBGFLAGS = -g -O0                     # Debug: include symbols, no optimisation
RELFLAGS = -O2                        # Release: optimise for speed

# ─── Directory layout ─────────────────────────────────────────────
SRCDIR  = src
INCDIR  = include
TESTDIR = test
BINDIR  = bin
OBJDIR  = build

# ─── Source files ─────────────────────────────────────────────────
SRCS = $(SRCDIR)/main.c $(SRCDIR)/decoder.c $(SRCDIR)/memory.c
OBJS = $(OBJDIR)/main.o $(OBJDIR)/decoder.o $(OBJDIR)/memory.o

TEST_SRCS = $(TESTDIR)/test_decoder.c $(SRCDIR)/decoder.c $(SRCDIR)/memory.c

# ─── Output binaries ──────────────────────────────────────────────
TARGET      = $(BINDIR)/riscv-decoder
TEST_TARGET = $(BINDIR)/test_decoder

# ─── Phony targets (not real files) ───────────────────────────────
.PHONY: all clean test debug valgrind

# ─── Default target: build the main executable ────────────────────
all: makedirs $(TARGET)
	@echo "Build complete: $(TARGET)"

makedirs:
	mkdir -p $(BINDIR) $(OBJDIR)

# ─── Link object files into final executable ──────────────────────
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(RELFLAGS) -o $@ $^

# ─── Compile each .c into a .o ────────────────────────────────────
$(OBJDIR)/main.o: $(SRCDIR)/main.c
	$(CC) $(CFLAGS) $(RELFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/decoder.o: $(SRCDIR)/decoder.c
	$(CC) $(CFLAGS) $(RELFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/memory.o: $(SRCDIR)/memory.c
	$(CC) $(CFLAGS) $(RELFLAGS) -I$(INCDIR) -c $< -o $@

# ─── test: build and run unit tests + integration tests ───────────
test: makedirs $(TEST_TARGET) $(TARGET)
	@echo "=== Unit Tests ==="
	./$(TEST_TARGET)
	@echo ""
	@echo "=== Integration: mixed.hex ==="
	./$(TARGET) $(TESTDIR)/programs/mixed.hex
	@echo ""
	@echo "=== Integration: r_type.hex ==="
	./$(TARGET) $(TESTDIR)/programs/r_type.hex
	@echo ""
	@echo "=== Integration: i_type.hex ==="
	./$(TARGET) $(TESTDIR)/programs/i_type.hex
	@echo ""
	@echo "=== Integration: branch.hex ==="
	./$(TARGET) $(TESTDIR)/programs/branch.hex

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $^

# ─── debug: build with symbols for GDB ────────────────────────────
debug: makedirs
	$(CC) $(CFLAGS) $(DBGFLAGS) -I$(INCDIR) $(SRCS) -o $(BINDIR)/riscv-decoder-dbg
	@echo "Debug build ready: $(BINDIR)/riscv-decoder-dbg"

# ─── valgrind: memory leak check ──────────────────────────────────
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --error-exitcode=1 \
	         ./$(TARGET) $(TESTDIR)/programs/mixed.hex

# ─── clean: remove all generated files ────────────────────────────
clean:
	rm -rf $(BINDIR) $(OBJDIR)
	@echo "Cleaned."
