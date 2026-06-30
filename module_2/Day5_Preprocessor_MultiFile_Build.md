# Day 5 — Preprocessor, Multi-File Projects & Build Systems

**MEDS Module 2 | C Language for Hardware Engineers**
**Ali Hassan | Roll No. 2024-EE-176 | UET Lahore**

---

## What Is This Day About?

Days 1–4 covered the language itself. Day 5 teaches you how to **organise a real project** — the kind of thing that gets pushed to GitHub, reviewed by a mentor, and grows over months.

Three pillars:

| Pillar              | What It Solves                                                    |
|---------------------|-------------------------------------------------------------------|
| **C Preprocessor**  | Avoid magic numbers; enable/disable code at compile time         |
| **Multi-File Organisation** | Split code into modules; each file has one responsibility |
| **Makefile**        | Automate building, testing, cleaning — never type `gcc` manually |

---

## 8.1 The C Preprocessor

The preprocessor runs **before** the compiler sees your code. It does pure text substitution — it doesn't understand C, it just transforms text. Lines starting with `#` are preprocessor directives.

### Constants — Never Use Magic Numbers

```c
/* BAD — magic numbers scattered through the code.
   If you need to change 32 to 64, you have to find every occurrence manually. */
uint32_t regs[32];         /* Why 32? */
if (count > 65536) { ... } /* Why 65536? */
cpu->pc = 0x00000000;      /* What is this? */

/* GOOD — named constants defined in one place. */
#define NUM_REGISTERS  32          /* RISC-V RV32I has exactly 32 GPRs          */
#define MEMORY_SIZE    65536       /* 64 KB = 65536 bytes                        */
#define PC_START       0x00000000  /* RISC-V reset vector — first instruction    */
#define OPCODE_MASK    0x7F        /* 7-bit mask: 0b01111111 extracts bits [6:0] */

/* Now the code is self-documenting: */
uint32_t regs[NUM_REGISTERS];
if (count > MEMORY_SIZE) { ... }
cpu->pc = PC_START;
```

The preprocessor replaces every occurrence of `NUM_REGISTERS` with `32` before compilation. The compiler never sees the name — it sees the number. But you get to write readable code.

### Function-Like Macros

```c
/* EXTRACT_BITS(val, high, low) — isolate bits [high:low] and right-align.
   This looks like a function call, but the preprocessor pastes the code inline.
   Key rule: wrap EVERY parameter in parentheses to prevent operator precedence bugs.

   Without parentheses: EXTRACT_BITS(a+b, 6, 0) would expand to
     ((a+b >> 0) & ...) — a+b is computed AFTER the shift, wrong result!
   With parentheses: (((a+b) >> (0)) & ...) — correct.                       */
#define EXTRACT_BITS(val, high, low) \
    (((val) >> (low)) & ((1U << ((high) - (low) + 1)) - 1))
/*    ^^^^ shift right by 'low' to bring target bits to position 0
                       ^^^ bit mask of exactly (high-low+1) ones             */

/* MAX and MIN macros — note the ternary operator '? :' */
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
/*   ternary: if (a > b) result is a, else result is b                       */
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

/* Usage */
uint32_t funct3 = EXTRACT_BITS(0x00500113, 14, 12);   /* Should be 0 (ADDI) */
int bigger = MAX(10, 20);   /* becomes: ((10) > (20) ? (10) : (20)) = 20   */
```

### The Debug Logging Macro — Conditional Compilation

```c
/* DEBUG macro — prints file name, line number, and a custom message.
   'ifdef DEBUG' means: "only include this code if DEBUG is defined".
   In release builds (no -DDEBUG), LOG() expands to nothing — zero cost.    */

#ifdef DEBUG   /* Is the symbol 'DEBUG' defined? (pass -DDEBUG to compiler)  */

#define LOG(fmt, ...) \
    fprintf(stderr,                   /* Print to stderr, not stdout         */ \
            "[%s:%d] " fmt "\n",      /* Format: [filename:linenum] message  */ \
            __FILE__,                 /* Built-in: expands to "decoder.c"    */ \
            __LINE__,                 /* Built-in: expands to current line   */ \
            ##__VA_ARGS__)            /* '...' captures any extra arguments;
                                         ##__VA_ARGS__ handles zero extra args */

#else   /* No DEBUG defined — release build */

#define LOG(fmt, ...)   /* Expands to nothing — LOG calls cost 0 cycles */

#endif  /* End of the ifdef block */

/* Usage — works the same in debug and release builds: */
LOG("Decoding instruction 0x%08X at PC=0x%08X", raw, pc);
/* Debug build:   [decoder.c:47] Decoding instruction 0x00500113 at PC=0x00000000
   Release build: (nothing printed — the line literally doesn't exist in the binary) */
```

**How to enable debug logging:**
```bash
gcc -DDEBUG -g -O0 -o decoder_debug decoder.c   # Define the DEBUG symbol
gcc           -O2 -o decoder         decoder.c   # No DEBUG = release build
```

