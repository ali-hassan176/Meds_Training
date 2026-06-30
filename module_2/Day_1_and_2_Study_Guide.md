# MEDS Module 2 - C Language for Hardware Engineers
## Day 1 & Day 2 Complete Study Guide

**Course:** C Language for Hardware Engineers  
**Module:** Module 2 - MEDS  
**Duration:** Days 1 & 2

---

## Table of Contents
1. [Day 1: C Foundations](#day-1)
2. [Day 2: Pointers, Arrays & Memory](#day-2)
3. [Complete Exercise Solutions](#exercise-solutions)

---

# Day 1: C Foundations: Compilation, Types & Control Flow

## 1.1 The C Compilation Pipeline

### What is the C Compilation Pipeline?

C code is not directly executed like Python or JavaScript. Instead, it goes through **4 distinct stages** before becoming a running program. Understanding this process is critical for hardware engineers.

### The 4 Stages Explained:

#### Stage 1: Preprocessing (cpp)
- **What happens:** The preprocessor expands:
  - `#include` directives (includes other files)
  - `#define` macros (text substitution)
  - `#ifdef` conditional compilation

- **Input:** `main.c` (your source code)
- **Output:** `main.i` (still C code, but fully expanded - no #includes or #defines)
- **Example:** If you have `#include <stdio.h>`, the preprocessor replaces this line with the entire contents of stdio.h

```bash
gcc -E main.c -o main.i    # Run only preprocessor
```

#### Stage 2: Compilation (cc1)
- **What happens:** Converts C code into **assembly language** (human-readable machine instructions)
- **Input:** `main.i` (preprocessed C)
- **Output:** `main.s` (assembly code)
- **Error Detection:** Type errors and syntax errors are caught here
- **Example:** C code `int x = 5;` becomes assembly instructions

```bash
gcc -S main.c -o main.s    # Run up to assembly generation
```

#### Stage 3: Assembly (as)
- **What happens:** Assembler translates assembly instructions to **machine code** (binary)
- **Input:** `main.s` (assembly)
- **Output:** `main.o` (object file - binary, platform-specific)
- **Platform-Specific:** The .o file is specific to your CPU architecture

```bash
gcc -c main.c -o main.o    # Compile to object file (stops before linking)
```

#### Stage 4: Linking (ld)
- **What happens:** Links multiple object files and libraries together
- **Input:** `main.o` + library files (like libc)
- **Output:** Final executable program
- **Resolves:** Function calls across files and libraries
- **Example:** Your `main.c` calls `printf()` from the C standard library - linking resolves this

```bash
gcc main.o -o main         # Link object file into executable
```

### Complete Compilation Command:

```bash
gcc -Wall -Wextra -std=c11 -g -o main main.c
```

This runs all 4 stages automatically.

**Flag Meanings:**
- `-Wall` - Enable most common warnings
- `-Wextra` - Enable extra warnings for better code quality
- `-std=c11` - Use C11 standard (modern C standard)
- `-g` - Include debugging information (for GDB debugger)
- `-O2` - Optimization level 2 (for production builds)
- `-o main` - Output file name

### Hardware Connection:
When you examine the `.s` assembly file, you see actual **RISC-V instructions** (or x86 if on x86 machine). These are exactly what your processor will execute. In Week 3-4, you'll write RISC-V assembly directly.

---

## 1.2 Data Types & Fixed-Width Integers

### Why This Matters:

In hardware engineering, you need **exact control** over data sizes. The problem: standard C types (`int`, `long`) have **platform-dependent sizes**:
- On a 32-bit system: `int` might be 2 bytes
- On a 64-bit system: `int` might be 4 or 8 bytes

**Solution:** Use fixed-width integer types from `<stdint.h>`

### Fixed-Width Integer Types:

| Type | Size | Range | Use Case |
|------|------|-------|----------|
| `uint8_t` | 1 byte | 0 to 255 | Single byte data, register bytes |
| `int8_t` | 1 byte | -128 to 127 | Signed byte values |
| `uint16_t` | 2 bytes | 0 to 65,535 | Half-word values, CSR fields |
| `int16_t` | 2 bytes | -32,768 to 32,767 | Signed 16-bit |
| `uint32_t` | 4 bytes | 0 to 4.3 billion | RV32 registers, instructions |
| `int32_t` | 4 bytes | -2.1B to +2.1B | Signed 32-bit values |
| `uint64_t` | 8 bytes | 0 to 18 quintillion | RV64 registers |
| `int64_t` | 8 bytes | ±9.2 quintillion | Signed 64-bit |

### Example Usage:

```c
#include <stdint.h>
#include <stdio.h>

int main(void) {
    // RISC-V register (always 32 bits)
    uint32_t register_value = 0xDEADBEEF;
    
    // Instruction encoding (always 32 bits)
    uint32_t instruction = 0x00A28233;
    
    // Memory address (32 bits on RV32, 64 bits on RV64)
    uint32_t *address = (uint32_t *)0x80000000;
    
    printf("Register: 0x%08X\n", register_value);
    printf("Size of uint32_t: %zu bytes\n", sizeof(uint32_t));
    
    return 0;
}
```

### Critical Rule:
**NEVER use `int` for hardware work!** Always explicitly use `uint32_t` or `int32_t`.

---

## 1.3 Bitwise Operations

### Introduction:

Bitwise operations manipulate individual bits - the fundamental operations in hardware. Every register access, instruction decode, and CSR manipulation uses bitwise operations.

### Basic Bitwise Operators:

#### AND Operator (`&`)
- **Operation:** Both bits must be 1 for result to be 1
- **Use:** Masking (selecting specific bits)

```c
uint32_t a = 0b1111_0000;  // 240
uint32_t b = 0b1010_1010;  // 170
uint32_t result = a & b;    // 0b1010_0000 = 160

// Practical example: Extract specific bits
uint32_t instruction = 0x00A28233;
uint32_t opcode = instruction & 0x7F;  // 0x7F = 0b0111_1111 masks lower 7 bits
```

#### OR Operator (`|`)
- **Operation:** If either bit is 1, result is 1
- **Use:** Setting specific bits

```c
uint32_t register_value = 0x00000000;
uint32_t register_value = register_value | 0x00000001;  // Set bit 0
// Result: 0x00000001
```

#### XOR Operator (`^`)
- **Operation:** Bits are 1 only if they're different
- **Use:** Toggling bits, comparison

```c
uint32_t bits = 0b1111_0000;
uint32_t toggle = 0b1010_1010;
uint32_t result = bits ^ toggle;  // 0b0101_1010 - toggles matching bits
```

#### NOT Operator (`~`)
- **Operation:** Inverts all bits (0→1, 1→0)
- **Use:** Creating bit masks

```c
uint32_t mask = ~0x000000FF;  // Inverts lower 8 bits
// Result: 0xFFFFFF00
```

#### Left Shift (`<<`)
- **Operation:** Moves all bits left, fills right with zeros
- **Effect:** Multiplies by 2^n

```c
uint32_t a = 0x00000001;
uint32_t result = a << 4;  // Shift left 4 positions
// Result: 0x00000010 (1 * 2^4 = 16)
```

#### Right Shift (`>>`)
- **Operation:** Moves all bits right, fills left with zeros
- **Effect:** Divides by 2^n

```c
uint32_t a = 0x00F00000;
uint32_t result = a >> 8;  // Shift right 8 positions
// Result: 0x0000F000
```

### Practical Example: RISC-V Instruction Decoding

A RISC-V instruction is 32 bits organized as:

```
[31:25] [24:20] [19:15] [14:12] [11:7] [6:0]
 funct7   rs2    rs1    funct3   rd    opcode
```

```c
#include <stdio.h>
#include <stdint.h>

int main(void) {
    // Instruction: 0x00A28233 (add x4, x5, x10)
    uint32_t instruction = 0x00A28233;
    
    // Extract fields using bitwise operations
    uint32_t opcode = instruction & 0x7F;              // bits [6:0]
    uint32_t rd = (instruction >> 7) & 0x1F;          // bits [11:7]
    uint32_t funct3 = (instruction >> 12) & 0x07;     // bits [14:12]
    uint32_t rs1 = (instruction >> 15) & 0x1F;        // bits [19:15]
    uint32_t rs2 = (instruction >> 20) & 0x1F;        // bits [24:20]
    uint32_t funct7 = (instruction >> 25) & 0x7F;     // bits [31:25]
    
    printf("Instruction: 0x%08X\n", instruction);
    printf("opcode: 0x%02X\n", opcode);    // 0x33
    printf("rd: x%u\n", rd);               // x4
    printf("funct3: %u\n", funct3);        // 0
    printf("rs1: x%u\n", rs1);             // x5
    printf("rs2: x%u\n", rs2);             // x10
    printf("funct7: 0x%02X\n", funct7);    // 0x00
    
    return 0;
}
```

### Common Bit Manipulation Patterns:

```c
// Extract bits [high:low] from value
#define EXTRACT_BITS(val, high, low) \
    (((val) >> (low)) & ((1U << ((high) - (low) + 1)) - 1))

// Set bit N to 1
#define SET_BIT(val, n) ((val) | (1U << (n)))

// Clear bit N to 0
#define CLEAR_BIT(val, n) ((val) & ~(1U << (n)))

// Toggle bit N (0→1, 1→0)
#define TOGGLE_BIT(val, n) ((val) ^ (1U << (n)))

// Check if bit N is set (returns 1 or 0)
#define IS_BIT_SET(val, n) (((val) >> (n)) & 1U)
```

### Sign Extension Example:

Sign extension converts a small signed number to a larger signed number while preserving its value:

```c
// Example: Convert 12-bit signed value to 32-bit signed
// 0xFFF in 12-bit is -1
// Should become 0xFFFFFFFF (-1 in 32-bit)

int32_t sign_extend(uint32_t val, int bit_width) {
    uint32_t sign_bit = 1U << (bit_width - 1);
    return (int32_t)((val ^ sign_bit) - sign_bit);
}

// Usage
int32_t result = sign_extend(0xFFF, 12);  // Returns -1 (0xFFFFFFFF)
```

---

## 1.4 Control Flow

Control flow determines which code executes based on conditions.

### Switch-Case (Perfect for Opcode Decoding):

```c
#include <stdio.h>

void execute_instruction(uint8_t opcode) {
    switch (opcode) {
        case 0x33:  // R-type instruction
            printf("Executing R-type\n");
            break;
        case 0x13:  // I-type (immediate)
            printf("Executing I-type\n");
            break;
        case 0x23:  // S-type (store)
            printf("Executing S-type\n");
            break;
        case 0x63:  // B-type (branch)
            printf("Executing B-type\n");
            break;
        default:
            printf("Unknown opcode: 0x%02X\n", opcode);
            break;
    }
}
```

### If-Else Statements:

```c
if (opcode == 0x33) {
    // Handle R-type
} else if (opcode == 0x13) {
    // Handle I-type
} else {
    // Handle other types
}
```

### Ternary Operator (Conditional Expression):

```c
// Useful for mux-like selections
uint32_t result = (opcode == 0x33) ? alu_result : immediate;

// Equivalent to:
uint32_t result;
if (opcode == 0x33) {
    result = alu_result;
} else {
    result = immediate;
}
```

### Loops:

```c
// For loop: iterate fixed number of times
for (int i = 0; i < 32; i++) {
    printf("Register x%d = 0x%08X\n", i, registers[i]);
}

// While loop: continue while condition is true
int i = 0;
while (i < 32) {
    printf("Register x%d = 0x%08X\n", i, registers[i]);
    i++;
}
```

---

## 1.5 Functions

Functions organize code into reusable blocks. In hardware, they model operations like instruction decode or ALU computation.

### Function Structure:

```c
// Function declaration (prototype) - usually in .h file
uint32_t decode_opcode(uint32_t instruction);

// Function definition - in .c file
uint32_t decode_opcode(uint32_t instruction) {
    return instruction & 0x7F;  // Return bits [6:0]
}

// Function that returns nothing (void)
void print_register(uint32_t reg_num, uint32_t value) {
    printf("x%u = 0x%08X (%d)\n", reg_num, value, (int32_t)value);
}
```

### Function Parameters:

```c
// By value: function receives a COPY
void increment_by_value(int x) {
    x++;  // Only increments local copy, doesn't affect caller's variable
}

// By reference (using pointers): function receives ADDRESS
void increment_by_reference(int *x) {
    (*x)++;  // Increments the actual variable
}
```

---

# Day 1 Exercise Solutions

## Exercise 1: Hex to Binary, Decimal, Hex Converter

**Problem:** Write a program that takes a 32-bit hex value as command-line argument and prints it in binary, decimal (signed & unsigned), and hex.

```c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hex_value>\n", argv[0]);
        return 1;
    }
    
    // Parse hex string using strtoul
    // strtoul(string, end_ptr, base)
    // Returns: parsed unsigned long
    uint32_t value = (uint32_t)strtoul(argv[1], NULL, 16);
    
    printf("Input (hex): %s\n", argv[1]);
    printf("Unsigned decimal: %u\n", value);
    printf("Signed decimal: %d\n", (int32_t)value);
    printf("Hex: 0x%08X\n", value);
    
    // Print binary representation
    printf("Binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%u", (value >> i) & 1);
        if (i % 4 == 0) printf(" ");  // Spacing for readability
    }
    printf("\n");
    
    return 0;
}
```

**Explanation:**
- `strtoul(argv[1], NULL, 16)` parses the string as hexadecimal (base 16)
- Cast to `uint32_t` ensures 32-bit value
- Print unsigned as `%u`, signed as `%d`
- Print binary by shifting and extracting each bit
- Add spaces every 4 bits for readability

**Test:**
```bash
./hex_converter DEADBEEF
```

---

## Exercise 2: Examine Compilation Stages

**Problem:** Compile a simple program through all 4 stages separately. Examine each output file.

```bash
# Create simple program
cat > simple.c << 'EOF'
#include <stdio.h>

int main(void) {
    int x = 42;
    printf("x = %d\n", x);
    return 0;
}
EOF

# Stage 1: Preprocessing (-E outputs, doesn't create file typically)
gcc -E simple.c -o simple.i
# Look at simple.i - see it includes full stdio.h

# Stage 2: Compilation to assembly (-S)
gcc -S simple.c -o simple.s
# Look at simple.s - see actual RISC-V or x86 assembly

# Stage 3: Assembly to object file (-c)
gcc -c simple.c -o simple.o
# simple.o is binary, not human-readable

# Stage 4: Linking
gcc simple.o -o simple
# simple is the executable

# Compare sizes
ls -lh simple.c simple.i simple.s simple.o simple
```

**Observations:**
- `simple.i` is much larger (includes all of stdio.h)
- `simple.s` is human-readable assembly
- `simple.o` is binary (platform-specific)
- `simple` is the final executable

---

## Exercise 3: Extract Instruction Fields

**Problem:** Write a function `uint32_t extract_field(uint32_t instruction, int high, int low)` that extracts bits [high:low].

```c
#include <stdio.h>
#include <stdint.h>

// Extract bits [high:low] from instruction
uint32_t extract_field(uint32_t instruction, int high, int low) {
    // Shift right to align low bit to position 0
    // Then mask to keep only (high - low + 1) bits
    int width = high - low + 1;
    return (instruction >> low) & ((1U << width) - 1);
}

int main(void) {
    // Test with R-type instruction: 0x00A28233 (add x4, x5, x10)
    uint32_t instruction = 0x00A28233;
    
    uint32_t opcode = extract_field(instruction, 6, 0);    // bits [6:0]
    uint32_t rd = extract_field(instruction, 11, 7);       // bits [11:7]
    uint32_t funct3 = extract_field(instruction, 14, 12);  // bits [14:12]
    uint32_t rs1 = extract_field(instruction, 19, 15);     // bits [19:15]
    uint32_t rs2 = extract_field(instruction, 24, 20);     // bits [24:20]
    uint32_t funct7 = extract_field(instruction, 31, 25);  // bits [31:25]
    
    printf("Instruction: 0x%08X\n", instruction);
    printf("opcode [6:0]:   0x%02X\n", opcode);    // 0x33
    printf("rd [11:7]:      x%u\n", rd);           // 4
    printf("funct3 [14:12]: %u\n", funct3);        // 0
    printf("rs1 [19:15]:    x%u\n", rs1);          // 5
    printf("rs2 [24:20]:    x%u\n", rs2);          // 10
    printf("funct7 [31:25]: 0x%02X\n", funct7);    // 0x00
    
    return 0;
}
```

**How extract_field works:**
1. `width = high - low + 1` → number of bits to extract
2. `instruction >> low` → align the low bit to position 0
3. `(1U << width) - 1` → create mask with width bits set
4. AND with mask → keep only the desired bits

---

## Exercise 4: RISC-V Instruction Decoder

**Problem:** Take an RV32 instruction as hex input and print all fields.

```c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Helper function
uint32_t extract_field(uint32_t instruction, int high, int low) {
    int width = high - low + 1;
    return (instruction >> low) & ((1U << width) - 1);
}

void decode_r_type(uint32_t instruction) {
    uint32_t opcode = extract_field(instruction, 6, 0);
    uint32_t rd = extract_field(instruction, 11, 7);
    uint32_t funct3 = extract_field(instruction, 14, 12);
    uint32_t rs1 = extract_field(instruction, 19, 15);
    uint32_t rs2 = extract_field(instruction, 24, 20);
    uint32_t funct7 = extract_field(instruction, 31, 25);
    
    printf("R-type Instruction: 0x%08X\n", instruction);
    printf("  opcode:  0x%02X\n", opcode);
    printf("  rd:      x%u\n", rd);
    printf("  funct3:  %u\n", funct3);
    printf("  rs1:     x%u\n", rs1);
    printf("  rs2:     x%u\n", rs2);
    printf("  funct7:  0x%02X\n", funct7);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <hex_instruction> [hex_instruction2] ...\n", argv[0]);
        return 1;
    }
    
    // Decode multiple instructions
    for (int i = 1; i < argc; i++) {
        uint32_t instruction = (uint32_t)strtoul(argv[i], NULL, 16);
        decode_r_type(instruction);
    }
    
    return 0;
}
```

**Test with RISC-V instructions:**
```bash
gcc -o decoder decoder.c
./decoder 0x00A28233 0x00428333 0x01030333
```

---

## Exercise 5: Sign Extension

**Problem:** Implement sign_extend(). Verify: sign_extend(0xFFF, 12) = -1 (0xFFFFFFFF as int32_t).

```c
#include <stdio.h>
#include <stdint.h>

// Sign-extend a value from bit_width to 32 bits
// Example: 0xFFF (12-bit -1) becomes 0xFFFFFFFF (32-bit -1)
int32_t sign_extend(uint32_t val, int bit_width) {
    // Get the sign bit position
    uint32_t sign_bit = 1U << (bit_width - 1);
    
    // If sign bit is set, extend with 1s; otherwise, result is just val masked
    // XOR with sign_bit toggles it to 0, then subtract to flip all extension bits
    return (int32_t)((val ^ sign_bit) - sign_bit);
}

int main(void) {
    // Test cases
    printf("Testing sign_extend:\n\n");
    
    // Test 1: 12-bit -1 (0xFFF)
    int32_t result = sign_extend(0xFFF, 12);
    printf("sign_extend(0xFFF, 12):\n");
    printf("  Result as int32: %d\n", result);           // -1
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);  // 0xFFFFFFFF
    printf("  Expected: -1 (0xFFFFFFFF) ✓\n\n");
    
    // Test 2: 12-bit positive value (0x7FF = 2047)
    result = sign_extend(0x7FF, 12);
    printf("sign_extend(0x7FF, 12):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: 2047 (0x000007FF) ✓\n\n");
    
    // Test 3: 8-bit -1 (0xFF)
    result = sign_extend(0xFF, 8);
    printf("sign_extend(0xFF, 8):\n");
    printf("  Result as int32: %d\n", result);           // -1
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);  // 0xFFFFFFFF
    printf("  Expected: -1 (0xFFFFFFFF) ✓\n\n");
    
    // Test 4: 8-bit positive (0x7F = 127)
    result = sign_extend(0x7F, 8);
    printf("sign_extend(0x7F, 8):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: 127 (0x0000007F) ✓\n");
    
    return 0;
}
```

**How it works:**
1. Get sign bit at position (bit_width - 1)
2. XOR with sign_bit to toggle it (so we can use subtraction)
3. Subtract from value - this extends the sign bit through all remaining bits
4. Cast to int32_t to interpret as signed

---

## Exercise 6 (Bonus): Pack R-type Instruction

**Problem:** Pack rd, rs1, rs2, funct3, funct7, and opcode back into a 32-bit R-type instruction.

```c
#include <stdio.h>
#include <stdint.h>

// Extract field (for testing)
uint32_t extract_field(uint32_t instruction, int high, int low) {
    int width = high - low + 1;
    return (instruction >> low) & ((1U << width) - 1);
}

// Pack fields into R-type instruction
uint32_t pack_r_type(uint32_t funct7, uint32_t rs2, uint32_t rs1,
                      uint32_t funct3, uint32_t rd, uint32_t opcode) {
    uint32_t instruction = 0;
    
    // Place each field at its correct position
    instruction |= (opcode & 0x7F);              // bits [6:0]
    instruction |= ((rd & 0x1F) << 7);           // bits [11:7]
    instruction |= ((funct3 & 0x07) << 12);      // bits [14:12]
    instruction |= ((rs1 & 0x1F) << 15);         // bits [19:15]
    instruction |= ((rs2 & 0x1F) << 20);         // bits [24:20]
    instruction |= ((funct7 & 0x7F) << 25);      // bits [31:25]
    
    return instruction;
}

int main(void) {
    printf("Testing pack/unpack of R-type instructions:\n\n");
    
    // Original instruction
    uint32_t original = 0x00A28233;  // add x4, x5, x10
    
    printf("Original instruction: 0x%08X\n", original);
    printf("\nExtracting fields:\n");
    
    uint32_t opcode = extract_field(original, 6, 0);
    uint32_t rd = extract_field(original, 11, 7);
    uint32_t funct3 = extract_field(original, 14, 12);
    uint32_t rs1 = extract_field(original, 19, 15);
    uint32_t rs2 = extract_field(original, 24, 20);
    uint32_t funct7 = extract_field(original, 31, 25);
    
    printf("  opcode:  0x%02X\n", opcode);
    printf("  rd:      x%u\n", rd);
    printf("  funct3:  %u\n", funct3);
    printf("  rs1:     x%u\n", rs1);
    printf("  rs2:     x%u\n", rs2);
    printf("  funct7:  0x%02X\n", funct7);
    
    printf("\nPacking back into instruction:\n");
    uint32_t packed = pack_r_type(funct7, rs2, rs1, funct3, rd, opcode);
    printf("Packed instruction: 0x%08X\n", packed);
    
    printf("\nVerification:\n");
    if (packed == original) {
        printf("✓ SUCCESS: Packed instruction matches original!\n");
    } else {
        printf("✗ FAILED: Packed instruction doesn't match\n");
        printf("  Expected: 0x%08X\n", original);
        printf("  Got:      0x%08X\n", packed);
    }
    
    return 0;
}
```

---

---

# Day 2: Pointers, Arrays & Memory Layout

## 2.1 Memory Layout of a C Program

### Understanding Virtual Address Space:

When your C program runs on Linux, the operating system gives it a **virtual address space** divided into segments:

```
┌─────────────────────────────────┐
│  High Addresses (0xFFFFFFFF)    │
├─────────────────────────────────┤
│        STACK                    │  ↓ grows downward
│   Local variables               │
│   Function arguments            │
│   Return addresses              │
├─────────────────────────────────┤
│   (unmapped memory)             │  Guard pages
├─────────────────────────────────┤
│        HEAP                     │  ↑ grows upward
│   malloc() allocations          │
│   calloc() allocations          │
├─────────────────────────────────┤
│        BSS                      │
│   Uninitialized globals         │
│   (zeroed at startup)           │
├─────────────────────────────────┤
│        DATA                     │
│   Initialized global variables  │
│   Static variables              │
├─────────────────────────────────┤
│        TEXT (Code)              │
│   Function instructions (read-only) │
│                                 │
│  Low Addresses (0x00000000)     │
└─────────────────────────────────┘
```

### Segment Explanations:

#### TEXT Segment (Code)
- Contains compiled machine instructions
- Read-only (prevents accidental code modification)
- Shared between multiple program instances
- Example: your `main()` function, `printf()` from libc

#### DATA Segment
- Initialized global and static variables
- Example: `int global_var = 42;`
- Preserved between program runs (on disk)
- Size known at compile time

#### BSS Segment (Block Started by Symbol)
- Uninitialized global and static variables
- Automatically zeroed (set to 0) at program startup
- Example: `int uninit_global;` (implicitly 0)
- Takes no space in executable file (only in running process)

#### HEAP
- Dynamically allocated memory (malloc, calloc)
- Managed manually by programmer
- Grows upward (increasing addresses)
- Must be freed to avoid memory leaks
- Errors here cause: memory leaks, use-after-free, buffer overflows

#### STACK
- Local variables (function scope)
- Function parameters
- Return addresses
- Grows downward (decreasing addresses)
- Automatic cleanup when function returns
- Stack overflow if too much allocated
- Very fast (hardware support)

### Example Program Demonstrating Segments:

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int global_initialized = 0xDEADBEEF;  // DATA segment
int global_uninitialized;               // BSS segment (implicitly 0)

void examine_addresses(void) {
    static int static_var = 0x12345678;  // DATA segment
    int local_var = 0xCAFEBABE;          // STACK
    
    int *heap_ptr = malloc(sizeof(int)); // HEAP
    if (heap_ptr) *heap_ptr = 0xBEEFCAFE;
    
    printf("TEXT address:       %p (main function)\n", (void *)&examine_addresses);
    printf("DATA address:       %p (global_initialized)\n", (void *)&global_initialized);
    printf("DATA address:       %p (static_var)\n", (void *)&static_var);
    printf("BSS address:        %p (global_uninitialized)\n", (void *)&global_uninitialized);
    printf("STACK address:      %p (local_var)\n", (void *)&local_var);
    printf("HEAP address:       %p (heap_ptr)\n", (void *)heap_ptr);
    
    free(heap_ptr);  // Always free malloc'd memory!
}

int main(void) {
    examine_addresses();
    return 0;
}
```

**Output (example, addresses vary):**
```
TEXT address:       0x10ac        (main function)
DATA address:       0x2000        (global_initialized)
DATA address:       0x2008        (static_var)
BSS address:        0x2010        (global_uninitialized)
STACK address:      0x7fffffffe000 (local_var)
HEAP address:       0x10b00       (heap_ptr)
```

### Hardware Connection:
This memory layout is exactly what your RISC-V processor implements! When you write a linker script for an FPGA SoC:
- `.text` placed at ROM addresses
- `.data` and `.bss` placed at RAM addresses
- Stack pointer initialized at top of RAM
Understanding this layout is understanding your processor's memory system.

---

## 2.2 Pointers — The Heart of C

### What is a Pointer?

A **pointer** is a variable that holds a **memory address**. When you dereference a pointer (use `*`), you access the data at that address.

Every pointer has **two attributes**:
1. **The address it stores** (memory location)
2. **The type of data** it points to (determines how many bytes to read/write)

### Pointer Basics:

```c
#include <stdio.h>
#include <stdint.h>

int main(void) {
    // Declare a 32-bit register value
    uint32_t register_value = 0xDEADBEEF;
    
    // Create a pointer to it
    uint32_t *ptr = &register_value;  // & = address-of operator
    
    printf("Value at register_value:     0x%08X\n", register_value);
    printf("Pointer ptr stores address:  %p\n", (void *)ptr);
    printf("Dereference *ptr:            0x%08X\n", *ptr);  // Read value
    
    // Modify value through pointer
    *ptr = 0xCAFEBABE;
    printf("After *ptr = 0xCAFEBABE:    0x%08X\n", register_value);
    
    // NULL pointer
    uint32_t *null_ptr = NULL;
    // *null_ptr = 42;  // CRASH! Segmentation fault
    
    printf("Size of pointer: %zu bytes\n", sizeof(ptr));  // 8 on 64-bit
    
    return 0;
}
```

**Key Operators:**
- `&variable` - Address-of: gets the address of a variable
- `*pointer` - Dereference: accesses the value at the address
- `->` - Arrow operator: shorthand for `(*struct_ptr).field`

### Pointer Types Matter:

```c
uint32_t value = 0xDEADBEEF;

// Different pointer types point to same address but interpret differently
uint32_t *word_ptr = &value;      // Points to 4 bytes
uint8_t *byte_ptr = (uint8_t *)&value;  // Points to 1 byte

printf("*word_ptr = 0x%08X\n", *word_ptr);  // 0xDEADBEEF
printf("*byte_ptr = 0x%02X\n", *byte_ptr);  // 0xEF (on little-endian)
```

### Pointer Arithmetic:

When you add 1 to a pointer, it advances by **`sizeof(type)`** bytes, not 1 byte:

```c
uint32_t memory[8] = {0};  // Array of 8 32-bit values

uint32_t *word_ptr = memory;  // Points to first element
printf("word_ptr + 0: address %p\n", (void *)(word_ptr + 0));  // address of memory[0]
printf("word_ptr + 1: address %p\n", (void *)(word_ptr + 1));  // address of memory[1]
// word_ptr + 1 advances by 4 bytes (sizeof(uint32_t))

// Byte-level access:
uint8_t *byte_ptr = (uint8_t *)memory;
printf("byte_ptr + 1: address %p\n", (void *)(byte_ptr + 1));  // +1 byte
```

### Pointers to Pointers:

```c
uint32_t value = 0xDEADBEEF;
uint32_t *ptr1 = &value;           // Pointer to value
uint32_t **ptr2 = &ptr1;            // Pointer to pointer

printf("value = 0x%08X\n", value);        // 0xDEADBEEF
printf("*ptr1 = 0x%08X\n", *ptr1);        // 0xDEADBEEF
printf("**ptr2 = 0x%08X\n", **ptr2);      // 0xDEADBEEF

// Modify through double pointer
**ptr2 = 0xCAFEBABE;
printf("value = 0x%08X\n", value);        // 0xCAFEBABE
```

### void Pointers:

`void *` is a generic pointer - points to "some data" but type is unknown:

```c
void *generic = NULL;

int int_val = 42;
generic = &int_val;  // Can assign to any type
int *int_ptr = (int *)generic;  // Must cast to use
printf("int value: %d\n", *int_ptr);

// Useful for: generic data structures, memory allocation (malloc returns void *)
void *allocated = malloc(128);  // Returns void *, must cast
uint32_t *data = (uint32_t *)allocated;
```

### Hardware Connection: Memory-Mapped I/O

In embedded RISC-V systems, hardware peripherals are accessed via specific memory addresses:

```c
// UART data register at address 0x10000000
volatile uint32_t *uart_data = (volatile uint32_t *)0x10000000;

// Send character to UART
char c = 'A';
*uart_data = c;  // Write 1 byte (lower 8 bits)

// Receive character from UART
char received = (char)*uart_data;
```

The `volatile` keyword tells compiler: "This value may change unexpectedly (by hardware), don't optimize."

---

## 2.3 Arrays

### Array Basics:

An array is a contiguous block of memory holding multiple elements of the same type:

```c
// Declare array of 32 RISC-V registers
uint32_t registers[32] = {0};  // All initialized to 0

// Access elements with indexing
registers[0] = 0;           // x0 is always 0 in RISC-V
registers[2] = 0x7FFFFFF0; // sp (stack pointer)
registers[10] = 42;         // x10 (general purpose)

// Array name IS a pointer to first element
uint32_t *reg_ptr = registers;  // No & needed!
printf("%p == %p\n", (void *)registers, (void *)reg_ptr);  // Same address

// Equivalence: registers[i] == *(registers + i)
printf("registers[5] = 0x%08X\n", registers[5]);
printf("*(registers+5) = 0x%08X\n", *(registers + 5));  // Identical
```

### 2D Arrays:

```c
// 4 cache lines, 64 bytes each
uint8_t cache[4][64];

// First cache line, first byte
cache[0][0] = 0xFF;

// Second cache line
uint8_t *line_ptr = cache[1];  // Pointer to line
line_ptr[10] = 0xAA;

// Row-major: elements stored row by row in memory
// cache[0][0], cache[0][1], ..., cache[0][63],
// cache[1][0], cache[1][1], ..., cache[1][63],
```

### Array Size Calculation:

```c
uint32_t registers[32];

// Number of elements
size_t count = sizeof(registers) / sizeof(registers[0]);
printf("Number of registers: %zu\n", count);  // 32

// This only works for static arrays! For function parameters or malloc'd arrays,
// you must track the size separately.
```

### Critical Warning: No Bounds Checking!

```c
uint32_t registers[32];

// C does NOT check bounds
registers[32] = 0xFF;  // ERROR! Out of bounds - corrupts memory
registers[100] = 0xFF; // ERROR! Even worse

// Stack smashing attack vector - security vulnerability
// Always validate indices before accessing arrays
```

### Using Arrays with Functions:

```c
// Arrays decay to pointers in function parameters
void clear_registers(uint32_t regs[], size_t count) {
    for (size_t i = 0; i < count; i++) {
        regs[i] = 0;
    }
}

// Called as:
uint32_t my_regs[32];
clear_registers(my_regs, 32);  // Pass array and count

// sizeof() inside function shows pointer size, not array size!
// That's why we need count parameter
```

---

## 2.4 Strings in C

### String Fundamentals:

A C **string** is simply a **null-terminated array of `char`**. The null terminator `\0` (byte value 0x00) marks the end.

```c
// String literal (read-only)
char *name = "RISC-V";  // Stored in TEXT segment
// Memory: 'R' 'I' 'S' 'C' '-' 'V' '\0'

// Mutable string (on stack)
char mnemonic[] = "ADD";  // Copy made on stack
// Memory: 'A' 'D' 'D' '\0'

// Strings are just arrays
printf("First char: %c\n", mnemonic[0]);      // 'A'
printf("Length: %zu\n", strlen(mnemonic));     // 3
```

### Common String Functions (from `<string.h>`):

```c
#include <string.h>

// String length (not counting null terminator)
size_t len = strlen("RISC-V");  // Returns 6

// Compare strings (< 0 if s1 < s2, 0 if equal, > 0 if s1 > s2)
int cmp = strcmp("ADD", "SUB");  // < 0 because "ADD" < "SUB"

// Dangerous - no size check:
strcpy(dest, src);  // DON'T USE! Can overflow

// Safer - limited copy:
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';  // Ensure null termination

// Safe formatted copy:
snprintf(buf, sizeof(buf), "x%u = 0x%08X", reg_num, value);
```

### Buffer Overflow - Critical Security Issue:

```c
// DON'T DO THIS:
char buffer[16];  // Only 16 bytes including null terminator
strcpy(buffer, "This is a much longer string that exceeds 16 bytes!");
// Result: Stack corruption, potential crash or exploitation

// DO THIS:
char buffer[16];
strncpy(buffer, "This is a much longer string", sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';  // Ensure null termination
```

---

## 2.5 Functions with Pointers (Pass-by-Reference)

### The Problem: Pass-by-Value

C is **always pass-by-value**. When you pass a variable to a function, the function receives a **copy**:

```c
void increment(int x) {
    x++;  // Increments local copy, doesn't affect caller's x
}

int main(void) {
    int value = 10;
    increment(value);
    printf("%d\n", value);  // Still 10, not 11!
    return 0;
}
```

### The Solution: Pointers (Pass-by-Reference)

To modify caller's data, pass a **pointer**:

```c
void increment(int *x) {
    (*x)++;  // Dereference pointer, then increment
}

int main(void) {
    int value = 10;
    increment(&value);  // Pass address
    printf("%d\n", value);  // Now 11!
    return 0;
}
```

### Practical Examples: Swapping and Clearing

```c
// Swap two register values
void swap_registers(uint32_t *a, uint32_t *b) {
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}

// Clear all registers to zero
void clear_registers(uint32_t regs[], size_t count) {
    for (size_t i = 0; i < count; i++) {
        regs[i] = 0;  // Arrays are already pointers
    }
}

// Usage:
uint32_t registers[32];
clear_registers(registers, 32);
```

### const Correctness:

The `const` keyword promises not to modify data through the pointer:

```c
// Print memory without modifying it
void print_memory(const uint8_t *mem, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", mem[i]);
    }
    printf("\n");
}

uint8_t my_mem[256];
print_memory(my_mem, 256);  // Compiler ensures my_mem isn't modified

// This wouldn't compile:
void modify_const_ptr(const uint8_t *mem) {
    mem[0] = 0xFF;  // ERROR: assignment of read-only location
}
```

---

# Day 2 Exercise Solutions

## Exercise 1: Memory Layout Examination

**Problem:** Declare variables in different segments and print their addresses.

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Global initialized - DATA segment
int global_initialized = 0xDEADBEEF;

// Global uninitialized - BSS segment
int global_uninitialized;

int main(void) {
    // Local variable - STACK
    int local_var = 42;
    
    // Static - DATA segment
    static int static_var = 0x12345678;
    
    // Dynamic - HEAP
    int *heap_var = malloc(sizeof(int));
    if (heap_var) *heap_var = 0xCAFEBABE;
    
    printf("MEMORY LAYOUT EXAMINATION:\n");
    printf("==========================\n\n");
    
    printf("TEXT Segment (Code):\n");
    printf("  main function:          %p\n", (void *)&main);
    printf("\nDATA Segment (Initialized globals):\n");
    printf("  global_initialized:     %p (value: 0x%08X)\n", 
           (void *)&global_initialized, global_initialized);
    printf("  static_var:             %p (value: 0x%08X)\n", 
           (void *)&static_var, static_var);
    
    printf("\nBSS Segment (Uninitialized globals):\n");
    printf("  global_uninitialized:   %p (value: %d)\n", 
           (void *)&global_uninitialized, global_uninitialized);
    
    printf("\nSTACK Segment (Local variables):\n");
    printf("  local_var:              %p (value: %d)\n", 
           (void *)&local_var, local_var);
    
    printf("\nHEAP Segment (Dynamically allocated):\n");
    printf("  heap_var (pointer):     %p\n", (void *)heap_var);
    printf("  heap_var (address):     %p (value: 0x%08X)\n", 
           (void *)heap_var, *heap_var);
    
    printf("\nAddressMEMORY ORDER:\n");
    if ((uintptr_t)&main < (uintptr_t)&global_initialized)
        printf("  TEXT < DATA ✓\n");
    if ((uintptr_t)&global_initialized < (uintptr_t)&global_uninitialized)
        printf("  DATA < BSS ✓\n");
    if ((uintptr_t)heap_var > (uintptr_t)&local_var)
        printf("  STACK > HEAP ✓\n");
    
    free(heap_var);
    return 0;
}
```

---

## Exercise 2: Register File with Pointer Functions

**Problem:** Simulated 32-register file with write_reg and read_reg functions.

```c
#include <stdio.h>
#include <stdint.h>

// Write to register (with x0=0 enforcement)
void write_reg(uint32_t *regs, uint8_t rd, uint32_t value) {
    if (rd == 0) {
        // x0 is always 0 in RISC-V - ignore writes
        return;
    }
    if (rd < 32) {
        regs[rd] = value;
    }
}

// Read from register
uint32_t read_reg(const uint32_t *regs, uint8_t rs) {
    if (rs < 32) {
        return regs[rs];
    }
    return 0;  // Invalid register returns 0
}

// Print all registers
void print_registers(const uint32_t *regs) {
    for (int i = 0; i < 32; i++) {
        printf("x%2d = 0x%08X", i, regs[i]);
        if ((i + 1) % 4 == 0) {
            printf("\n");
        } else {
            printf("  |  ");
        }
    }
}

int main(void) {
    // Simulated register file
    uint32_t registers[32] = {0};
    
    printf("Initial state (all zeros):\n");
    print_registers(registers);
    printf("\n");
    
    // Write some values
    write_reg(registers, 0, 0xDEADBEEF);   // Should be ignored (x0)
    write_reg(registers, 1, 0x11111111);
    write_reg(registers, 2, 0x22222222);
    write_reg(registers, 10, 0xCAFEBABE);
    write_reg(registers, 31, 0xFFFFFFFF);
    
    printf("After writes:\n");
    print_registers(registers);
    printf("\n");
    
    // Verify x0 is still 0
    printf("Read tests:\n");
    printf("  x0 = 0x%08X (should be 0)\n", read_reg(registers, 0));
    printf("  x1 = 0x%08X\n", read_reg(registers, 1));
    printf("  x2 = 0x%08X\n", read_reg(registers, 2));
    printf("  x10 = 0x%08X\n", read_reg(registers, 10));
    printf("  x31 = 0x%08X\n", read_reg(registers, 31));
    
    return 0;
}
```

---

## Exercise 3: Memory Dump Function

**Problem:** Print memory as hex dump with addresses (like xxd).

```c
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

void memory_dump(const uint8_t *mem, size_t size) {
    printf("Address  | Hex Data                         | ASCII\n");
    printf("---------+----------------------------------+------------------\n");
    
    for (size_t i = 0; i < size; i += 16) {
        // Print address
        printf("0x%06zX | ", i);
        
        // Print hex bytes
        for (int j = 0; j < 16; j++) {
            if (i + j < size) {
                printf("%02X ", mem[i + j]);
            } else {
                printf("   ");
            }
            // Extra space after 8 bytes
            if (j == 7) printf(" ");
        }
        
        printf("| ");
        
        // Print ASCII representation
        for (int j = 0; j < 16 && i + j < size; j++) {
            char c = mem[i + j];
            printf("%c", isprint(c) ? c : '.');
        }
        printf("\n");
    }
}

int main(void) {
    // Test data
    uint8_t test_memory[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        'H',  'e',  'l',  'l',  'o',  ',',  ' ',  'W',
        'o',  'r',  'l',  'd',  '!',  0x00, 0x00, 0x00,
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
    };
    
    printf("Memory dump of test data:\n\n");
    memory_dump(test_memory, sizeof(test_memory));
    
    return 0;
}
```

**Output:**
```
Address  | Hex Data                         | ASCII
---------+----------------------------------+------------------
0x000000 | DE AD BE EF CA FE BA BE  48 65 6C 6C 6F 2C 20 57 | ........Hello, W
0x000010 | 6F 72 6C 64 21 00 00 00  12 34 56 78 9A BC DE F0 | orld!.......4Vx..
```

---

## Exercise 4: In-Place Array Reversal with Pointers

**Problem:** Reverse array in-place using pointer arithmetic (no `[]`).

```c
#include <stdio.h>
#include <stdint.h>

void reverse_array(uint32_t *arr, size_t size) {
    uint32_t *left = arr;                // Pointer to start
    uint32_t *right = arr + size - 1;    // Pointer to end
    
    // Swap elements from outside to inside
    while (left < right) {
        // Swap using pointers only (no indexing)
        uint32_t temp = *left;
        *left = *right;
        *right = temp;
        
        left++;    // Move left pointer forward
        right--;   // Move right pointer backward
    }
}

void print_array(const uint32_t *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%u ", *(arr + i));  // Using pointer arithmetic
    }
    printf("\n");
}

int main(void) {
    uint32_t values[] = {1, 2, 3, 4, 5};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    printf("Original array:\n");
    print_array(values, count);
    
    printf("After reversal:\n");
    reverse_array(values, count);
    print_array(values, count);
    
    return 0;
}
```

**Output:**
```
Original array:
1 2 3 4 5
After reversal:
5 4 3 2 1
```

---

## Exercise 5: Safe String Concatenation

**Problem:** Implement strcat_safe() that concatenates strings without buffer overflow.

```c
#include <stdio.h>
#include <string.h>

// Safe string concatenation
// Returns: 0 on success, -1 on error
int strcat_safe(char *dest, size_t dest_size, const char *src) {
    // Validate inputs
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    // Find end of destination string
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    // Check if concatenation would exceed buffer
    if (dest_len + src_len >= dest_size) {
        printf("Error: Buffer too small (need %zu bytes, have %zu)\n",
               dest_len + src_len + 1, dest_size);
        return -1;
    }
    
    // Append source to destination
    // dest_size - dest_len - 1 is available space for src + null terminator
    strncpy(&dest[dest_len], src, dest_size - dest_len - 1);
    dest[dest_size - 1] = '\0';  // Ensure null termination
    
    return 0;
}

int main(void) {
    char buffer[32] = {0};
    
    printf("Test 1: Safe concatenation\n");
    strcat_safe(buffer, sizeof(buffer), "Hello");
    printf("After \"Hello\": \"%s\"\n", buffer);
    
    strcat_safe(buffer, sizeof(buffer), " ");
    strcat_safe(buffer, sizeof(buffer), "World");
    printf("After \" World\": \"%s\"\n\n", buffer);
    
    printf("Test 2: Buffer overflow attempt\n");
    char small_buffer[10] = "Start";
    if (strcat_safe(small_buffer, sizeof(small_buffer), 
                    " this is a very long string that wont fit") == -1) {
        printf("Concatenation prevented (buffer protected)\n");
    }
    printf("Buffer still contains: \"%s\"\n", small_buffer);
    
    return 0;
}
```

---

## Exercise 6 (Bonus): Memory Simulation with Load/Store

**Problem:** Simulate 256-byte memory with load_word/store_word functions.

```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 256

// Alignment check (addresses must be 4-byte aligned)
static inline int is_aligned(uint32_t addr) {
    return (addr & 0x3) == 0;  // Lower 2 bits must be 0
}

// Load 32-bit word from memory
// Returns: 0 on success, -1 on error
int load_word(const uint8_t *mem, uint32_t addr, uint32_t *value) {
    // Check bounds
    if (addr + 4 > MEMORY_SIZE) {
        printf("Error: Address 0x%08X out of bounds\n", addr);
        return -1;
    }
    
    // Check alignment
    if (!is_aligned(addr)) {
        printf("Error: Address 0x%08X not aligned (must be 4-byte aligned)\n", addr);
        return -1;
    }
    
    // Load bytes in little-endian order
    *value = mem[addr + 0] |
             (mem[addr + 1] << 8) |
             (mem[addr + 2] << 16) |
             (mem[addr + 3] << 24);
    
    return 0;
}

// Store 32-bit word to memory
int store_word(uint8_t *mem, uint32_t addr, uint32_t value) {
    // Check bounds
    if (addr + 4 > MEMORY_SIZE) {
        printf("Error: Address 0x%08X out of bounds\n", addr);
        return -1;
    }
    
    // Check alignment
    if (!is_aligned(addr)) {
        printf("Error: Address 0x%08X not aligned\n", addr);
        return -1;
    }
    
    // Store bytes in little-endian order
    mem[addr + 0] = (value & 0x000000FF);
    mem[addr + 1] = (value & 0x0000FF00) >> 8;
    mem[addr + 2] = (value & 0x00FF0000) >> 16;
    mem[addr + 3] = (value & 0xFF000000) >> 24;
    
    return 0;
}

int main(void) {
    uint8_t memory[MEMORY_SIZE] = {0};
    uint32_t value;
    
    printf("Simulated 256-byte Memory Test\n");
    printf("================================\n\n");
    
    // Test 1: Aligned store and load
    printf("Test 1: Aligned 4-byte operations\n");
    if (store_word(memory, 0x00, 0xDEADBEEF) == 0) {
        printf("✓ Stored 0xDEADBEEF at address 0x00\n");
    }
    
    if (load_word(memory, 0x00, &value) == 0) {
        printf("✓ Loaded value: 0x%08X\n\n", value);
    }
    
    // Test 2: Multiple stores
    printf("Test 2: Multiple stores\n");
    store_word(memory, 0x04, 0xCAFEBABE);
    store_word(memory, 0x08, 0x12345678);
    store_word(memory, 0x0C, 0xFFFFFFFF);
    printf("✓ Stored values at offsets 0x04, 0x08, 0x0C\n\n");
    
    printf("Test 3: Load back\n");
    for (uint32_t addr = 0x00; addr < 0x10; addr += 4) {
        if (load_word(memory, addr, &value) == 0) {
            printf("  Memory[0x%02X] = 0x%08X\n", addr, value);
        }
    }
    printf("\n");
    
    // Test 4: Unaligned access (error)
    printf("Test 4: Unaligned address (should fail)\n");
    if (load_word(memory, 0x01, &value) == -1) {
        printf("✓ Correctly rejected unaligned address\n\n");
    }
    
    // Test 5: Out of bounds (error)
    printf("Test 5: Out of bounds access (should fail)\n");
    if (store_word(memory, 0xFE, 0x11223344) == -1) {
        printf("✓ Correctly rejected out-of-bounds access\n");
    }
    
    return 0;
}
```

---

## Summary of Key Concepts

### Day 1 Key Takeaways:
1. **Compilation Pipeline**: C → Preprocessed C → Assembly → Object file → Executable
2. **Fixed-Width Types**: Always use `uint32_t`, `int32_t`, etc. from `<stdint.h>`
3. **Bitwise Operations**: AND, OR, XOR, NOT, shifts - fundamental for hardware
4. **Instruction Decoding**: Extract fields from 32-bit instructions using bit masking

### Day 2 Key Takeaways:
1. **Memory Layout**: TEXT, DATA, BSS, HEAP, STACK
2. **Pointers**: Hold addresses, dereference with `*`, take address with `&`
3. **Arrays**: Contiguous memory blocks, no bounds checking
4. **Strings**: Null-terminated character arrays, need size for safe operations
5. **Pass-by-Reference**: Use pointers to modify caller's data

### Hardware Applications:
- Register modeling with arrays and pointers
- Instruction decoding with bitwise operations
- Memory-mapped I/O with volatile pointers
- DMA and buffer management with pointer arithmetic

---

**Document Created for MEDS Module 2 - C Language for Hardware Engineers**
**Days 1 & 2 Complete Study Guide with Explanations and Exercises**
