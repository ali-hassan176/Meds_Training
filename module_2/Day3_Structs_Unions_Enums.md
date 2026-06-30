# Day 3 — Structs, Unions, Enums & The Hardware Connection

**MEDS Module 2 | C Language for Hardware Engineers**
**Ali Hassan | Roll No. 2024-EE-176 | UET Lahore**

---

## What Is This Day About?

Before Day 3, you worked with simple variables: `int x = 5;`, `uint32_t address = 0x1000;`. These hold **one piece of information** at a time.

Real hardware has **many related pieces of information grouped together**. A RISC-V instruction has an opcode, registers, an immediate, a function code — all part of one 32-bit word. A UART peripheral has a control register, a status register, a transmit buffer, and a receive buffer — all part of one chip.

Day 3 teaches you three tools to model this in C:

| Tool    | Purpose                                          | Real-World Analogy                              |
|---------|--------------------------------------------------|-------------------------------------------------|
| `struct` | Group different variables under one name         | A form with multiple fields (name, age, ID)     |
| `enum`   | Give human-readable names to numeric constants   | Traffic light states: RED=0, YELLOW=1, GREEN=2  |
| `union`  | View the same memory block through different lenses | One coin seen as heads or tails, same coin       |

---

## 6.1 Structs — Grouping Related Data

### The Problem Without Structs

Imagine tracking a decoded RISC-V instruction without a struct:

```c
uint32_t instr_opcode;   /* which operation?               */
uint32_t instr_rd;       /* which destination register?    */
uint32_t instr_rs1;      /* which source register 1?       */
uint32_t instr_rs2;      /* which source register 2?       */
int32_t  instr_imm;      /* what is the immediate value?   */
```

These five separate variables are hard to pass around. If you have 100 instructions, you need 500 variables. Structs solve this.

### Defining a Struct

```c
/* 'typedef struct' creates a new type name.
   Without typedef, you'd have to write 'struct decoded_instr_t' every time.
   With typedef, you just write 'decoded_instr_t'.                           */
typedef struct {

    uint32_t opcode;   /* bits [6:0]  — which family of instruction is this? */
    uint32_t rd;       /* bits [11:7] — destination register index (0–31)    */
    uint32_t funct3;   /* bits [14:12]— narrows down the exact instruction    */
    uint32_t rs1;      /* bits [19:15]— first source register index (0–31)   */
    uint32_t rs2;      /* bits [24:20]— second source register index (0–31)  */
    uint32_t funct7;   /* bits [31:25]— further narrows for R-type only      */
    int32_t  imm;      /* sign-extended immediate value — int32 because it
                          can be negative (e.g., branch offset = -8)          */

} decoded_instr_t;     /* This is the type name — like 'int' but your own type */
```

### Creating and Using a Struct

```c
decoded_instr_t instr;   /* Declare one variable of type decoded_instr_t.
                            This reserves memory for ALL fields at once.
                            The total size = sum of all field sizes (+ padding, see below). */

/* Access fields using the dot operator '.' */
instr.opcode = 0x33;   /* Set the opcode field to 0x33 (OP_REG = R-type instructions) */
instr.rd     = 4;       /* Instruction writes its result to register x4                */
instr.rs1    = 5;       /* First operand comes from register x5                        */
instr.rs2    = 10;      /* Second operand comes from register x10                      */
```

### Pointers to Structs — The Arrow Operator `->`

This pattern is everywhere in C. Instead of passing a whole struct to a function (which copies it), you pass a pointer (just an address — 4 or 8 bytes).

```c
/* Create the struct and get its address */
decoded_instr_t instr;
decoded_instr_t *p = &instr;   /* p now holds the memory address of 'instr'
                                   '&' means "address of"
                                   '*' in the type means "pointer to decoded_instr_t" */

/* Two ways to access fields through a pointer — both are identical: */
(*p).opcode = 0x13;   /* Dereference p to get the struct, then access .opcode
                         The parentheses are needed because '.' binds tighter than '*' */

p->opcode = 0x13;     /* Arrow operator: shorthand for (*p).opcode
                         This is what everyone uses. Same result, shorter to write.
                         Read it as: "the opcode field of whatever p points to"       */
```

**Why use a pointer instead of a copy?**
- A function receiving `decoded_instr_t instr` gets a **copy** — changes inside the function don't affect the original.
- A function receiving `decoded_instr_t *p` gets the **original's address** — changes inside DO affect the original.
- Also, copying a large struct is slow. Passing a pointer (always 8 bytes) is always fast.

---

### Struct Memory Layout and Padding

This is critical knowledge for **hardware register programming**.

The compiler does not always pack struct fields back-to-back. It inserts invisible **padding bytes** between fields to ensure each field sits at its "natural alignment" (a `uint32_t` must start at an address divisible by 4, etc.).

```c
/* BAD order — wastes memory due to padding */
struct example {
    uint8_t  a;    /* 1 byte at offset 0                                   */
                   /* 3 bytes of INVISIBLE PADDING inserted by compiler     */
    uint32_t b;    /* 4 bytes at offset 4 (must be 4-byte aligned)         */
    uint8_t  c;    /* 1 byte at offset 8                                   */
                   /* 3 bytes of INVISIBLE PADDING at end for alignment     */
};
/* sizeof(struct example) = 12, NOT 6 — padding doubled the expected size! */

/* GOOD order — reorder fields from largest to smallest, minimises padding */
struct example_packed {
    uint32_t b;    /* 4 bytes at offset 0 — naturally aligned at start    */
    uint8_t  a;    /* 1 byte at offset 4                                  */
    uint8_t  c;    /* 1 byte at offset 5                                  */
                   /* 2 bytes of padding at end to make total a multiple of 4 */
};
/* sizeof(struct example_packed) = 8 — still has some padding but much less */
```