### Conditional Compilation for RV32 vs RV64

```c
/* Support both 32-bit and 64-bit RISC-V without duplicate code.
   The same source file compiles differently depending on what you pass to gcc. */

#ifdef RV64   /* If compiled with 'gcc -DRV64 ...' */

    typedef uint64_t reg_t;         /* 64-bit register type for RV64        */
    #define REG_FMT  "0x%016lX"    /* 16-digit hex — shows all 64 bits      */
    #define XLEN     64             /* Register width in bits                */

#else   /* Default: RV32 (no -DRV64 flag) */

    typedef uint32_t reg_t;         /* 32-bit register type for RV32        */
    #define REG_FMT  "0x%08X"      /* 8-digit hex — shows all 32 bits       */
    #define XLEN     32             /* Register width in bits                */

#endif

/* Now this code works for both RV32 and RV64: */
reg_t register_value = 0;   /* uint32_t or uint64_t depending on compilation */
printf("Value: " REG_FMT "\n", register_value);   /* Correct format for width */
```

```bash
gcc -DRV64 -o simulator_64 main.c   # Compile as 64-bit simulator
gcc        -o simulator_32 main.c   # Compile as 32-bit simulator (default)
```

---

## 8.2 Header Files and Include Guards

### Why Header Files Exist

Without headers, every `.c` file that wants to call `decode_instruction()` would need to know its full signature. If you change the signature, you update it in 10 places.

With a header:
- The function signature lives in **one place** (`decoder.h`)
- Every `.c` file that needs it does `#include "decoder.h"`
- Change the signature once → all files see the change

### The Full Header File Pattern

```c
/* ===== cpu.h ===== */

/* INCLUDE GUARD — prevents this header from being processed twice.
   Without this, if two files both include cpu.h, the compiler sees
   typedef cpu_state_t twice and throws a "redefinition" error.

   How it works:
   First inclusion:
     #ifndef CPU_H → symbol not defined yet → proceed
     #define CPU_H → define the symbol as a flag
     ... all the declarations ...
     #endif
   Second inclusion:
     #ifndef CPU_H → symbol IS defined → SKIP EVERYTHING until #endif
                                                                           */
#ifndef CPU_H
#define CPU_H

/* Standard library includes that this header needs.
   Put them here so any file that includes cpu.h automatically gets them.   */
#include <stdint.h>   /* uint32_t, uint8_t, etc.                            */
#include <stddef.h>   /* size_t                                              */

/* ── Type definitions — visible to everyone who includes cpu.h ─────────── */

typedef struct {
    uint32_t x[32];    /* 32 general-purpose registers                       */
    uint32_t pc;       /* Program counter                                    */
    uint8_t *memory;   /* Pointer to simulated RAM (allocated in cpu_init)   */
    size_t   mem_size; /* Size of simulated RAM in bytes                     */
} cpu_state_t;

/* ── Function DECLARATIONS (prototypes) — NOT definitions ──────────────── */
/* These tell the compiler: "these functions exist and have these signatures".
   The actual code (definitions) is in cpu.c.
   Without these prototypes, the compiler would warn about unknown functions
   when you call them from other files.                                      */

void     cpu_init(cpu_state_t *cpu, size_t mem_size);   /* Setup + allocate */
void     cpu_destroy(cpu_state_t *cpu);                 /* Free memory       */
uint32_t cpu_fetch(cpu_state_t *cpu);                   /* Fetch next instr  */
int      cpu_execute(cpu_state_t *cpu, uint32_t instr); /* Execute one instr */
void     cpu_dump_regs(const cpu_state_t *cpu);         /* Print all regs    */

#endif /* CPU_H */   /* Always put a comment here so you know what you're ending */
```

```c
/* ===== cpu.c ===== */

/* The .c file includes its own header FIRST.
   This is a deliberate technique: if cpu.h has a bug that makes it self-
   inconsistent, including it first in cpu.c catches the error immediately. */
#include "cpu.h"     /* Our own header — cpu_state_t, prototypes             */
#include <stdio.h>   /* printf, fprintf                                       */
#include <stdlib.h>  /* calloc, free                                          */
#include <string.h>  /* memset                                                */

/* Function DEFINITIONS — the actual code.
   These match the prototypes declared in cpu.h.                              */

void cpu_init(cpu_state_t *cpu, size_t mem_size)
{
    /* Zero all registers using memset — same as writing x[0]=x[1]=...=0    */
    memset(cpu->x, 0, sizeof(cpu->x));
    cpu->pc = 0;   /* Reset program counter to 0                             */

    /* calloc: allocate mem_size bytes, all initialised to zero.
       If NULL is returned, allocation failed (out of memory).               */
    cpu->memory = calloc(mem_size, 1);
    if (!cpu->memory) {
        fprintf(stderr, "Failed to allocate %zu bytes\n", mem_size);
        exit(EXIT_FAILURE);   /* Fatal — can't continue without memory       */
    }
    cpu->mem_size = mem_size;
}

void cpu_destroy(cpu_state_t *cpu)
{
    free(cpu->memory);    /* Return memory to OS — Valgrind needs this!      */
    cpu->memory = NULL;   /* Prevent dangling pointer                        */
}
```

