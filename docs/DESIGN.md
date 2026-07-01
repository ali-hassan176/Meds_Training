# DESIGN.md — Decoder Design Decisions

## Overview

The decoder follows a **two-pass approach** for each instruction:

1. **Field extraction** — bits are pulled out using the `EXTRACT_BITS(value, hi, lo)` macro, which isolates any bit range without relying on struct bit-fields (which are compiler-dependent).
2. **Format dispatch** — the 7-bit opcode is used in a `switch` to route each instruction to the correct format-specific decoder function.

---

## Key Design Decisions

### 1. No Struct Bit-Fields

RISC-V fields like `rd`, `rs1`, `funct3` are extracted manually using shifts and masks rather than C bit-fields. This is because bit-field layout in memory is implementation-defined (compiler and endianness can change it). The `EXTRACT_BITS` macro gives fully portable, explicit behaviour.

### 2. Sign Extension Macro

The `SIGN_EXTEND(value, bits)` macro works by left-shifting the raw value so the sign bit lands at bit 31, then doing an arithmetic right shift back. In C, right-shifting a signed integer is arithmetic (fills with the sign bit) — this is guaranteed in practice and relied upon by nearly all embedded C code.

### 3. Immediate Reconstruction

Each format has its own `imm_x()` helper (imm_i, imm_s, imm_b, imm_u, imm_j) that reassembles the scattered immediate bits in the exact order specified by the RISC-V spec. Every comment in these functions cites the bit positions being moved.

### 4. Enums for Opcodes and funct3/funct7

Using `typedef enum` instead of raw hex literals means the code documents itself. `OP_LOAD = 0x03` is readable; `0x03` is not. The compiler also warns if a switch misses an enum value.

### 5. Struct for Decoded Instructions

`DecodedInstr` holds every field of the instruction plus the pre-built mnemonic string. Separating decoding from printing means the struct can be passed to a future simulator without changes to the decoder.

### 6. Zero Memory Leaks

The program uses no dynamic memory allocation (`malloc`/`free`). All storage is either stack-allocated (`DecodedInstr instr` in main) or a fixed-size global array (`Memory.data[]`). Valgrind will report zero leaks.

---

## Instruction Encoding Quick Reference

```
R-type: [funct7(7) | rs2(5) | rs1(5) | funct3(3) | rd(5) | opcode(7)]
I-type: [imm[11:0](12) | rs1(5) | funct3(3) | rd(5) | opcode(7)]
S-type: [imm[11:5](7) | rs2(5) | rs1(5) | funct3(3) | imm[4:0](5) | opcode(7)]
B-type: [imm[12|10:5](7) | rs2(5) | rs1(5) | funct3(3) | imm[4:1|11](5) | opcode(7)]
U-type: [imm[31:12](20) | rd(5) | opcode(7)]
J-type: [imm[20|10:1|11|19:12](20) | rd(5) | opcode(7)]
```