**Hardware register connection:** When you write a struct to map a peripheral's memory-mapped registers, the struct field at offset 0 maps to the first register, at offset 4 to the second register, etc. If padding sneaks in, your offset is wrong and you read the wrong register — a hardware bug that is very hard to diagnose.

---

## 6.2 Enums — Named Constants

### The Problem Without Enums

```c
/* Magic numbers everywhere — what does 0x33 mean? What does 0x03 mean? */
if (opcode == 0x33) { /* ... */ }
if (opcode == 0x03) { /* ... */ }
```

Enums replace magic numbers with readable names.

### Defining an Enum

```c
/* 'typedef enum' creates a named type for a group of integer constants.
   Under the hood, each name is just an integer.
   OP_R_TYPE is 0x33, OP_LOAD is 0x03, etc. — the compiler substitutes the number. */
typedef enum {

    OP_R_TYPE  = 0x33,   /* opcode for ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU */
    OP_I_TYPE  = 0x13,   /* opcode for ADDI, SLTI, ANDI, ORI, XORI, SLLI, SRLI, SRAI    */
    OP_LOAD    = 0x03,   /* opcode for LB, LH, LW, LBU, LHU                              */
    OP_STORE   = 0x23,   /* opcode for SB, SH, SW                                        */
    OP_BRANCH  = 0x63,   /* opcode for BEQ, BNE, BLT, BGE, BLTU, BGEU                   */
    OP_JAL     = 0x6F,   /* opcode for JAL (Jump And Link)                               */
    OP_JALR    = 0x67,   /* opcode for JALR (Jump And Link Register)                     */
    OP_LUI     = 0x37,   /* opcode for LUI (Load Upper Immediate)                        */
    OP_AUIPC   = 0x17,   /* opcode for AUIPC (Add Upper Immediate to PC)                 */
    OP_SYSTEM  = 0x73    /* opcode for ECALL, EBREAK, CSR instructions                   */

} opcode_t;              /* The type name — use 'opcode_t' where you'd use 'int'         */

/* ALU operation codes — used inside the CPU datapath to tell the ALU what to do.
   Notice these start from 0 automatically (no '=' needed when you want 0, 1, 2, ...) */
typedef enum {
    ALU_ADD,    /* 0 — addition:            result = a + b         */
    ALU_SUB,    /* 1 — subtraction:         result = a - b         */
    ALU_AND,    /* 2 — bitwise AND:         result = a & b         */
    ALU_OR,     /* 3 — bitwise OR:          result = a | b         */
    ALU_XOR,    /* 4 — bitwise XOR:         result = a ^ b         */
    ALU_SLL,    /* 5 — shift left logical:  result = a << b[4:0]   */
    ALU_SRL,    /* 6 — shift right logical: result = a >> b[4:0] (fills with 0s)  */
    ALU_SRA,    /* 7 — shift right arith:   result = a >> b[4:0] (fills with sign) */
    ALU_SLT,    /* 8 — set less than (signed):   result = (a < b) ? 1 : 0          */
    ALU_SLTU    /* 9 — set less than (unsigned):  result = (a < b) ? 1 : 0          */
} alu_op_t;
```

### Using Enums in a Switch Statement

```c
/* Cast opcode to opcode_t type so the compiler can match against our enum names.
   'switch' compares one value against many 'case' labels.
   Without 'break', execution falls through to the next case (usually a bug).        */
switch ((opcode_t)opcode) {

    case OP_R_TYPE:
        /* Handle R-type instructions: ADD, SUB, AND, OR, XOR, ... */
        break;   /* STOP here — do not fall into the next case */

    case OP_LOAD:
        /* Handle load instructions: LB, LH, LW, ... */
        break;

    case OP_STORE:
        /* Handle store instructions: SB, SH, SW */
        break;

    case OP_BRANCH:
        /* Handle branch instructions: BEQ, BNE, BLT, ... */
        break;

    default:
        /* Catches any opcode not listed above — UNKNOWN instruction */
        printf("Unknown opcode: 0x%02X\n", opcode);
        break;
}
```

---

## 6.3 Unions — Same Memory, Different Views

### What Is a Union?

A `struct` gives each field its **own separate memory**.
A `union` makes all fields **share the same memory**.

Think of it this way: a 32-bit RISC-V instruction is just 32 bits. Depending on its format (R-type, I-type, etc.), those 32 bits mean different things. A union lets you read those same 32 bits as either format without copying or converting.

```c
/* A union that views a 32-bit instruction three different ways:
   1. As a raw uint32_t (the whole word)
   2. As R-type fields (using bit-fields)
   3. As I-type fields (using bit-fields)
   ALL THREE share the same 4 bytes of memory.                                        */
typedef union {

    uint32_t raw;   /* View 1: the entire 32-bit instruction as a single number.
                       Loading 'raw' = loading the whole instruction in one go.        */

    struct {                         /* View 2: R-type instruction layout              */
        uint32_t opcode : 7;         /* bits [6:0]   — 7 bits for opcode               */
        uint32_t rd     : 5;         /* bits [11:7]  — 5 bits for destination register */
        uint32_t funct3 : 3;         /* bits [14:12] — 3 bits for function code        */
        uint32_t rs1    : 5;         /* bits [19:15] — 5 bits for source register 1    */
        uint32_t rs2    : 5;         /* bits [24:20] — 5 bits for source register 2    */
        uint32_t funct7 : 7;         /* bits [31:25] — 7 bits for function code 2      */
                                     /* The 'N :' syntax declares a bit-field of N bits */
    } r_type;                        /* Access this view as 'inst.r_type.rd' etc.       */

    struct {                         /* View 3: I-type instruction layout              */
        uint32_t opcode : 7;         /* bits [6:0]   — same opcode position             */
        uint32_t rd     : 5;         /* bits [11:7]  — same rd position                 */
        uint32_t funct3 : 3;         /* bits [14:12] — same funct3 position             */
        uint32_t rs1    : 5;         /* bits [19:15] — same rs1 position                */
        uint32_t imm    : 12;        /* bits [31:20] — 12-bit immediate value
                                        (where rs2+funct7 are in R-type, here it's imm) */
    } i_type;                        /* Access as 'inst.i_type.imm' etc.                */

} instruction_t;   /* Total size = 4 bytes (size of the largest member: uint32_t raw) */
```