---

## 8.3 Multi-File Project Organisation

### The Principle: One Module = One Responsibility

```
riscv-decoder/
│
├── include/            ← Header files: the "contracts" between modules
│   ├── common.h        ← Shared macros (#define EXTRACT_BITS), types, constants
│   ├── cpu.h           ← CPU state struct + function prototypes
│   ├── decoder.h       ← DecodedInstr struct + decode function prototypes
│   └── memory.h        ← Memory struct + load/read function prototypes
│
├── src/                ← Source files: the "implementations" of each module
│   ├── main.c          ← Entry point ONLY — argument parsing, top-level loop
│   ├── cpu.c           ← Implements cpu.h — cpu_init, cpu_fetch, cpu_execute
│   ├── decoder.c       ← Implements decoder.h — all bit manipulation
│   └── memory.c        ← Implements memory.h — file I/O, byte array access
│
├── test/               ← Tests live separately from source
│   ├── test_decoder.c  ← Unit tests for decoder functions
│   └── test_programs/  ← Hex files used by tests
│       ├── add.hex
│       └── branch.hex
│
├── docs/               ← Documentation
│   └── DESIGN.md       ← Why decisions were made (not what — the code shows what)
│
└── Makefile            ← Build automation
```

**Why separate include/ from src/?**
Headers are the public interface. Source files are the private implementation. Someone using your decoder module only needs to read `decoder.h`. The `.c` file is an implementation detail they shouldn't need to read.

**Why is main.c separate?**
`main.c` only orchestrates — it calls other modules. If you want to use the decoder as a library in another project, you include `decoder.h` and link `decoder.o`. `main.c` is not needed.

### How Separate Compilation Works

```
Source files     Compilation (gcc -c)    Object files     Linking (gcc -o)
──────────────   ────────────────────    ────────────     ───────────────

main.c      ──►  gcc -c main.c      ──►  main.o     ──┐
decoder.c   ──►  gcc -c decoder.c   ──►  decoder.o  ──┼──► riscv-decoder
memory.c    ──►  gcc -c memory.c    ──►  memory.o   ──┘

Each .c file is compiled INDEPENDENTLY into a .o (object) file.
The linker combines all .o files into one executable.
If you change decoder.c, only decoder.o is recompiled — not main.o or memory.o.
This is why large projects don't recompile everything when you change one file.
```

---

## 8.4 Makefile — Automated Build System

The Makefile tells `make` how to build the project. `make` is smart: it only rebuilds files that have changed (based on timestamps). You never type `gcc` manually.

