# RISC-V RV32I Instruction Decoder — Complete Project Guide

**Author:** Ali Hassan | Roll No. 2024-EE-176 | MEDS Module 2, UET Lahore

---

## Table of Contents

1. [Big Picture — What This Project Does](#1-big-picture)
2. [Complete Workflow — Start to End](#2-complete-workflow)
3. [File-by-File Role Explanation](#3-file-by-file-role-explanation)
4. [Every Keyword Explained](#4-every-keyword-explained)
   - [C Language Keywords](#41-c-language-keywords)
   - [Preprocessor Keywords and Directives](#42-preprocessor-keywords-and-directives)
   - [Standard Library Functions](#43-standard-library-functions)
   - [Project Macros](#44-project-macros-defined-in-commonh)
   - [Project Enums](#45-project-enums-defined-in-decoderh)
   - [Project Structs](#46-project-structs)
   - [Project Functions](#47-project-functions)
   - [Makefile Keywords](#48-makefile-keywords)
   - [RISC-V Architecture Terms](#49-risc-v-architecture-terms)
5. [Data Flow Diagram](#5-data-flow-diagram)
6. [Bit Manipulation Deep Dive](#6-bit-manipulation-deep-dive)
7. [Sign Extension Deep Dive](#7-sign-extension-deep-dive)
8. [Git Workflow for Part C](#8-git-workflow-for-part-c)

---

## 1. Big Picture

The program takes a `.hex` file containing raw RISC-V machine code, reads it word by word, and for each 32-bit number it prints the human-readable assembly instruction it represents.

```
Input:   00500113           (raw hex in a text file)
Output:  0x00000000 00500113   addi x2, x0, 5
```

Think of it as a **translator**: binary machine code → human-readable assembly. This is exactly what a CPU's front-end (instruction fetch + decode stage) does in hardware. You are building the software version.

---

## 2. Complete Workflow — Start to End

### Step 0 — You type the command

```bash
./bin/riscv-decoder test/programs/mixed.hex
```

The shell finds the binary at `bin/riscv-decoder` and starts executing it, passing `mixed.hex` as a command-line argument.

---

### Step 1 — `main()` validates arguments

Inside `src/main.c`, the `main()` function runs first. It checks `argc` (argument count). If you didn't provide exactly one filename argument, it prints usage help and exits. If the argument exists, execution continues.

```
argc = 2
argv[0] = "./bin/riscv-decoder"   ← program name
argv[1] = "test/programs/mixed.hex"  ← your filename
```

---

### Step 2 — Memory initialisation (`mem_init`)

`main()` declares a `Memory` struct on the stack and calls `mem_init(&mem)`. This function (in `src/memory.c`) uses `memset` to fill the entire struct with zeros so no garbage values are left from whatever was previously at that memory address. It also sets `base_addr = 0x00000000`, meaning the first instruction lives at address zero.

---

### Step 3 — Hex file loading (`mem_load_hex`)

`mem_load_hex(&mem, argv[1])` opens the `.hex` file using `fopen`. It reads one line at a time with `fgets`. For each line:

- Lines starting with `#` or `//` are skipped (treated as comments).
- Blank lines are skipped.
- Lines with whitespace-only content are skipped.
- Valid lines are parsed with `sscanf(line, "%8x", &word)`, converting the 8-character hex string into a `uint32_t` integer.
- Each word is stored in `mem.data[count]` and `count` is incremented.

At the end, `mem.count` holds the total number of instructions loaded. The function returns this count.

---

### Step 4 — The decode loop

Back in `main()`, a `for` loop runs once per instruction. For each iteration:

1. **Calculate PC**: `pc = mem.base_addr + (i * WORD_SIZE)`. Since `WORD_SIZE = 4`, the addresses go `0x00, 0x04, 0x08, 0x0C, ...` — exactly how a real CPU counts instruction addresses.
2. **Read word**: `mem_read_word(&mem, i)` returns the raw `uint32_t` from `mem.data[i]`.
3. **Decode**: `decode_instruction(raw, pc, &instr)` is called. This is the main engine.
4. **Print**: `print_instruction(&instr)` prints one row.
5. **Count**: `instr.valid` is checked to update the valid/unknown counters.

---

### Step 5 — Inside `decode_instruction` (the heart)

This function in `src/decoder.c` does the following in order:

**A. Zero-initialise the output struct**
`memset(out, 0, sizeof(DecodedInstr))` clears the output struct so no stale data from a previous call leaks through.

**B. Extract all common fields**
Every RISC-V instruction shares the same bit positions for the major fields, regardless of instruction type. So these are extracted first for all instructions:

| Field    | Bits    | Extraction Call           |
|----------|---------|---------------------------|
| `opcode` | [6:0]   | `EXTRACT_BITS(raw, 6, 0)` |
| `rd`     | [11:7]  | `EXTRACT_BITS(raw, 11, 7)`|
| `funct3` | [14:12] | `EXTRACT_BITS(raw, 14, 12)`|
| `rs1`    | [19:15] | `EXTRACT_BITS(raw, 19, 15)`|
| `rs2`    | [24:20] | `EXTRACT_BITS(raw, 24, 20)`|
| `funct7` | [31:25] | `EXTRACT_BITS(raw, 31, 25)`|

**C. Dispatch using `switch(out->opcode)`**
The 7-bit opcode uniquely identifies the instruction format. A `switch` statement routes to the correct format-specific helper function:

```
opcode 0x03  →  decode_load()
opcode 0x13  →  decode_i_arith()
opcode 0x17  →  (AUIPC inline)
opcode 0x23  →  decode_store()
opcode 0x33  →  decode_r_type()
opcode 0x37  →  (LUI inline)
opcode 0x63  →  decode_branch()
opcode 0x67  →  (JALR inline)
opcode 0x6F  →  (JAL inline)
anything else → UNKNOWN
```

**D. Format-specific decoding**
Inside each helper (e.g., `decode_r_type`), the `funct3` (and sometimes `funct7`) fields are used in another `switch` to pick the exact instruction. The result is written as a human-readable string into `out->mnemonic` using `snprintf`.

**E. Immediate extraction**
Loads, stores, branches, and jumps carry an **immediate** — a constant value encoded inside the instruction bits. Each format scrambles the bits differently. Five helper functions (`imm_i`, `imm_s`, `imm_b`, `imm_u`, `imm_j`) reassemble and sign-extend the immediate correctly.

**F. Return result**
`SUCCESS (0)` if the instruction was recognised. `FAILURE (-1)` if the opcode was unknown, in which case `out->mnemonic` is set to `"UNKNOWN"`.

---

### Step 6 — Printing

`print_instruction` uses `printf` with `0x%08X %08X  %s\n` to print the PC (as zero-padded hex), the raw word, and the mnemonic string.

---

### Step 7 — Summary and exit

After the loop, `main()` prints the summary line:
```
Decoded 8 instructions (8 valid, 0 unknown)
```
Then returns `EXIT_SUCCESS` (which is `0`) to the shell, signalling that the program completed without errors.

---

## 3. File-by-File Role Explanation

```
riscv-decoder/
│
├── include/common.h    ← Shared foundation: macros, constants, #includes
│                          Every other file includes this first.
│
├── include/decoder.h   ← Decoder contract: enums, DecodedInstr struct,
│                          function prototypes for decoder.c
│
├── include/memory.h    ← Memory contract: Memory struct, function
│                          prototypes for memory.c
│
├── src/main.c          ← Program entry point. Owns the top-level workflow:
│                          parse args → init → load → loop(decode+print) → summary
│
├── src/memory.c        ← Implements memory.h. All file I/O lives here.
│                          Reads the .hex file, stores words in an array.
│
├── src/decoder.c       ← Implements decoder.h. All bit manipulation lives here.
│                          Extracts fields, builds mnemonic strings.
│
├── test/test_decoder.c ← Unit tests. Tests each instruction type independently
│                          using known hex→mnemonic pairs.
│
├── test/programs/      ← Four .hex test files covering every instruction format.
│
├── Makefile            ← Build automation. Defines how to compile, test, clean.
│
└── docs/DESIGN.md      ← Design rationale (why decisions were made).
```

**Why separate files?**
Each file has one responsibility. `memory.c` only handles I/O. `decoder.c` only handles bit manipulation. `main.c` only orchestrates. This is called **separation of concerns** — it makes the code easier to read, test, and modify.

---

## 4. Every Keyword Explained

---

### 4.1 C Language Keywords

#### `int`
The basic signed integer type. On most systems it is 32 bits. Used for return values, counters, and `main`'s return type. When you write `int decode_instruction(...)`, you are saying this function returns an integer (either `SUCCESS=0` or `FAILURE=-1`).

#### `void`
Means "nothing". `void mem_init(Memory *mem)` returns nothing. `void print_header(void)` takes no arguments. Used as a return type when the function performs an action but produces no value.

#### `char`
A single byte (8-bit) character. Used in two ways in this project:
- `char line[64]` — a buffer (array of bytes) to hold one text line read from the file.
- `char mnemonic[32]` — a string buffer inside `DecodedInstr` to hold the decoded text like `"addi x2, x0, 5"`.

#### `static`
Has two different meanings depending on where it appears:

1. **On a local variable** (`static const char *names[REG_COUNT]` inside `reg_name()`): the variable is created once and persists for the entire life of the program, not just the function call. Without `static`, `names` would be re-created on every call.

2. **On a function** (`static int decode_r_type(...)`, `static int32_t imm_i(...)`): the function is **private** to the file it is defined in. It cannot be called from any other `.c` file. This is good design — format-specific helpers are internal details that outsiders should not see.

#### `const`
Means the value cannot be changed after initialisation. `const char *filename` means the function promises not to modify the filename string. `const DecodedInstr *instr` in `print_instruction` means the function only reads the struct, never writes to it. `const` is good practice — it lets the compiler catch accidental modifications.

#### `typedef`
Creates a new name (alias) for an existing type. Instead of writing `struct DecodedInstr` everywhere, `typedef struct { ... } DecodedInstr` lets you just write `DecodedInstr`. Same for `typedef enum { ... } Opcode`. Makes code shorter and more readable.

#### `struct`
Groups multiple variables of different types together under one name. `DecodedInstr` is a struct with fields for the raw word, PC, opcode, registers, immediate, mnemonic string, and validity flag. A struct is accessed with `.` (direct) or `->` (through pointer).

#### `enum`
A list of named integer constants. `typedef enum { OP_LOAD = 0x03, OP_IMM = 0x13, ... } Opcode` means `OP_LOAD` is just the number `3`, but writing `OP_LOAD` in a `switch` is far clearer than writing `0x03`. Enums make code self-documenting.

#### `switch` / `case` / `default` / `break`
A multi-way branch. The value in `switch(out->opcode)` is compared against each `case`. When a match is found, that block runs. `break` exits the switch — without it, execution "falls through" to the next case (usually a bug). `default` catches anything that didn't match any case. In this project, `switch` is used extensively to dispatch on opcode, funct3, and funct7.

#### `if` / `else`
Basic conditional. `if (argc != 2)` checks if the number of arguments is wrong. `if (fp == NULL)` checks if the file failed to open. `else` provides the alternative path.

#### `while`
A loop that repeats as long as its condition is true. `while (fgets(line, sizeof(line), fp) != NULL)` keeps reading lines until the file ends (when `fgets` returns `NULL`).

#### `for`
A counted loop. `for (i = 0; i < (uint32_t)total; i++)` runs once per instruction. `i` starts at `0`, runs while `i < total`, and increments by 1 each iteration.

#### `continue`
Skips the rest of the current loop iteration and goes back to the loop condition. Used in `mem_load_hex` to skip blank and comment lines without breaking out of the entire loop.

#### `break`
Exits the nearest `switch` or loop immediately. In the `while` loop of `mem_load_hex`, `break` stops reading if the memory array is full. In `switch` statements, it prevents fall-through.

#### `return`
Exits the current function and optionally passes a value back to the caller. `return SUCCESS` at the end of `decode_r_type` tells the caller decoding was successful. `return EXIT_FAILURE` in `main` tells the shell the program encountered an error.

#### `sizeof`
An operator (not a function) that returns the size in bytes of a type or variable at compile time. `sizeof(Memory)` returns the total byte size of the `Memory` struct. `sizeof(line)` returns 64 (the size of the `char line[64]` buffer). Used to avoid hardcoding sizes.

#### `NULL`
A special pointer value meaning "points to nothing". `fopen` returns `NULL` if the file cannot be opened. `fgets` returns `NULL` when it reaches the end of the file. Always check for `NULL` before using a pointer from these functions.

---

### 4.2 Preprocessor Keywords and Directives

The preprocessor runs **before** the C compiler. Lines starting with `#` are preprocessor directives, not C statements.

#### `#ifndef` / `#define` / `#endif` — Include Guards

```c
#ifndef COMMON_H
#define COMMON_H
// ... file contents ...
#endif
```

This trio is called an **include guard**. It prevents a header file from being included more than once in the same compilation. Without it, if two files both include `common.h`, the compiler would see all of `common.h`'s declarations twice and throw a "redefinition" error.

- `#ifndef COMMON_H` — "if the symbol `COMMON_H` has NOT been defined yet..."
- `#define COMMON_H` — "...then define it now (just as a flag, no value needed)..."
- `#endif` — "...end the conditional block."

On the second include, `COMMON_H` is already defined, so everything between `#ifndef` and `#endif` is skipped entirely.

#### `#include`

Copies the entire contents of a file into the current file at that exact point.

- `#include <stdio.h>` — angle brackets: system/standard library headers (the compiler knows where to find these).
- `#include "../include/memory.h"` — quotes: your own project headers (path is relative to the current file).

Order matters: include your own headers after standard library headers to avoid confusion.

#### `#define` — Constant and Macro Definition

```c
#define MAX_INSTRUCTIONS  4096
#define WORD_SIZE         4
#define SUCCESS           0
#define FAILURE          -1
```

The preprocessor does a **text substitution** before compilation. Every occurrence of `MAX_INSTRUCTIONS` in the code is replaced with `4096` before the compiler even sees it. This means there are no "magic numbers" scattered through the code — if you need to change the maximum, you change it in one place.

---

### 4.3 Standard Library Functions

#### `printf(format, ...)` — `<stdio.h>`
Prints formatted text to **standard output** (the terminal). The format string controls what gets printed:
- `%d` — signed decimal integer
- `%u` — unsigned decimal integer
- `%x` / `%X` — unsigned hex (lowercase / uppercase)
- `%08X` — uppercase hex, at least 8 digits wide, padded with zeros on the left
- `%s` — string (null-terminated char array)
- `%-10s` — left-aligned string in a 10-character-wide column
- `\n` — newline character

#### `fprintf(stream, format, ...)` — `<stdio.h>`
Same as `printf` but sends output to a specific stream. We use `fprintf(stderr, ...)` for error and warning messages. `stderr` is the standard error stream — it appears in the terminal separately from normal output, so it doesn't corrupt the decoder output when you redirect `stdout` to a file.

#### `snprintf(buffer, size, format, ...)` — `<stdio.h>`
Like `printf` but writes the result into a `char` array (`buffer`) instead of printing it. The `size` argument limits how many bytes get written — this prevents **buffer overflow** (writing past the end of the array). Used in `decoder.c` to build the mnemonic string safely inside `out->mnemonic[32]`.

#### `fopen(filename, mode)` — `<stdio.h>`
Opens a file and returns a `FILE *` pointer. Mode `"r"` means open for reading. Returns `NULL` if the file does not exist or cannot be opened. Always check for `NULL` before using the pointer.

#### `fgets(buffer, size, stream)` — `<stdio.h>`
Reads one line of text from a file into `buffer`, reading at most `size-1` characters, then adding a null terminator `\0`. Keeps the newline `\n` at the end of the line if it fits. Returns `NULL` at end-of-file or on error. Used to read the hex file one line at a time.

#### `fclose(fp)` — `<stdio.h>`
Closes a file and flushes any buffered writes. **Always call this** when you are done with a file. Not closing files is a resource leak.

#### `sscanf(string, format, ...)` — `<stdio.h>`
Like `scanf` but reads from a string instead of from keyboard input. `sscanf(line, "%8x", &word)` parses up to 8 hexadecimal characters from `line` and stores the result as an unsigned integer in `word`. Returns the number of items successfully read (we check it equals `1`).

#### `memset(pointer, value, size)` — `<string.h>`
Fills `size` bytes starting at `pointer` with the byte `value`. `memset(out, 0, sizeof(DecodedInstr))` zeros every byte of the struct. Used to ensure no garbage data remains from whatever was previously at that memory location.

#### `memcpy(dest, src, size)` — `<string.h>`
Copies `size` bytes from `src` to `dest`. Not used directly in this project but available through `<string.h>`.

#### `strcmp(s1, s2)` — `<string.h>`
Compares two strings. Returns `0` if they are identical, a negative number if `s1` comes before `s2` alphabetically, positive otherwise. Used in `test_decoder.c` to verify that a decoded mnemonic matches the expected string.

#### `isspace(c)` — `<ctype.h>`
Returns non-zero if the character `c` is a whitespace character (space, tab, newline, etc.). Used in `mem_load_hex` to skip leading whitespace on a line.

---

### 4.4 Project Macros (defined in `common.h`)

#### `MAX_INSTRUCTIONS 4096`
The size of the `data[]` array inside the `Memory` struct. If a hex file has more than 4096 lines, loading stops and a warning is printed. `4096` is chosen because `4096 × 4 bytes = 16 KB`, a typical small instruction cache size.

#### `HEX_LINE_LEN 16`
Maximum characters expected on a single line of a `.hex` file. An 8-character hex word plus newline is 9 characters, so 16 is a safe upper bound.

#### `WORD_SIZE 4`
The size of one RISC-V instruction in bytes. RISC-V RV32I uses **fixed-width 32-bit (4-byte) instructions**. Used to calculate the PC of each instruction: `pc = base_addr + (i * WORD_SIZE)`.

#### `REG_COUNT 32`
RISC-V has exactly 32 integer registers, named `x0` through `x31`. Used to size the `names[]` array in `reg_name()` and for bounds checking.

#### `SUCCESS 0`
The return value indicating a function completed without errors. `0` is the conventional success code in C — it is what `main` returns to the shell on success, and what `decode_instruction` returns when it recognises the instruction.

#### `FAILURE -1`
The return value indicating an error. `-1` is a conventional error code. It is what `mem_load_hex` returns if the file cannot be opened, and what `decode_instruction` returns for an unknown opcode.

#### `EXTRACT_BITS(value, hi, lo)`

```c
#define EXTRACT_BITS(value, hi, lo) \
    (((uint32_t)(value) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))
```

This is a **function-like macro** — it looks like a function call but the preprocessor expands it inline. It extracts any contiguous range of bits `[hi:lo]` from a 32-bit value.

**Step-by-step for `EXTRACT_BITS(raw, 14, 12)` (funct3 field):**
1. `(uint32_t)(raw) >> 12` — shift all bits right by 12. Bits [14:12] are now at positions [2:0].
2. `(hi - lo + 1) = (14 - 12 + 1) = 3`. We want 3 bits.
3. `(1u << 3) - 1 = 8 - 1 = 7 = 0b0111`. This is a mask of 3 ones.
4. `shifted_value & 0b0111` — zero out everything above bit 2. Result: the 3-bit funct3 value.

**Why a macro and not a function?** Because `hi` and `lo` are compile-time constants in every call, the compiler can optimise the entire expression to a single shift and AND instruction — no function call overhead.

#### `SIGN_EXTEND(value, bits)`

```c
#define SIGN_EXTEND(value, bits) \
    (((int32_t)((value) << (32 - (bits)))) >> (32 - (bits)))
```

Converts an `n`-bit value into a full 32-bit signed integer, preserving the sign.

**Step-by-step for a 12-bit immediate value of `0b111111111010` (which represents -6 in 12-bit two's complement):**
1. Cast to `int32_t` and left-shift by `32 - 12 = 20`. The 12-bit sign bit (bit 11) is now at bit 31 (the MSB of `int32_t`).
2. Arithmetic right-shift by `20`. Because the value is `int32_t` (signed), C fills the vacated bits with copies of bit 31 (the sign bit). This propagates the sign through all 20 upper bits.
3. Result: the full 32-bit two's complement representation of -6.

See Section 7 for a deeper example.

---

### 4.5 Project Enums (defined in `decoder.h`)

#### `Opcode` enum

The 7-bit opcode field is the first thing checked during decode. It tells you the instruction's format group.

| Enum Name   | Hex Value | Instructions in This Group              |
|-------------|-----------|----------------------------------------|
| `OP_LOAD`   | `0x03`    | LB, LH, LW, LBU, LHU                  |
| `OP_IMM`    | `0x13`    | ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI |
| `OP_AUIPC`  | `0x17`    | AUIPC                                   |
| `OP_STORE`  | `0x23`    | SB, SH, SW                             |
| `OP_REG`    | `0x33`    | ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU |
| `OP_LUI`    | `0x37`    | LUI                                     |
| `OP_BRANCH` | `0x63`    | BEQ, BNE, BLT, BGE, BLTU, BGEU        |
| `OP_JALR`   | `0x67`    | JALR                                    |
| `OP_JAL`    | `0x6F`    | JAL                                     |

These values come directly from the RISC-V Unprivileged Specification, Table 24.1.

#### `Funct3` enum

Once the opcode narrows things down to a format group, `funct3` (bits [14:12]) picks the exact operation. The same numeric value means different things in different opcode groups (e.g., `0x0` = ADD in R-type, BEQ in B-type, LB in loads). That's fine — the opcode is checked first, so context is already known.

| Enum Name   | Value | Meaning in context                     |
|-------------|-------|----------------------------------------|
| `F3_ADD_SUB`| `0x0` | ADD/ADDI (or SUB when funct7=0x20)     |
| `F3_SLL`    | `0x1` | SLL, SLLI                              |
| `F3_SLT`    | `0x2` | SLT, SLTI                              |
| `F3_SLTU`   | `0x3` | SLTU, SLTIU                            |
| `F3_XOR`    | `0x4` | XOR, XORI / BLT (in branch context)   |
| `F3_SRL_SRA`| `0x5` | SRL/SRLI (funct7=0x00) or SRA/SRAI (funct7=0x20) |
| `F3_OR`     | `0x6` | OR, ORI / BLTU (in branch context)    |
| `F3_AND`    | `0x7` | AND, ANDI / BGEU (in branch context)  |
| `F3_BEQ`    | `0x0` | Branch if Equal                        |
| `F3_BNE`    | `0x1` | Branch if Not Equal                    |

#### `Funct7` enum

Only used in R-type instructions to distinguish the two variants that share the same opcode and funct3.

| Enum Name   | Value  | Meaning                              |
|-------------|--------|--------------------------------------|
| `F7_NORMAL` | `0x00` | Standard: ADD, SRL                   |
| `F7_ALT`    | `0x20` | Alternate: SUB (instead of ADD), SRA (instead of SRL) |

Bit 30 of the instruction (which is bit 5 of funct7) is the distinguishing bit. `0x20` in binary is `0010 0000`, so bit 5 is set.

#### `InstrFormat` enum

Tracks which of the six RISC-V encoding formats an instruction uses. Stored in `DecodedInstr.format` but mainly useful for debugging and future simulator extensions.

| Value         | Format  | Immediate Width | Instructions             |
|---------------|---------|-----------------|--------------------------|
| `FMT_R`       | R-type  | None            | ADD, SUB, AND, OR, ...   |
| `FMT_I`       | I-type  | 12-bit signed   | ADDI, LW, JALR, ...      |
| `FMT_S`       | S-type  | 12-bit signed   | SB, SH, SW               |
| `FMT_B`       | B-type  | 13-bit signed   | BEQ, BNE, BLT, ...       |
| `FMT_U`       | U-type  | 20-bit unsigned | LUI, AUIPC               |
| `FMT_J`       | J-type  | 21-bit signed   | JAL                      |
| `FMT_UNKNOWN` | —       | —               | Unrecognised opcode      |

---

### 4.6 Project Structs

#### `Memory` (defined in `memory.h`)

```c
typedef struct {
    uint32_t data[MAX_INSTRUCTIONS];
    uint32_t count;
    uint32_t base_addr;
} Memory;
```

| Field        | Type              | Purpose                                           |
|--------------|-------------------|---------------------------------------------------|
| `data[]`     | `uint32_t[4096]`  | Flat array storing every loaded instruction word  |
| `count`      | `uint32_t`        | How many words are actually in `data[]`           |
| `base_addr`  | `uint32_t`        | PC of the first instruction (always `0x00000000`) |

There is **no dynamic memory allocation** (no `malloc`). The array is a fixed-size block on the stack. This is why Valgrind reports zero leaks — no heap memory is used.

#### `DecodedInstr` (defined in `decoder.h`)

The central data structure. One instance is created per instruction in the decode loop.

```c
typedef struct {
    uint32_t    raw;
    uint32_t    pc;
    Opcode      opcode;
    InstrFormat format;
    uint8_t     rd;
    uint8_t     rs1;
    uint8_t     rs2;
    uint8_t     funct3;
    uint8_t     funct7;
    int32_t     imm;
    char        mnemonic[32];
    int         valid;
} DecodedInstr;
```

| Field       | Type           | Purpose                                                   |
|-------------|----------------|-----------------------------------------------------------|
| `raw`       | `uint32_t`     | The original 32-bit machine word (unchanged)             |
| `pc`        | `uint32_t`     | Program counter — address of this instruction            |
| `opcode`    | `Opcode`       | Extracted bits [6:0]; identifies the instruction group   |
| `format`    | `InstrFormat`  | R/I/S/B/U/J — set by the format-specific decoder        |
| `rd`        | `uint8_t`      | Destination register index 0–31                          |
| `rs1`       | `uint8_t`      | Source register 1 index 0–31                             |
| `rs2`       | `uint8_t`      | Source register 2 index 0–31 (not used by I-type)        |
| `funct3`    | `uint8_t`      | 3-bit function code; disambiguates within opcode group   |
| `funct7`    | `uint8_t`      | 7-bit function code; only meaningful in R-type           |
| `imm`       | `int32_t`      | Sign-extended immediate value (0 for R-type)             |
| `mnemonic`  | `char[32]`     | The assembled human-readable string, e.g. `"addi x2, x0, 5"` |
| `valid`     | `int`          | `1` = successfully decoded, `0` = UNKNOWN                |

`uint8_t` (8-bit unsigned) is used for register indices and function codes because they never exceed 31 or 127 respectively. `int32_t` (32-bit signed) is used for the immediate because RISC-V immediates are signed.

---

### 4.7 Project Functions

#### `mem_init(Memory *mem)`
Takes a **pointer** to a `Memory` struct (not a copy). Zeroes all bytes with `memset`, then sets `base_addr = 0`. The `*` in the parameter type means "pointer to Memory" — changes made via the pointer affect the original struct in `main`.

#### `mem_load_hex(Memory *mem, const char *filename)` → `int`
Opens `filename`, reads it line by line with `fgets`, parses each hex string with `sscanf`, stores words in `mem->data[]`. Returns instruction count on success, `FAILURE` on error. The `->` operator accesses struct members through a pointer.

#### `mem_read_word(const Memory *mem, uint32_t index)` → `uint32_t`
Returns `mem->data[index]`. `const Memory *` means the function promises not to modify the struct — it is read-only access.

#### `decode_instruction(uint32_t raw, uint32_t pc, DecodedInstr *out)` → `int`
The main decode function. Extracts all fields, dispatches on opcode, fills `*out`, returns `SUCCESS` or `FAILURE`.

#### `print_instruction(const DecodedInstr *instr)`
Prints one row: `0x00000000 00500113  addi x2, x0, 5`. Uses `const` because it only reads, never modifies.

#### `print_header(void)`
Prints the column header line. `void` parameter means it takes no arguments.

#### `reg_name(uint8_t reg)` → `const char *`
Returns a pointer to a string like `"x2"`. The returned pointer points into a `static` array that lives forever, so it is safe to use after the function returns. `const char *` means the caller cannot modify the string.

#### `decode_r_type(DecodedInstr *out)` — `static`
Handles R-format instructions. `static` = private to `decoder.c`. Uses a `switch` on `out->funct3` (and checks `out->funct7` for ADD vs SUB, SRL vs SRA). Writes the mnemonic into `out->mnemonic`.

#### `decode_i_arith(DecodedInstr *out)` — `static`
Handles I-format arithmetic: ADDI, SLTI, ANDI, ORI, XORI, SLLI, SRLI, SRAI. Calls `imm_i(raw)` to extract the 12-bit signed immediate.

#### `decode_load(DecodedInstr *out)` — `static`
Handles LB, LH, LW, LBU, LHU. Load syntax: `lw rd, imm(rs1)`. The immediate is the byte offset from the base register.

#### `decode_store(DecodedInstr *out)` — `static`
Handles SB, SH, SW. Store syntax: `sw rs2, imm(rs1)`. Note that in stores, **there is no destination register** — data goes from `rs2` into memory at address `rs1 + imm`.

#### `decode_branch(DecodedInstr *out)` — `static`
Handles BEQ, BNE, BLT, BGE, BLTU, BGEU. The immediate is a **PC-relative offset** — the CPU adds it to the current PC to calculate the branch target. Branch offsets are always multiples of 2 (bit 0 is always 0 and is not stored in the instruction).

#### `imm_i(uint32_t raw)` → `int32_t` — `static`
Extracts and sign-extends the 12-bit I-type immediate from bits [31:20].

#### `imm_s(uint32_t raw)` → `int32_t` — `static`
Extracts the S-type immediate: upper 7 bits from [31:25], lower 5 bits from [11:7]. Reassembles, then sign-extends to 12 bits.

#### `imm_b(uint32_t raw)` → `int32_t` — `static`
The most complex immediate. Bits are scattered in this order in the instruction: bit12 at [31], bit11 at [7], bits10:5 at [30:25], bits4:1 at [11:8]. Bit 0 is always 0 (not stored). After reassembly, sign-extended to 13 bits.

#### `imm_u(uint32_t raw)` → `int32_t` — `static`
Simplest immediate: just zero out the lower 12 bits with `raw & 0xFFFFF000`. The upper 20 bits are already in place. For display, we right-shift by 12 to show the page number.

#### `imm_j(uint32_t raw)` → `int32_t` — `static`
J-type for JAL: bit20 at [31], bits10:1 at [30:21], bit11 at [20], bits19:12 at [19:12]. Bit 0 = 0. After reassembly, sign-extended to 21 bits. JAL can jump ±1 MB from the current PC.

---

### 4.8 Makefile Keywords

#### `CC = gcc`
Sets the compiler variable. `gcc` is the GNU C Compiler. Every compile command uses `$(CC)` so you can switch compilers by changing one line.

#### `CFLAGS = -Wall -Wextra -std=c99`
Compiler flags applied to every compilation:
- `-Wall` — enable all standard warnings (catches common mistakes like uninitialised variables, unused variables, etc.)
- `-Wextra` — enable extra warnings beyond `-Wall`
- `-std=c99` — compile as C99 standard (allows `//` comments, `uint32_t`, and mixed declarations/statements)

#### `-O2` (in `RELFLAGS`)
Optimisation level 2. The compiler reorganises and optimises the generated machine code for speed. Used in the release build.

#### `-g -O0` (in `DBGFLAGS`)
`-g` embeds debug symbols (variable names, line numbers) into the binary so GDB can show them. `-O0` disables optimisation so the code runs exactly as written — crucial for debugging because optimisation can reorder or eliminate statements.

#### `-I$(INCDIR)`
Tells the compiler to search the `include/` directory for header files. Without this, `#include "decoder.h"` would only look in the current directory.

#### `-c`
Compile only — produce a `.o` object file, do not link. Linking happens in a separate step.

#### `-o $@`
Output file name. `$@` is a Makefile automatic variable meaning "the target of this rule".

#### `$^`
Another automatic variable meaning "all prerequisites of this rule" — the list of all `.o` files being linked.

#### `$<`
"The first prerequisite" — the single `.c` file being compiled in a pattern rule.

#### `.PHONY`
Declares that `all`, `clean`, `test`, `debug`, `valgrind` are not actual file names — they are command names. Without `.PHONY`, if a file named `clean` existed in the directory, `make clean` would do nothing (thinking the file is already "up to date").

#### `mkdir -p`
Creates a directory. `-p` means "also create any missing parent directories, and don't error if the directory already exists".

#### `rm -rf`
Removes files and directories. `-r` = recursive (deletes directories and their contents). `-f` = force (no error if the file doesn't exist). Used by `make clean` to wipe `bin/` and `build/`.

#### `valgrind --leak-check=full`
Runs the program under Valgrind's memory error detector. `--leak-check=full` shows a full report of any heap memory that was allocated but never freed. Since this project uses no `malloc`, Valgrind should report "0 bytes in 0 blocks" for leaks.

---

### 4.9 RISC-V Architecture Terms

#### ISA (Instruction Set Architecture)
The specification that defines what instructions a processor understands, what they do, and how they are encoded in binary. RISC-V is an open ISA. `RV32I` means: RISC-V, 32-bit address space, Integer base instruction set.

#### Opcode
The **operation code** — the primary identifier of an instruction. In RISC-V, bits [6:0] of every instruction encode the opcode. There are only ~10 unique opcodes in RV32I because `funct3` and `funct7` further narrow down the operation.

#### funct3
A 3-bit field at bits [14:12] that differentiates instructions within the same opcode group. For example, opcode `0x33` (OP_REG) with funct3 `0x0` could be ADD or SUB — funct7 resolves which one.

#### funct7
A 7-bit field at bits [31:25], only significant in R-type instructions. Bit 30 (the second-most-significant bit of funct7) is the key differentiator: `0` → normal operation (ADD, SRL), `1` → alternate (SUB, SRA).

#### rd (Destination Register)
Bits [11:7]. The register where the result is written. For example, `addi x2, x0, 5` writes the value 5 into `x2`. In store instructions, there is no `rd` — those bits are part of the immediate.

#### rs1 (Source Register 1)
Bits [19:15]. The first input operand register. In `add x1, x2, x3`, `rs1` is `x2`.

#### rs2 (Source Register 2)
Bits [24:20]. The second input operand register. In `add x1, x2, x3`, `rs2` is `x3`. Not present in I-type (those bits are used for the immediate instead).

#### Immediate
A constant value **encoded directly inside the instruction** rather than read from a register. In `addi x2, x0, 5`, the value `5` is the immediate. RISC-V splits and scrambles immediate bits across the instruction word to simplify hardware routing in the CPU datapath.

#### PC (Program Counter)
A special register inside the CPU that holds the memory address of the **currently executing instruction**. After each instruction, PC advances by 4 (one word). Branch and jump instructions change the PC to something other than `PC + 4`.

#### Sign Extension
Expanding a smaller integer (e.g., 12-bit) to a larger integer (32-bit) while preserving its signed value. A 12-bit value with bit 11 = 1 is negative in two's complement. When extended to 32 bits, all upper 20 bits must be filled with `1`s to keep the same negative value. If bit 11 = 0 (positive), the upper bits are filled with `0`s.

#### Two's Complement
The standard way modern computers represent negative integers in binary. To negate a number: flip all bits, then add 1. For an `n`-bit value, the range is `-2^(n-1)` to `+2^(n-1) - 1`. A 12-bit immediate ranges from `-2048` to `+2047`.

#### R-type, I-type, S-type, B-type, U-type, J-type
The six instruction encoding formats in RV32I. Each format places fields at different bit positions. The opcode always occupies bits [6:0] across all formats.

#### ABI (Application Binary Interface)
Conventions for how compiled code passes data between functions. The RISC-V ABI assigns human-friendly names to registers: `x0=zero`, `x1=ra`, `x2=sp`, `x10-x11=a0-a1`, etc. This project uses the simple `xN` naming (canonical ISA names) rather than ABI names.

#### LUI / AUIPC
**Load Upper Immediate**: loads a 20-bit constant into the upper 20 bits of a register (lower 12 bits = 0). Used to construct large constants.
**Add Upper Immediate to PC**: adds the upper 20-bit immediate to the current PC. Used together with ADDI to load a full 32-bit PC-relative address.

#### JAL / JALR
**Jump And Link**: unconditional jump to `PC + imm`. Saves `PC + 4` (return address) in `rd`.
**Jump And Link Register**: unconditional jump to `rs1 + imm` (register-based target). Used to implement function returns: `jalr x0, x1, 0` (where `x1=ra`) jumps to the return address.

---

## 5. Data Flow Diagram

```
Shell command
     │
     ▼
main(argc, argv)
     │
     ├─► validate argc == 2
     │
     ├─► mem_init(&mem)
     │        └─► memset to 0, base_addr = 0
     │
     ├─► mem_load_hex(&mem, argv[1])
     │        ├─► fopen(filename, "r")
     │        ├─► loop: fgets → sscanf → mem.data[count++]
     │        └─► fclose; return count
     │
     ├─► print_header()
     │
     └─► for i = 0 to count-1:
              │
              ├─► pc = base_addr + i * 4
              ├─► raw = mem_read_word(&mem, i)
              │
              ├─► decode_instruction(raw, pc, &instr)
              │        ├─► memset(&instr, 0)
              │        ├─► extract: opcode, rd, funct3, rs1, rs2, funct7
              │        │
              │        └─► switch(opcode)
              │                 ├─ 0x33 → decode_r_type()
              │                 │           └─ switch(funct3) → snprintf mnemonic
              │                 ├─ 0x13 → decode_i_arith()
              │                 │           └─ imm_i() → snprintf mnemonic
              │                 ├─ 0x03 → decode_load()
              │                 │           └─ imm_i() → snprintf mnemonic
              │                 ├─ 0x23 → decode_store()
              │                 │           └─ imm_s() → snprintf mnemonic
              │                 ├─ 0x63 → decode_branch()
              │                 │           └─ imm_b() → snprintf mnemonic
              │                 ├─ 0x37 → LUI  (imm_u)
              │                 ├─ 0x17 → AUIPC (imm_u)
              │                 ├─ 0x6F → JAL  (imm_j)
              │                 ├─ 0x67 → JALR (imm_i)
              │                 └─ default → "UNKNOWN"
              │
              └─► print_instruction(&instr)
                       └─► printf("0x%08X %08X  %s\n", pc, raw, mnemonic)
```

---

## 6. Bit Manipulation Deep Dive

### How EXTRACT_BITS works — worked example

Instruction: `0x00500113` (ADDI x2, x0, 5)

In binary (32 bits):
```
Bit:  31      24 23    20 19   15 14 12 11    7 6      0
      00000000 0101 00000 000 00010 0010011
      ──────── ──── ───── ─── ───── ───────
      funct7   imm  rs1   f3   rd   opcode
```

**Extract opcode (bits [6:0]):**
```
raw              = 0x00500113 = 0000 0000 0101 0000 0001 0001 0011
raw >> 0         = same (no shift)
mask = (1<<7)-1  = 0111 1111 = 0x7F
result           = 0001 0011 = 0x13 = OP_IMM ✓
```

**Extract funct3 (bits [14:12]):**
```
raw >> 12        = 0x00000005 → last nibbles: ...0101 0000 0001
                  (the 12 we care about are now at positions [2:0])
mask = (1<<3)-1  = 0b111
result           = 0b000 = 0 = F3_ADD_SUB → ADDI ✓
```

**Extract immediate (bits [31:20]) for I-type:**
```
raw >> 20        = 0x00000005
mask = (1<<12)-1 = 0xFFF
result           = 0x005 = 5
SIGN_EXTEND(5, 12): bit 11 of 5 is 0, so upper bits stay 0 → +5 ✓
```

---

## 7. Sign Extension Deep Dive

### Why sign extension is necessary

A 12-bit immediate can represent values from -2048 to +2047. When we store that 12-bit value in a 32-bit `int32_t`, we need the upper 20 bits to correctly reflect the sign.

### Positive example: `imm = 5` (0b000000000101)

```
12-bit value:     0000 0000 0101
Bit 11 (sign):    0 → positive

Left shift by 20: 0000 0101 0000 0000 0000 0000 0000 0000  (as int32_t)
Arithmetic right shift by 20: fills upper bits with 0 (sign bit was 0)
Result:           0000 0000 0000 0000 0000 0000 0000 0101 = +5 ✓
```

### Negative example: `imm = 0xFFE` (which is -2 in 12-bit two's complement)

```
12-bit value:     1111 1111 1110
Bit 11 (sign):    1 → negative

Left shift by 20: 1111 1110 0000 0000 0000 0000 0000 0000  (as int32_t, MSB=1)
Arithmetic right shift by 20: fills upper bits with 1 (sign bit was 1)
Result:           1111 1111 1111 1111 1111 1111 1111 1110 = -2 ✓
```

This is the standard **arithmetic right shift** trick used in all RISC-V implementations.

---

## 8. Git Workflow for Part C

Part C of the assignment requires a specific git history. Here is the exact sequence of commands to satisfy all requirements:

```bash
# ── Initial setup ───────────────────────────────────────────────
cd riscv-decoder
git init
git add .gitignore README.md Makefile docs/
git commit -m "project: initial structure, Makefile, README, .gitignore"

# ── Branch 1: memory subsystem ──────────────────────────────────
git checkout -b feature/memory-loader
git add include/common.h include/memory.h src/memory.c
git commit -m "memory: add Memory struct and mem_init"
git add src/memory.c   # after adding mem_load_hex
git commit -m "memory: implement mem_load_hex with comment and blank-line skipping"
git add src/memory.c   # after adding mem_read_word
git commit -m "memory: implement mem_read_word with bounds check"
git checkout main
git merge feature/memory-loader

# ── Branch 2: R-type decoder ─────────────────────────────────────
git checkout -b feature/r-type-decode
git add include/decoder.h
git commit -m "decoder: add DecodedInstr struct, Opcode/Funct3/Funct7 enums"
git add src/decoder.c   # after adding EXTRACT_BITS helpers + decode_r_type
git commit -m "decoder: implement field extraction helpers and decode_r_type"
git checkout main
git merge feature/r-type-decode

# ── Branch 3: full decoder ───────────────────────────────────────
git checkout -b feature/full-decode
git add src/decoder.c   # after completing all format decoders
git commit -m "decoder: implement I-type arithmetic decode (ADDI, ANDI, SLLI, ...)"
git add src/decoder.c
git commit -m "decoder: implement load, store, branch decoders"
git add src/decoder.c
git commit -m "decoder: implement U-type (LUI, AUIPC) and J-type (JAL, JALR)"
git add src/decoder.c
git commit -m "decoder: implement sign-extended immediates for all formats"
git checkout main
git merge feature/full-decode

# ── Main integration ─────────────────────────────────────────────
git add src/main.c
git commit -m "main: implement CLI parsing, decode loop, and summary output"

# ── Test files ────────────────────────────────────────────────────
git add test/
git commit -m "test: add unit tests and four hex test programs"

# ── Final check ──────────────────────────────────────────────────
git add docs/DESIGN.md
git commit -m "docs: add DESIGN.md with decoder architecture rationale"
```

This gives you: more than 12 commits, 3 feature branches, 3 merges into main, and a clean `.gitignore` excluding `bin/` and `build/`.

---

*End of Project Guide — MEDS Module 2, UET Lahore*