### Using a Union

```c
instruction_t inst;            /* Declare the union — allocates 4 bytes total            */

inst.raw = 0x00A28233;         /* Load the raw instruction: write all 32 bits at once.
                                  0x00A28233 = ADD x4, x5, x10 in RISC-V machine code.
                                  NOW all three views are valid for reading.              */

/* Read as R-type — no calculation needed! The bit-field struct handles it. */
printf("rd  = x%u\n", inst.r_type.rd);    /* prints: rd  = x4  */
printf("rs1 = x%u\n", inst.r_type.rs1);   /* prints: rs1 = x5  */
printf("rs2 = x%u\n", inst.r_type.rs2);   /* prints: rs2 = x10 */
```

### Portability Warning

Bit-fields are **implementation-defined**. The C standard does not guarantee which order bit-fields appear in memory. On most x86/ARM/RISC-V platforms with GCC they work as expected, but for portable production code, use `EXTRACT_BITS` shifts and masks (as done in the Grand Assignment).

Bit-field unions are excellent for **debugging and visualisation** — you can inspect any field of a raw instruction instantly in GDB.

---

## 6.4 The Complete RISC-V CPU State

This brings everything together: a `struct` that models the entire state of a RISC-V CPU.

```c
/* The full state of a RISC-V RV32I CPU in one struct.
   A real CPU has exactly these pieces of state (plus some CSRs we ignore here).     */
typedef struct {

    uint32_t x[32];      /* The 32 general-purpose registers: x0 through x31.
                            x[0] is always 0 (writes to it are ignored).
                            x[1] is the return address (ra).
                            x[2] is the stack pointer (sp).
                            Stored as an array: x[i] accesses register xi.            */

    uint32_t pc;         /* Program Counter — address of the NEXT instruction to fetch.
                            Starts at 0x00000000. Increments by 4 each cycle (usually). */

    uint8_t *memory;     /* Pointer to the simulated RAM.
                            This is a pointer (address), not the RAM itself.
                            The actual RAM is allocated separately with malloc/calloc.
                            uint8_t* = "array of bytes" — memory is byte-addressable.  */

    size_t mem_size;     /* Total size of the simulated memory in bytes.
                            'size_t' is the correct type for sizes/counts — it is
                            unsigned and large enough for any object size on the platform. */

    uint64_t instr_count;  /* Number of instructions executed so far.
                              uint64_t (64-bit) because a long simulation runs billions
                              of instructions — uint32_t would overflow in ~4 billion.   */

    uint64_t cycle_count;  /* Number of clock cycles simulated.
                              Useful for performance analysis and CPI measurement.       */

} cpu_state_t;
```

### CPU Initialisation Function

```c
/* cpu_init — sets the CPU to its reset state.
   Parameters:
     cpu      — pointer to the cpu_state_t to initialise (not a copy!)
     mem_size — how many bytes of RAM to allocate for this simulation              */
void cpu_init(cpu_state_t *cpu, size_t mem_size)
{
    /* memset(pointer, value, count) — fills 'count' bytes at 'pointer' with 'value'.
       sizeof(cpu->x) = 32 * 4 = 128 bytes.
       Setting all 128 bytes to 0 initialises all registers to 0.
       This matches hardware reset: on power-up, registers are typically 0.          */
    memset(cpu->x, 0, sizeof(cpu->x));

    cpu->pc = 0x00000000;   /* RISC-V reset vector — first instruction fetched from here */

    /* calloc(count, size) — allocates count*size bytes AND fills them all with 0.
       This is 'mem_size' bytes total.
       Using calloc (not malloc) ensures memory starts as all-zeros — clean slate.
       calloc returns a void* which implicitly converts to uint8_t*.                 */
    cpu->memory = calloc(mem_size, 1);

    cpu->mem_size   = mem_size;   /* Store so bounds-checks know the memory limit     */
    cpu->instr_count = 0;          /* Haven't executed anything yet                   */
    cpu->cycle_count = 0;          /* Haven't run any cycles yet                      */
}

/* reg_write — writes a value to a register, enforcing the x0=0 rule.
   In RISC-V, x0 is hardwired to 0. Writing to it must be silently ignored.
   Parameters:
     cpu   — pointer to CPU state
     rd    — register index to write (0–31)
     value — the 32-bit value to write                                               */
void reg_write(cpu_state_t *cpu, uint8_t rd, uint32_t value)
{
    if (rd != 0) {            /* Only write if destination is NOT x0                  */
        cpu->x[rd] = value;   /* Store value into the register array at index rd      */
    }
    /* If rd == 0: do nothing. x0 stays 0. This enforces the RISC-V specification.   */
}
```

---

## Day 3 Exercises — Fully Solved

---

### Exercise 1: decode_r_type Function

**Task:** Define `decoded_instr_t`. Write `decode_r_type(uint32_t raw, decoded_instr_t *out)`. Test with `0x00A28233` (ADD x4, x5, x10).