```makefile
# ── Variables ──────────────────────────────────────────────────────────────
CC      = gcc           # Which compiler to use (could also be 'clang')
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
# -Wall:    enable all standard warnings (catch common mistakes)
# -Wextra:  enable extra warnings beyond -Wall
# -std=c11: compile as C11 (allows C99 features + a few extras)
# -g:       embed debug info for GDB
# -Iinclude: search the 'include/' directory for header files

LDFLAGS =   # Linker flags (empty here; could include -lm for math library)

# ── Directory variables ─────────────────────────────────────────────────────
SRC_DIR  = src       # Where .c source files live
OBJ_DIR  = build     # Where .o compiled files go (keeps src/ clean)
BIN_DIR  = bin       # Where finished executables go
TEST_DIR = test      # Where tests live

# ── Automatic file lists ────────────────────────────────────────────────────
# $(wildcard pattern) returns all files matching the pattern
SRC = $(wildcard $(SRC_DIR)/*.c)
# Expands to: src/main.c src/decoder.c src/memory.c

# String substitution: replace 'src/%.c' with 'build/%.o' for each file
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
# Expands to: build/main.o build/decoder.o build/memory.o

TARGET = $(BIN_DIR)/riscv-decoder   # The final executable

# ── .PHONY — targets that are command names, not real files ─────────────────
# Without .PHONY, 'make clean' would do nothing if a file named 'clean' existed
.PHONY: all clean test debug release help

# ── Default target (first target = what 'make' runs with no argument) ───────
all: dirs $(TARGET)
# 'all' depends on 'dirs' (create directories) and $(TARGET) (build executable)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
# '@' before a command suppresses echoing the command itself (cleaner output)
# '-p' means: create parent directories and don't error if already exists

# ── Linking rule — combine all .o files into the executable ─────────────────
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
# $@ = the target (bin/riscv-decoder)
# $^ = all prerequisites (build/main.o build/decoder.o build/memory.o)
# So this expands to: gcc  -o bin/riscv-decoder build/main.o build/decoder.o build/memory.o

# ── Pattern rule — compile any .c in src/ to a .o in build/ ─────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
# %    = wildcard (matches 'main', 'decoder', 'memory')
# $<   = first prerequisite (src/main.c, src/decoder.c, etc.)
# $@   = target file (build/main.o, build/decoder.o, etc.)
# -c   = compile only, don't link (produces .o, not an executable)

# ── Automatic dependency tracking ───────────────────────────────────────────
# This generates .d files listing what .h files each .c depends on.
# If common.h changes, make knows to recompile anything that includes it.
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@
# -MM: list dependencies (all #include'd files), not compile
# -MT: change the target name in the dependency list to be in OBJ_DIR

# Include the auto-generated dependency files (if they exist)
-include $(OBJ:.o=.d)
# The '-' before 'include' means: don't error if the files don't exist yet

# ── debug target ─────────────────────────────────────────────────────────────
debug: CFLAGS += -DDEBUG -O0   # Add -DDEBUG and disable optimisation
debug: all                      # Then run the 'all' target normally

# ── release target ────────────────────────────────────────────────────────────
release: CFLAGS += -O2 -DNDEBUG   # Maximum optimisation, disable asserts
release: all

# ── test target ───────────────────────────────────────────────────────────────
test: all
	./$(TARGET) $(TEST_DIR)/test_programs/add.hex

# ── valgrind target ───────────────────────────────────────────────────────────
valgrind: all
	valgrind --leak-check=full ./$(TARGET) $(TEST_DIR)/test_programs/add.hex

# ── clean target ──────────────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
# -rf: recursive (remove directories too), force (no error if not found)

# ── help target ───────────────────────────────────────────────────────────────
help:
	@echo "Targets: all, debug, release, test, valgrind, clean, help"
	@echo "  make           — build in default mode"
	@echo "  make debug     — build with DEBUG macro and no optimisation"
	@echo "  make release   — build with -O2 optimisation"
	@echo "  make test      — build and run tests"
	@echo "  make valgrind  — run under Valgrind"
	@echo "  make clean     — delete all build artifacts"
```

---

## Day 5 Exercises — Fully Solved

---

### Exercise 1: Split a Single-File Decoder into 3 Files

**Task:** Split a decoder into `main.c`, `decoder.c`, `decoder.h`. Write a Makefile. Verify it builds.

**`include/decoder.h`:**
```c
/* include/decoder.h — Public interface for the RISC-V instruction decoder */

#ifndef DECODER_H   /* Include guard — prevent double inclusion              */
#define DECODER_H

#include <stdint.h>   /* uint32_t, int32_t — exact-width integers            */

/* Bit extraction macro — used in decoder.c but declared here
   so any file that includes decoder.h can also use it                       */
#define EXTRACT_BITS(val, hi, lo) \
    (((uint32_t)(val) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))

/* Sign extension macro */
#define SIGN_EXTEND(val, bits) \
    (((int32_t)((val) << (32 - (bits)))) >> (32 - (bits)))

/* Decoded instruction struct — all fields extracted from one 32-bit word    */
typedef struct {
    uint32_t raw;          /* Original 32-bit machine word (unchanged)       */
    uint32_t pc;           /* Address of this instruction                    */
    uint32_t opcode;       /* Bits [6:0]   — instruction family              */
    uint32_t rd;           /* Bits [11:7]  — destination register (0–31)    */
    uint32_t funct3;       /* Bits [14:12] — function code                  */
    uint32_t rs1;          /* Bits [19:15] — source register 1 (0–31)       */
    uint32_t rs2;          /* Bits [24:20] — source register 2 (0–31)       */
    uint32_t funct7;       /* Bits [31:25] — extended function code          */
    int32_t  imm;          /* Sign-extended immediate value                  */
    char     mnemonic[32]; /* Human-readable string: "addi x2, x0, 5"       */
    int      valid;        /* 1 = decoded successfully, 0 = UNKNOWN          */
} decoded_instr_t;

/* Function prototypes — implemented in decoder.c                            */
int         decode_instruction(uint32_t raw, uint32_t pc, decoded_instr_t *out);
void        print_instruction(const decoded_instr_t *instr);
void        print_header(void);
const char *reg_name(uint32_t reg);

#endif /* DECODER_H */
```