```c
/* exercise1.c — Decode an R-type RISC-V instruction from a raw 32-bit word */

#include <stdio.h>     /* printf                                               */
#include <stdint.h>    /* uint32_t, int32_t                                    */

/* EXTRACT_BITS(value, hi, lo) — pull out bits [hi:lo] and right-align them.
   Step 1: shift right by 'lo' — moves target bits down to position 0.
   Step 2: mask with (2^(hi-lo+1) - 1) — zeros out anything above the target. */
#define EXTRACT_BITS(val, hi, lo) \
    (((uint32_t)(val) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))

/* The decoded instruction struct — holds every field of one instruction */
typedef struct {
    uint32_t opcode;   /* 7-bit opcode  — identifies instruction family        */
    uint32_t rd;       /* 5-bit rd      — destination register index (0–31)    */
    uint32_t funct3;   /* 3-bit funct3  — distinguishes instructions in group  */
    uint32_t rs1;      /* 5-bit rs1     — source register 1 index (0–31)      */
    uint32_t rs2;      /* 5-bit rs2     — source register 2 index (0–31)      */
    uint32_t funct7;   /* 7-bit funct7  — further distinguishes R-type ops     */
    int32_t  imm;      /* sign-extended immediate (0 for R-type, no immediate) */
} decoded_instr_t;

/* decode_r_type — extracts all fields from a 32-bit R-type instruction.
   Parameters:
     raw — the 32-bit machine word to decode
     out — pointer to where results are written (caller owns the struct)       */
void decode_r_type(uint32_t raw, decoded_instr_t *out)
{
    /* R-type instruction layout:
       [31:25] funct7 | [24:20] rs2 | [19:15] rs1 | [14:12] funct3 | [11:7] rd | [6:0] opcode
       We use EXTRACT_BITS to isolate each field by its bit position.                */

    out->opcode = EXTRACT_BITS(raw, 6,  0);    /* bits  6:0  → 7-bit opcode   */
    out->rd     = EXTRACT_BITS(raw, 11, 7);    /* bits 11:7  → 5-bit rd       */
    out->funct3 = EXTRACT_BITS(raw, 14, 12);   /* bits 14:12 → 3-bit funct3   */
    out->rs1    = EXTRACT_BITS(raw, 19, 15);   /* bits 19:15 → 5-bit rs1      */
    out->rs2    = EXTRACT_BITS(raw, 24, 20);   /* bits 24:20 → 5-bit rs2      */
    out->funct7 = EXTRACT_BITS(raw, 31, 25);   /* bits 31:25 → 7-bit funct7   */
    out->imm    = 0;                            /* R-type has no immediate       */
}

int main(void)
{
    decoded_instr_t instr;   /* Declare the struct — memory for all fields     */

    /* 0x00A28233 = ADD x4, x5, x10 in RISC-V machine code.
       Let's verify manually:
         opcode = 0b0110011 = 0x33 (OP_R_TYPE)
         rd     = 0b00100  = 4    (x4)
         funct3 = 0b000    = 0    (ADD/SUB family)
         rs1    = 0b00101  = 5    (x5)
         rs2    = 0b01010  = 10   (x10)
         funct7 = 0b0000000 = 0   (ADD, not SUB)                               */
    decode_r_type(0x00A28233, &instr);   /* '&instr' passes the ADDRESS of instr */

    /* Print all decoded fields */
    printf("Raw instruction: 0x%08X\n",  0x00A28233);
    printf("Opcode:  0x%02X (should be 0x33 = R-type)\n", instr.opcode);
    printf("rd:      x%u   (should be x4)\n",  instr.rd);
    printf("funct3:  %u    (should be 0 = ADD/SUB)\n",    instr.funct3);
    printf("rs1:     x%u   (should be x5)\n",  instr.rs1);
    printf("rs2:     x%u  (should be x10)\n", instr.rs2);
    printf("funct7:  0x%02X (should be 0x00 = ADD not SUB)\n", instr.funct7);

    /* Determine the mnemonic based on funct3 and funct7 */
    if (instr.funct3 == 0 && instr.funct7 == 0x00) {
        printf("Instruction: ADD x%u, x%u, x%u\n",
               instr.rd, instr.rs1, instr.rs2);
    } else if (instr.funct3 == 0 && instr.funct7 == 0x20) {
        printf("Instruction: SUB x%u, x%u, x%u\n",
               instr.rd, instr.rs1, instr.rs2);
    }

    return 0;   /* 0 = success */
}
```

**Expected output:**
```
Raw instruction: 0x00A28233
Opcode:  0x33 (should be 0x33 = R-type)
rd:      x4   (should be x4)
funct3:  0    (should be 0 = ADD/SUB)
rs1:     x5   (should be x5)
rs2:     x10  (should be x10)
funct7:  0x00 (should be 0x00 = ADD not SUB)
Instruction: ADD x4, x5, x10
```

---

### Exercise 2: opcode_t Enum and opcode_to_string

**Task:** Define `opcode_t` with all RV32I opcodes. Write `opcode_to_string` that returns the mnemonic family name.