**`src/decoder.c`:**
```c
/* src/decoder.c — Instruction decoding implementation */

#include "../include/decoder.h"   /* Our interface — must be first           */
#include <stdio.h>                /* snprintf, printf                         */
#include <string.h>               /* memset                                   */

/* RV32I opcode constants */
#define OP_REG    0x33   /* R-type: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA  */
#define OP_IMM    0x13   /* I-type: ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI */
#define OP_LOAD   0x03   /* Load: LB, LH, LW, LBU, LHU                      */
#define OP_STORE  0x23   /* Store: SB, SH, SW                                */
#define OP_BRANCH 0x63   /* Branch: BEQ, BNE, BLT, BGE, BLTU, BGEU         */
#define OP_LUI    0x37   /* LUI                                               */
#define OP_AUIPC  0x17   /* AUIPC                                             */
#define OP_JAL    0x6F   /* JAL                                               */
#define OP_JALR   0x67   /* JALR                                              */

/* Register name table — 'static' = private to this file */
static const char *reg_names[32] = {
    "x0","x1","x2","x3","x4","x5","x6","x7",
    "x8","x9","x10","x11","x12","x13","x14","x15",
    "x16","x17","x18","x19","x20","x21","x22","x23",
    "x24","x25","x26","x27","x28","x29","x30","x31"
};

/* reg_name — return register name string for index 0–31 */
const char *reg_name(uint32_t reg)
{
    if (reg >= 32) return "??";
    return reg_names[reg];
}

/* decode_instruction — extract all fields from raw 32-bit instruction.
   Returns 0 (SUCCESS) if known, -1 (FAILURE) if unknown opcode.           */
int decode_instruction(uint32_t raw, uint32_t pc, decoded_instr_t *out)
{
    /* Clear all fields — no leftover garbage from previous call             */
    memset(out, 0, sizeof(decoded_instr_t));
    out->raw   = raw;
    out->pc    = pc;
    out->valid = 0;   /* Assume unknown until proven otherwise               */

    /* Extract fields common to all formats */
    out->opcode = EXTRACT_BITS(raw,  6,  0);   /* 7-bit opcode              */
    out->rd     = EXTRACT_BITS(raw, 11,  7);   /* 5-bit destination reg     */
    out->funct3 = EXTRACT_BITS(raw, 14, 12);   /* 3-bit function code       */
    out->rs1    = EXTRACT_BITS(raw, 19, 15);   /* 5-bit source reg 1        */
    out->rs2    = EXTRACT_BITS(raw, 24, 20);   /* 5-bit source reg 2        */
    out->funct7 = EXTRACT_BITS(raw, 31, 25);   /* 7-bit extended code       */

    /* Dispatch on opcode */
    switch (out->opcode) {

        case OP_REG: {   /* R-type: rd = rs1 OP rs2                          */
            uint32_t f3 = out->funct3;
            uint32_t f7 = out->funct7;
            const char *op = "?";
            if      (f3==0 && f7==0x00) op = "add";
            else if (f3==0 && f7==0x20) op = "sub";
            else if (f3==1)             op = "sll";
            else if (f3==2)             op = "slt";
            else if (f3==3)             op = "sltu";
            else if (f3==4)             op = "xor";
            else if (f3==5 && f7==0x00) op = "srl";
            else if (f3==5 && f7==0x20) op = "sra";
            else if (f3==6)             op = "or";
            else if (f3==7)             op = "and";
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "%s %s, %s, %s", op,
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            out->valid = 1;
            break;
        }

        case OP_IMM: {   /* I-type arithmetic: rd = rs1 OP imm               */
            uint32_t raw_imm = EXTRACT_BITS(raw, 31, 20);
            out->imm = SIGN_EXTEND(raw_imm, 12);   /* 12-bit signed imm     */
            const char *op = "?";
            switch (out->funct3) {
                case 0: op = "addi";  break;
                case 2: op = "slti";  break;
                case 3: op = "sltiu"; break;
                case 4: op = "xori";  break;
                case 6: op = "ori";   break;
                case 7: op = "andi";  break;
                case 1: op = "slli";  break;
                case 5: op = EXTRACT_BITS(raw,30,30) ? "srai" : "srli"; break;
            }
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "%s %s, %s, %d", op,
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            out->valid = 1;
            break;
        }

        case OP_LOAD: {   /* Load: rd = mem[rs1 + imm]                        */
            uint32_t raw_imm = EXTRACT_BITS(raw, 31, 20);
            out->imm = SIGN_EXTEND(raw_imm, 12);
            const char *op = "?";
            switch (out->funct3) {
                case 0: op = "lb";  break;
                case 1: op = "lh";  break;
                case 2: op = "lw";  break;
                case 4: op = "lbu"; break;
                case 5: op = "lhu"; break;
            }
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "%s %s, %d(%s)", op,
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            out->valid = 1;
            break;
        }

        case OP_STORE: {   /* Store: mem[rs1+imm] = rs2                       */
            uint32_t hi = EXTRACT_BITS(raw, 31, 25);   /* imm[11:5]          */
            uint32_t lo = EXTRACT_BITS(raw, 11,  7);   /* imm[4:0]           */
            out->imm = SIGN_EXTEND((hi << 5) | lo, 12);
            const char *op = "?";
            switch (out->funct3) {
                case 0: op = "sb"; break;
                case 1: op = "sh"; break;
                case 2: op = "sw"; break;
            }
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "%s %s, %d(%s)", op,
                     reg_name(out->rs2), out->imm, reg_name(out->rs1));
            out->valid = 1;
            break;
        }

        case OP_LUI: {   /* LUI rd, imm[31:12]                                */
            out->imm = (int32_t)(raw & 0xFFFFF000u);
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lui %s, %d", reg_name(out->rd), out->imm >> 12);
            out->valid = 1;
            break;
        }

        default:
            snprintf(out->mnemonic, sizeof(out->mnemonic), "UNKNOWN");
            out->valid = 0;
            return -1;   /* FAILURE                                           */
    }

    return 0;   /* SUCCESS                                                    */
}

/* print_header — prints the table header row */
void print_header(void)
{
    printf("%-10s %-10s  %s\n", "Addr", "Hex", "Assembly");
    printf("---------- ----------  -------------------------\n");
}

/* print_instruction — prints one decoded instruction row */
void print_instruction(const decoded_instr_t *instr)
{
    printf("0x%08X %08X  %s\n", instr->pc, instr->raw, instr->mnemonic);
}
```