```c
/* exercise2.c — Enum for RISC-V opcodes and a function to get their names */

#include <stdio.h>    /* printf                                              */
#include <stdint.h>   /* uint32_t                                            */

/* opcode_t enum — each value is the 7-bit opcode from the RISC-V spec.
   We give each number a descriptive name so code reads like documentation. */
typedef enum {
    OP_R_TYPE  = 0x33,   /* ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU */
    OP_I_TYPE  = 0x13,   /* ADDI, SLTI, ANDI, ORI, XORI, SLLI, SRLI, SRAI   */
    OP_LOAD    = 0x03,   /* LB, LH, LW, LBU, LHU                              */
    OP_STORE   = 0x23,   /* SB, SH, SW                                        */
    OP_BRANCH  = 0x63,   /* BEQ, BNE, BLT, BGE, BLTU, BGEU                   */
    OP_JAL     = 0x6F,   /* JAL (Jump And Link) — J-type format               */
    OP_JALR    = 0x67,   /* JALR (Jump And Link Register) — I-type format     */
    OP_LUI     = 0x37,   /* LUI (Load Upper Immediate) — U-type format        */
    OP_AUIPC   = 0x17,   /* AUIPC (Add Upper Immediate to PC) — U-type        */
    OP_SYSTEM  = 0x73    /* ECALL, EBREAK, CSR instructions                   */
} opcode_t;

/* opcode_to_string — returns a human-readable name for an opcode.
   'const char *' means: returns a pointer to a string that must NOT be modified.
   The strings are string literals (stored in read-only memory by the compiler).  */
const char *opcode_to_string(opcode_t op)
{
    switch (op) {                     /* Match op against each known opcode     */
        case OP_R_TYPE:  return "R-TYPE (ADD/SUB/AND/OR/XOR/SLL/SRL/SRA/SLT/SLTU)";
        case OP_I_TYPE:  return "I-TYPE (ADDI/SLTI/ANDI/ORI/XORI/SLLI/SRLI/SRAI)";
        case OP_LOAD:    return "LOAD   (LB/LH/LW/LBU/LHU)";
        case OP_STORE:   return "STORE  (SB/SH/SW)";
        case OP_BRANCH:  return "BRANCH (BEQ/BNE/BLT/BGE/BLTU/BGEU)";
        case OP_JAL:     return "JAL    (Jump And Link)";
        case OP_JALR:    return "JALR   (Jump And Link Register)";
        case OP_LUI:     return "LUI    (Load Upper Immediate)";
        case OP_AUIPC:   return "AUIPC  (Add Upper Immediate to PC)";
        case OP_SYSTEM:  return "SYSTEM (ECALL/EBREAK/CSR)";
        default:         return "UNKNOWN opcode";  /* Catch-all for bad input  */
    }
}

/* Test: extract opcode from a raw instruction and look it up */
int main(void)
{
    /* Array of test instruction words to decode */
    uint32_t tests[] = {
        0x003100B3,   /* ADD x1, x2, x3   — opcode = 0x33 */
        0x00500113,   /* ADDI x2, x0, 5   — opcode = 0x13 */
        0x0000A103,   /* LW x2, 0(x1)     — opcode = 0x03 */
        0x0020A023,   /* SW x2, 0(x1)     — opcode = 0x23 */
        0x00108063,   /* BEQ x1, x1, 0    — opcode = 0x63 */
        0x004000EF,   /* JAL x1, 4        — opcode = 0x6F */
        0x000010B7    /* LUI x1, 1        — opcode = 0x37 */
    };

    /* Calculate array length: total bytes / bytes per element */
    int n = sizeof(tests) / sizeof(tests[0]);

    printf("Opcode decoding test:\n");
    printf("%-12s  %-6s  %s\n", "Instruction", "Opcode", "Family");
    printf("%-12s  %-6s  %s\n", "-----------", "------", "------");

    int i;   /* Loop counter — declared before the loop for C99 compatibility */
    for (i = 0; i < n; i++) {
        /* Extract bits [6:0] — the opcode field — from the raw instruction   */
        uint32_t opcode_bits = tests[i] & 0x7F;   /* 0x7F = 0b01111111 — mask for 7 bits */

        /* Cast to our enum type so we can pass it to opcode_to_string        */
        opcode_t op = (opcode_t)opcode_bits;

        printf("0x%08X    0x%02X    %s\n",
               tests[i],          /* the raw instruction word                 */
               opcode_bits,        /* the extracted opcode                     */
               opcode_to_string(op)); /* the name string                       */
    }

    return 0;
}
```

---

### Exercise 3: Full CPU State with Register Dump

**Task:** Create `cpu_state_t`. Implement `cpu_init()`, `reg_write()`, `reg_read()`, and `dump_registers()` with ABI names.

```c
/* exercise3.c — Complete RISC-V CPU state with register dump */

#include <stdio.h>     /* printf                                              */
#include <stdint.h>    /* uint32_t, uint8_t, uint64_t                        */
#include <stdlib.h>    /* calloc, free                                        */
#include <string.h>    /* memset                                              */

#define REG_COUNT 32   /* RISC-V has exactly 32 integer registers: x0–x31   */

/* Full CPU state struct — models the observable state of a RISC-V RV32I CPU */
typedef struct {
    uint32_t x[REG_COUNT];  /* General-purpose registers x0–x31              */
    uint32_t pc;             /* Program counter                               */
    uint8_t *memory;         /* Pointer to simulated RAM (allocated on heap)  */
    size_t   mem_size;       /* How many bytes of RAM we have                 */
    uint64_t instr_count;    /* How many instructions have been executed      */
    uint64_t cycle_count;    /* How many cycles have been simulated           */
} cpu_state_t;

/* ABI register names — the standard names programmers use.
   Index i of this array is the ABI name for register xi.
   'static' means this array is created once and shared across all calls.    */
static const char *abi_names[REG_COUNT] = {
    "zero",  /* x0  — always zero, writes ignored                            */
    "ra",    /* x1  — return address (link register)                         */
    "sp",    /* x2  — stack pointer                                          */
    "gp",    /* x3  — global pointer                                         */
    "tp",    /* x4  — thread pointer                                         */
    "t0",    /* x5  — temporary register 0                                   */
    "t1",    /* x6  — temporary register 1                                   */
    "t2",    /* x7  — temporary register 2                                   */
    "s0",    /* x8  — saved register 0 / frame pointer                       */
    "s1",    /* x9  — saved register 1                                       */
    "a0",    /* x10 — function argument 0 / return value 0                   */
    "a1",    /* x11 — function argument 1 / return value 1                   */
    "a2",    /* x12 — function argument 2                                    */
    "a3",    /* x13 — function argument 3                                    */
    "a4",    /* x14 — function argument 4                                    */
    "a5",    /* x15 — function argument 5                                    */
    "a6",    /* x16 — function argument 6                                    */
    "a7",    /* x17 — function argument 7                                    */
    "s2",    /* x18 — saved register 2                                       */
    "s3",    /* x19 — saved register 3                                       */
    "s4",    /* x20 — saved register 4                                       */
    "s5",    /* x21 — saved register 5                                       */
    "s6",    /* x22 — saved register 6                                       */
    "s7",    /* x23 — saved register 7                                       */
    "s8",    /* x24 — saved register 8                                       */
    "s9",    /* x25 — saved register 9                                       */
    "s10",   /* x26 — saved register 10                                      */
    "s11",   /* x27 — saved register 11                                      */
    "t3",    /* x28 — temporary register 3                                   */
    "t4",    /* x29 — temporary register 4                                   */
    "t5",    /* x30 — temporary register 5                                   */
    "t6"     /* x31 — temporary register 6                                   */
};

/* cpu_init — initialise CPU to reset state and allocate RAM */
void cpu_init(cpu_state_t *cpu, size_t mem_size)
{
    int i;
    for (i = 0; i < REG_COUNT; i++) {
        cpu->x[i] = 0;   /* Set every register to 0 (reset value)            */
    }
    cpu->pc          = 0x00000000;   /* RISC-V reset vector                  */
    cpu->memory      = calloc(mem_size, 1); /* Allocate zero-initialised RAM  */
    cpu->mem_size    = mem_size;
    cpu->instr_count = 0;
    cpu->cycle_count = 0;

    /* Always check if calloc succeeded — it can fail if system is out of memory */
    if (cpu->memory == NULL) {
        fprintf(stderr, "FATAL: failed to allocate %zu bytes of memory\n", mem_size);
    }
}

/* reg_write — write to a register, enforcing x0 = 0 */
void reg_write(cpu_state_t *cpu, uint8_t rd, uint32_t value)
{
    if (rd == 0) {
        return;   /* x0 is hardwired to 0 — silently ignore writes           */
    }
    if (rd >= REG_COUNT) {
        fprintf(stderr, "Error: register index %u out of range\n", rd);
        return;   /* Invalid index — don't write, don't crash                */
    }
    cpu->x[rd] = value;   /* Write the value into the register array         */
}

/* reg_read — read from a register (x0 always returns 0) */
uint32_t reg_read(const cpu_state_t *cpu, uint8_t rs)
{
    if (rs >= REG_COUNT) {
        fprintf(stderr, "Error: register index %u out of range\n", rs);
        return 0;   /* Return safe value on bad index                        */
    }
    return cpu->x[rs];   /* Return the register value; x[0] is always 0     */
}

/* dump_registers — print all 32 registers in a formatted table */
void dump_registers(const cpu_state_t *cpu)
{
    int i;
    printf("\n=== CPU Register Dump ===\n");
    printf("%-4s  %-5s  %10s\n", "Reg", "ABI", "Value (hex)");
    printf("%-4s  %-5s  %10s\n", "---", "---", "-----------");

    for (i = 0; i < REG_COUNT; i++) {
        /* Print each register: canonical name, ABI name, and value.
           %-4s   = left-aligned, 4-char wide field for "x0", "x31", etc.
           %-5s   = left-aligned, 5-char wide field for ABI name
           0x%08X = zero-padded 8-digit hex (always shows all 32 bits)       */
        printf("x%-3d  %-5s  0x%08X", i, abi_names[i], cpu->x[i]);

        if (cpu->x[i] != 0) {
            /* Also print as signed decimal for non-zero registers            */
            printf("  (%d)", (int32_t)cpu->x[i]);
        }
        printf("\n");
    }
    printf("PC:             0x%08X\n", cpu->pc);
    printf("instr_count:    %llu\n", (unsigned long long)cpu->instr_count);
}

int main(void)
{
    cpu_state_t cpu;          /* Declare the CPU state struct on the stack    */
    cpu_init(&cpu, 4096);     /* Initialise with 4 KB of simulated memory    */

    /* Write some test values to registers */
    reg_write(&cpu, 1,  0x00400000);   /* x1/ra = return address             */
    reg_write(&cpu, 2,  0x7FFFFF00);   /* x2/sp = top of stack               */
    reg_write(&cpu, 10, 42);           /* x10/a0 = return value 42           */
    reg_write(&cpu, 0,  999);          /* x0/zero — should stay 0 (ignored!) */

    cpu.pc          = 0x00000008;
    cpu.instr_count = 3;

    /* Verify reads */
    printf("reg_read(x10) = %u (should be 42)\n", reg_read(&cpu, 10));
    printf("reg_read(x0)  = %u (should be 0)\n",  reg_read(&cpu, 0));

    dump_registers(&cpu);   /* Print all registers                           */

    /* Free the heap-allocated memory — NEVER forget this! */
    free(cpu.memory);
    cpu.memory = NULL;   /* Good practice: NULL the pointer after free       */

    return 0;
}
```

---

### Exercise 4: Struct Padding — Predict and Verify

**Task:** Predict struct sizes accounting for padding. Verify with `sizeof`. Reorder fields.