**`src/main.c`:**
```c
/* src/main.c — Entry point: argument parsing and decode loop */

#include "../include/decoder.h"   /* decoded_instr_t, decode_instruction     */
#include <stdio.h>                /* printf, fprintf                          */
#include <stdlib.h>               /* EXIT_FAILURE, EXIT_SUCCESS               */

/* Simple hex loader — reads one word per line from a text file              */
static int load_hex(const char *filename, uint32_t *buf, int max)
{
    FILE *fp = fopen(filename, "r");   /* Open for reading                   */
    if (!fp) { perror(filename); return -1; }

    char line[32];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max) {
        if (line[0] == '#' || line[0] == '\n') continue;   /* Skip comments */
        /* sscanf parses hex string; returns 1 if successful                 */
        if (sscanf(line, "%x", &buf[count]) == 1) {
            count++;
        }
    }
    fclose(fp);
    return count;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hexfile>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t instructions[4096];   /* Stack buffer for up to 4096 instructions */
    int count = load_hex(argv[1], instructions, 4096);
    if (count < 0) return EXIT_FAILURE;

    printf("RISC-V Decoder\n==============\n");
    printf("Loaded %d instructions from '%s'\n\n", count, argv[1]);
    print_header();

    int     i;
    int     valid = 0, unknown = 0;
    decoded_instr_t instr;   /* One struct reused per iteration              */

    for (i = 0; i < count; i++) {
        uint32_t pc = (uint32_t)(i * 4);   /* PC = word_index * 4 bytes     */
        decode_instruction(instructions[i], pc, &instr);
        print_instruction(&instr);
        if (instr.valid) valid++; else unknown++;
    }

    printf("\nDecoded %d instructions (%d valid, %d unknown)\n",
           count, valid, unknown);
    return EXIT_SUCCESS;
}
```

**`Makefile`:**
```makefile
# Makefile for Exercise 1 — multi-file decoder

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinclude

.PHONY: all clean

all: bin/decoder

bin/decoder: build/main.o build/decoder.o
	@mkdir -p bin
	$(CC) -o $@ $^
	@echo "Built: $@"

build/main.o: src/main.c include/decoder.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/decoder.o: src/decoder.c include/decoder.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build/ bin/
	@echo "Cleaned."
```

**Build and run:**
```bash
make                                       # Compile everything
./bin/decoder test/programs/mixed.hex      # Run with a hex file
make clean                                 # Remove build artifacts
```

---

### Exercise 2: Conditional Compilation with Debug Macro

**Task:** Add `#ifdef RV64` support and a debug `LOG` macro enabled by `-DDEBUG`.