```c
/* exercise4.c — Demonstrate struct padding and how to minimise it */

#include <stdio.h>     /* printf                                              */
#include <stdint.h>    /* uint8_t, uint16_t, uint32_t                        */

/* BAD order — fields not sorted by size, lots of wasted padding             */
struct bad_order {
    uint8_t  a;    /* 1 byte  at offset 0                                    */
                   /* 3 bytes PADDING (compiler inserts to align b to 4)     */
    uint32_t b;    /* 4 bytes at offset 4                                    */
    uint8_t  c;    /* 1 byte  at offset 8                                    */
    uint16_t d;    /* 2 bytes at offset 10 (uint16 needs 2-byte alignment)   */
    uint8_t  e;    /* 1 byte  at offset 12                                   */
                   /* 3 bytes PADDING (to make total size a multiple of 4)   */
};                 /* Predicted total: 1+3+4+1+1+2+1+3 = 16 bytes           */

/* GOOD order — sorted largest to smallest, minimal padding                  */
struct good_order {
    uint32_t b;    /* 4 bytes at offset 0                                    */
    uint16_t d;    /* 2 bytes at offset 4                                    */
    uint8_t  a;    /* 1 byte  at offset 6                                    */
    uint8_t  c;    /* 1 byte  at offset 7                                    */
    uint8_t  e;    /* 1 byte  at offset 8                                    */
                   /* 3 bytes PADDING (to make total a multiple of 4)        */
};                 /* Predicted total: 4+2+1+1+1+3 = 12 bytes               */

/* Hardware peripheral register example — ORDER MATTERS!
   If this struct represents memory-mapped registers starting at 0x10000000:
     control  → address 0x10000000
     status   → address 0x10000004
     tx_data  → address 0x10000005 ← WRONG! uint8_t not 4-byte aligned here
   Don't mix sizes without thinking about the hardware layout.               */
struct uart_registers {
    uint32_t control;    /* offset 0: UART control register                  */
    uint32_t status;     /* offset 4: UART status register                   */
    uint8_t  tx_data;    /* offset 8: transmit data register (1 byte)        */
                         /* offset 9-11: 3 bytes padding                     */
    uint32_t rx_data;    /* offset 12: receive data register                 */
};

int main(void)
{
    printf("=== Struct Size and Padding Demo ===\n\n");

    printf("struct bad_order:\n");
    printf("  sizeof = %zu bytes (we predicted 16)\n", sizeof(struct bad_order));
    printf("  Wasted padding: %zu bytes\n",
           sizeof(struct bad_order) - (1 + 4 + 1 + 2 + 1));

    printf("\nstruct good_order:\n");
    printf("  sizeof = %zu bytes (we predicted 12)\n", sizeof(struct good_order));
    printf("  Wasted padding: %zu bytes\n",
           sizeof(struct good_order) - (4 + 2 + 1 + 1 + 1));

    printf("\nMemory saved by reordering: %zu bytes per instance\n",
           sizeof(struct bad_order) - sizeof(struct good_order));

    printf("\nstruct uart_registers:\n");
    printf("  sizeof = %zu bytes\n", sizeof(struct uart_registers));

    /* Use the __builtin_offsetof (GCC extension) or manual check to inspect
       field offsets. Here we use a cast trick that is portable:              */
    struct uart_registers uart;
    printf("  offset of control: %zu\n",
           (size_t)((char*)&uart.control - (char*)&uart));
    printf("  offset of status:  %zu\n",
           (size_t)((char*)&uart.status  - (char*)&uart));
    printf("  offset of tx_data: %zu\n",
           (size_t)((char*)&uart.tx_data - (char*)&uart));
    printf("  offset of rx_data: %zu\n",
           (size_t)((char*)&uart.rx_data - (char*)&uart));

    return 0;
}
```

---

### Exercise 5: Union — Raw and Bit-Field Views Together

**Task:** Union with raw access + R-type/I-type struct views. Decode `0x00500113` (ADDI x2, x0, 5) using I-type view.

```c
/* exercise5.c — Union to view a 32-bit instruction as raw or structured fields */

#include <stdio.h>    /* printf                                               */
#include <stdint.h>   /* uint32_t                                             */

/* Union: all members share the same 4 bytes of memory.
   Writing to 'raw' makes all the bit-field views valid for reading.          */
typedef union {

    uint32_t raw;           /* Full 32-bit instruction word — read/write all at once */

    struct {                /* R-type layout */
        uint32_t opcode : 7;    /* bits [6:0]   */
        uint32_t rd     : 5;    /* bits [11:7]  */
        uint32_t funct3 : 3;    /* bits [14:12] */
        uint32_t rs1    : 5;    /* bits [19:15] */
        uint32_t rs2    : 5;    /* bits [24:20] */
        uint32_t funct7 : 7;    /* bits [31:25] */
    } r;   /* Access as inst.r.rd, inst.r.rs1, etc. */

    struct {                /* I-type layout */
        uint32_t opcode : 7;    /* bits [6:0]   — same position as R-type */
        uint32_t rd     : 5;    /* bits [11:7]  — same position            */
        uint32_t funct3 : 3;    /* bits [14:12] — same position            */
        uint32_t rs1    : 5;    /* bits [19:15] — same position            */
        uint32_t imm    : 12;   /* bits [31:20] — immediate (where rs2+funct7 are in R-type) */
    } i;   /* Access as inst.i.imm, inst.i.rd, etc. */

} instruction_t;   /* Size = 4 bytes (largest member = uint32_t raw = 4 bytes) */

int main(void)
{
    instruction_t inst;   /* Declare the union — allocates 4 bytes           */

    /* Test 1: ADD x4, x5, x10 = 0x00A28233 — decode as R-type */
    inst.raw = 0x00A28233;    /* Write all 32 bits at once                   */

    printf("=== R-type decode: 0x00A28233 (ADD x4, x5, x10) ===\n");
    printf("opcode = 0x%02X (should be 0x33)\n", inst.r.opcode);
    printf("rd     = x%u  (should be x4)\n",  inst.r.rd);
    printf("funct3 = %u   (should be 0)\n",    inst.r.funct3);
    printf("rs1    = x%u  (should be x5)\n",  inst.r.rs1);
    printf("rs2    = x%u (should be x10)\n", inst.r.rs2);
    printf("funct7 = 0x%02X (should be 0x00)\n", inst.r.funct7);

    /* Test 2: ADDI x2, x0, 5 = 0x00500113 — decode as I-type */
    inst.raw = 0x00500113;    /* Overwrite with the ADDI instruction         */

    printf("\n=== I-type decode: 0x00500113 (ADDI x2, x0, 5) ===\n");
    printf("opcode = 0x%02X (should be 0x13 = I-type arith)\n", inst.i.opcode);
    printf("rd     = x%u  (should be x2)\n",  inst.i.rd);
    printf("funct3 = %u   (should be 0 = ADDI)\n",    inst.i.funct3);
    printf("rs1    = x%u  (should be x0)\n",  inst.i.rs1);
    printf("imm    = %u   (should be 5)\n",    inst.i.imm);
    /* Note: 'imm' here is unsigned (12 bits). For signed immediates you need
       sign extension — that's what SIGN_EXTEND does in the Grand Assignment. */

    printf("\nSize of instruction_t union: %zu bytes (always 4)\n",
           sizeof(instruction_t));

    return 0;
}
```