```c
/* exercise2_day5.c — Conditional compilation for RV32/RV64 and debug logging */

#include <stdio.h>    /* printf, fprintf                                      */
#include <stdint.h>   /* uint32_t, uint64_t                                  */

/* ── Conditional type selection ─────────────────────────────────────────── */
#ifdef RV64
    /* Compiled with: gcc -DRV64 ... */
    typedef uint64_t xlen_t;       /* Register type = 64-bit unsigned        */
    #define REG_FMT   "0x%016llX"  /* Format spec for 64-bit hex printing   */
    #define XLEN      64           /* Bit width of general-purpose registers  */
    #define MAX_IMMED 2147483647   /* 2^31 - 1 (32-bit sign-extended immed)  */
#else
    /* Default: RV32 (no flag needed) */
    typedef uint32_t xlen_t;       /* Register type = 32-bit unsigned        */
    #define REG_FMT   "0x%08X"     /* Format spec for 32-bit hex printing   */
    #define XLEN      32           /* Bit width of general-purpose registers  */
    #define MAX_IMMED 2047         /* 2^11 - 1 (12-bit immediate max value)  */
#endif

/* ── Debug logging macro ─────────────────────────────────────────────────── */
#ifdef DEBUG
    /* Debug build: LOG prints file, line number, and message to stderr.
       __FILE__ and __LINE__ are built-in macros substituted by the compiler. */
    #define LOG(fmt, ...) \
        fprintf(stderr, "[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    /* Release build: LOG expands to nothing — zero cost, zero output.        */
    #define LOG(fmt, ...)   /* empty — compiler removes all LOG calls        */
#endif

/* ── NDEBUG / assert ─────────────────────────────────────────────────────── */
/* When compiled with -DNDEBUG (release mode), assert() becomes a no-op.
   In debug mode, assert crashes the program with a message if the condition
   is false — catches programming errors early.                               */
#include <assert.h>
/* Usage: assert(reg_index < 32);   -- crashes with message if false         */

/* Simulated register file using the conditional type */
static xlen_t registers[32];   /* 32 registers, each XLEN bits wide         */

void reg_write(int rd, xlen_t value)
{
    /* LOG only prints when compiled with -DDEBUG */
    LOG("reg_write: x%d = " REG_FMT, rd, (unsigned long long)value);

    assert(rd >= 0 && rd < 32);   /* Catches bad index in debug builds       */
    if (rd == 0) {
        LOG("  -> x0 is hardwired to 0, write ignored");
        return;   /* x0 is always 0 — RISC-V specification requirement       */
    }
    registers[rd] = value;
    LOG("  -> x%d now = " REG_FMT, rd, (unsigned long long)registers[rd]);
}

xlen_t reg_read(int rs)
{
    assert(rs >= 0 && rs < 32);
    LOG("reg_read: x%d = " REG_FMT, rs, (unsigned long long)registers[rs]);
    return registers[rs];
}

int main(void)
{
    int i;

    printf("=== RISC-V Simulator Configuration ===\n");
    printf("Architecture: RV%d\n", XLEN);
    printf("Register width: %d bits\n", XLEN);
    printf("Max 12-bit immediate: %d\n", MAX_IMMED);
    printf("Debug logging: %s\n",
           #ifdef DEBUG
               "ON (output goes to stderr)"
           #else
               "OFF (compiled out)"
           #endif
    );

    /* Initialise registers to 0 */
    for (i = 0; i < 32; i++) {
        registers[i] = 0;
    }

    /* Test register reads and writes */
    reg_write(1, (xlen_t)0x400);     /* x1 (ra) = 0x400                     */
    reg_write(2, (xlen_t)0x7FFFFC);  /* x2 (sp) = stack pointer             */
    reg_write(10, (xlen_t)42);       /* x10 (a0) = 42 (return value)        */
    reg_write(0, (xlen_t)999);       /* x0: should be silently ignored       */

    printf("\nRegister values:\n");
    printf("x0  = " REG_FMT " (should be 0)\n",  (unsigned long long)reg_read(0));
    printf("x1  = " REG_FMT "\n",                 (unsigned long long)reg_read(1));
    printf("x2  = " REG_FMT "\n",                 (unsigned long long)reg_read(2));
    printf("x10 = " REG_FMT "\n",                 (unsigned long long)reg_read(10));

    return 0;
}
```

**How to compile:**
```bash
# RV32, no debug output:
gcc -std=c99 -o sim32        exercise2_day5.c

# RV32, with debug output:
gcc -std=c99 -DDEBUG -g -O0 -o sim32_debug  exercise2_day5.c

# RV64, no debug:
gcc -std=c99 -DRV64          -o sim64        exercise2_day5.c

# RV64, with debug:
gcc -std=c99 -DRV64 -DDEBUG -g -O0 -o sim64_debug exercise2_day5.c
```

```bash
./sim32        # Silent output
./sim32_debug  # Debug lines appear on stderr
```

---

### Exercise 3: Include Guards — Test Double Inclusion

**Task:** Create headers with proper include guards. Verify that including the same header twice causes no errors.

```c
/* include/types.h — Custom type definitions */

#ifndef TYPES_H   /* First inclusion: TYPES_H not yet defined, so proceed   */
#define TYPES_H   /* Define TYPES_H as a flag                                */

#include <stdint.h>   /* uint32_t                                            */

/* Custom type aliases for this project */
typedef uint32_t word_t;    /* A 32-bit machine word                         */
typedef uint32_t addr_t;    /* A 32-bit memory address                       */
typedef uint8_t  byte_t;    /* A single byte                                 */

#define WORD_SIZE  4     /* Bytes per word                                   */
#define ADDR_BITS 32     /* Width of address bus in bits                     */

#endif /* TYPES_H */   /* Second inclusion: TYPES_H IS defined, skip to here */
```

```c
/* include/constants.h — Project-wide constants */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "types.h"   /* Pulls in word_t, addr_t — safe even if already included
                        because types.h has its own include guard             */

#define MAX_INSTRUCTIONS  4096        /* Max instructions in memory          */
#define RESET_VECTOR      0x00000000u /* PC on power-up or reset             */
#define STACK_TOP         0x7FFFFFFFu /* Default initial stack pointer       */

#endif /* CONSTANTS_H */
```

```c
/* exercise3_day5.c — Demonstrates that double-including headers is safe */

/* Include the same header TWICE — without include guards, this would cause:
   "error: redefinition of typedef 'word_t'"
   With include guards, the second inclusion is silently skipped.            */
#include "../include/types.h"
#include "../include/types.h"       /* Second inclusion — should cause NO error */

/* Include a header that internally includes another header */
#include "../include/constants.h"   /* This includes types.h internally      */
/* types.h is now "included" three times total — still no error              */

#include <stdio.h>

int main(void)
{
    /* All types and constants are available exactly once */
    word_t instruction = 0x003100B3;   /* A sample ADD instruction           */
    addr_t pc          = RESET_VECTOR; /* Start at reset vector              */

    printf("=== Include Guard Test ===\n");
    printf("sizeof(word_t)  = %zu bytes\n", sizeof(word_t));
    printf("sizeof(addr_t)  = %zu bytes\n", sizeof(addr_t));
    printf("sizeof(byte_t)  = %zu byte\n",  sizeof(byte_t));
    printf("WORD_SIZE       = %d\n",         WORD_SIZE);
    printf("MAX_INSTRUCTIONS= %d\n",         MAX_INSTRUCTIONS);
    printf("RESET_VECTOR    = 0x%08X\n",     RESET_VECTOR);
    printf("instruction     = 0x%08X\n",     instruction);
    printf("pc              = 0x%08X\n",     pc);

    printf("\nAll three inclusions of types.h compiled without errors.\n");
    printf("Include guards are working correctly.\n");

    return 0;
}
```

**Build and verify:**
```bash
gcc -Iinclude -o test3 exercise3_day5.c
# Should compile cleanly with NO redefinition errors
./test3
```

---

## Quick Reference: Preprocessor Symbols

| Directive          | Meaning                                                      |
|--------------------|--------------------------------------------------------------|
| `#define X val`    | Replace every occurrence of `X` with `val`                  |
| `#define X(a,b) …` | Function-like macro; wrap all args in `()`                   |
| `#include "file"`  | Copy contents of `file` here (your headers)                 |
| `#include <file>`  | Copy system header (stdio.h, stdint.h, etc.)                |
| `#ifndef GUARD`    | If `GUARD` is not defined, process until `#endif`           |
| `#ifdef DEBUG`     | If `DEBUG` is defined (via `-DDEBUG`), process until `#endif`|
| `#define GUARD`    | Define `GUARD` as a flag (no value needed for guards)       |
| `#endif`           | End a `#ifdef`/`#ifndef`/`#if` block                        |
| `__FILE__`         | Built-in: expands to the current source filename string     |
| `__LINE__`         | Built-in: expands to the current line number integer        |

---

## Quick Reference: Makefile Automatic Variables

| Variable | Meaning                                      | Example                                |
|----------|----------------------------------------------|----------------------------------------|
| `$@`     | The target of the current rule               | `bin/riscv-decoder`                   |
| `$^`     | All prerequisites (dependencies) of the rule | `build/main.o build/decoder.o`        |
| `$<`     | The first prerequisite only                  | `src/main.c`                          |
| `$*`     | The pattern `%` matched in a pattern rule    | `main` (from `build/main.o` pattern)  |

---

## Summary — What You Learned on Day 5

| Concept              | Key Point                                                            |
|----------------------|----------------------------------------------------------------------|
| `#define`            | Named constants and function-like macros — no magic numbers          |
| `#ifdef` / `#ifndef` | Enable code at compile time; `#ifdef DEBUG` = debug builds           |
| Include guard        | `#ifndef HEADER_H / #define HEADER_H / ... / #endif` in every header |
| Header file          | Declarations (prototypes, typedefs, macros) shared across `.c` files |
| Source file          | One module, one responsibility; include its own header first         |
| Separate compilation | Each `.c` → one `.o`; linker combines into executable               |
| Makefile `all`       | Default target; depends on all other targets                        |
| Makefile `clean`     | `rm -rf build/ bin/` — remove generated files                       |
| `$(wildcard ...)`    | Automatically find all `.c` files in a directory                    |
| `.PHONY`             | Declare targets that are command names, not file names              |
| `debug` target       | `CFLAGS += -DDEBUG -O0` then rebuild                                |
| `release` target     | `CFLAGS += -O2 -DNDEBUG` then rebuild                               |
| `__FILE__` `__LINE__`| Built-in macros for the filename and line number of current code    |

---

*Day 5 Complete — MEDS Module 2 | UET Lahore*

*All three days cover the complete foundation you need for the Grand Assignment:
Day 3 → data structures | Day 4 → memory and I/O | Day 5 → project organisation*