---

### Exercise 6 (Bonus): UART Peripheral Model

**Task:** Model a UART as a struct. Write `uart_putchar()` and `uart_getchar()`.

```c
/* exercise6.c — UART peripheral modelled as a struct */

#include <stdio.h>    /* printf, putchar, getchar                             */
#include <stdint.h>   /* uint32_t, uint8_t                                   */

/* UART Status Register bit masks — each bit has a specific meaning.
   In real hardware, you check these bits by ANDing with the status register. */
#define UART_TX_READY   (1u << 0)   /* bit 0: 1=transmit buffer empty (ready)  */
#define UART_RX_VALID   (1u << 1)   /* bit 1: 1=receive buffer has a character */
#define UART_TX_ERROR   (1u << 2)   /* bit 2: 1=transmit error occurred        */
#define UART_RX_ERROR   (1u << 3)   /* bit 3: 1=receive error occurred         */

/* UART control register bits */
#define UART_ENABLE     (1u << 0)   /* bit 0: 1=UART enabled, 0=disabled       */
#define UART_TX_INT_EN  (1u << 1)   /* bit 1: 1=enable TX done interrupt       */
#define UART_RX_INT_EN  (1u << 2)   /* bit 2: 1=enable RX ready interrupt      */

/* Simulated UART peripheral register map.
   In a real system this struct would be mapped to a specific hardware address:
   volatile uart_t *uart = (uart_t *)0x10000000;
   'volatile' tells the compiler not to cache reads — hardware can change them! */
typedef struct {
    uint32_t control;     /* offset 0: control register — enable, interrupts  */
    uint32_t status;      /* offset 4: status register — read-only in hardware */
    uint8_t  tx_data;     /* offset 8: write here to send a character          */
                          /* offset 9-11: padding (hardware reserved)          */
    uint8_t  rx_data;     /* offset 12: read here to receive a character       */
                          /* offset 13-15: padding                             */
    uint32_t baud_rate;   /* offset 16: baud rate divisor (e.g., 9600)        */
} uart_t;

/* Our simulated UART — in real life this would be at a hardware address       */
static uart_t simulated_uart;

/* uart_init — initialise the UART peripheral to a known state                */
void uart_init(uart_t *uart, uint32_t baud)
{
    uart->control   = UART_ENABLE;   /* Enable UART                           */
    uart->status    = UART_TX_READY; /* TX buffer starts empty (ready to send)*/
    uart->tx_data   = 0;             /* No data queued                        */
    uart->rx_data   = 0;             /* No data received yet                  */
    uart->baud_rate = baud;          /* Set baud rate                         */
}

/* uart_putchar — send one character through the UART.
   In real hardware you would:
   1. Wait until TX_READY bit is set.
   2. Write the character to tx_data.
   3. Hardware automatically clears TX_READY and starts sending.
   Here we simulate it by writing to stdout.                                   */
void uart_putchar(uart_t *uart, char c)
{
    /* Check if TX buffer is ready (bit 0 of status is 1)                     */
    if (!(uart->status & UART_TX_READY)) {
        fprintf(stderr, "UART: TX not ready, dropping character '%c'\n", c);
        return;   /* In real hardware you'd loop and wait                     */
    }

    uart->tx_data  = (uint8_t)c;     /* Put character in transmit buffer      */
    uart->status  &= ~UART_TX_READY; /* Clear TX_READY — hardware is sending  */

    /* Simulate the hardware sending the byte: print to stdout                 */
    putchar(c);

    uart->status |= UART_TX_READY;   /* TX done — buffer empty again          */
}

/* uart_getchar — receive one character from the UART.
   Returns 0 if no data is available (RX_VALID not set).                       */
char uart_getchar(uart_t *uart)
{
    if (!(uart->status & UART_RX_VALID)) {
        return 0;   /* No character available                                  */
    }

    char c = (char)uart->rx_data;     /* Read the received character          */
    uart->status &= ~UART_RX_VALID;   /* Clear flag — character consumed       */
    return c;
}

/* uart_puts — send a null-terminated string character by character            */
void uart_puts(uart_t *uart, const char *str)
{
    while (*str != '\0') {         /* '\0' is the null terminator — end of string */
        uart_putchar(uart, *str);  /* Send current character                       */
        str++;                     /* Advance pointer to next character             */
    }
}

int main(void)
{
    uart_init(&simulated_uart, 9600);   /* Init at 9600 baud                   */

    printf("UART control register:  0x%08X\n", simulated_uart.control);
    printf("UART status register:   0x%08X\n", simulated_uart.status);
    printf("UART baud rate:         %u\n\n",   simulated_uart.baud_rate);

    printf("Sending via UART: ");
    uart_puts(&simulated_uart, "Hello from RISC-V!\n");

    /* Simulate receiving a character by putting one in the RX buffer          */
    simulated_uart.rx_data  = 'A';
    simulated_uart.status  |= UART_RX_VALID;   /* Signal: new character available */

    char received = uart_getchar(&simulated_uart);
    printf("Received via UART: '%c'\n", received);

    printf("\nSizeof uart_t: %zu bytes\n", sizeof(uart_t));

    return 0;
}
```

---

## Summary — What You Learned on Day 3

| Concept        | Key Point                                                              |
|----------------|------------------------------------------------------------------------|
| `struct`        | Group multiple variables into one named type; access with `.` or `->` |
| Struct pointer  | Pass `&myStruct` to functions; use `->` inside the function            |
| Padding         | Compiler inserts invisible bytes for alignment; sort fields large→small |
| `enum`          | Name your integer constants; use in `switch` for readable dispatch     |
| `union`         | All members share the same memory; size = largest member               |
| Bit-fields      | `uint32_t field : N` declares an N-bit field inside a struct/union     |
| Hardware link   | Structs map to hardware register layouts; wrong offsets = wrong register |

---

*Day 3 Complete — MEDS Module 2 | UET Lahore*
