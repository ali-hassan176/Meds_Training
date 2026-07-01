# RISC-V RV32I Instruction Decoder

A command-line tool that reads a hex file containing RISC-V machine code, decodes each instruction, and prints human-readable assembly.

---

## Project Structure

```
riscv-decoder/
├── README.md
├── Makefile
├── .gitignore
├── include/
│   ├── common.h      # Shared macros, types, constants
│   ├── decoder.h     # Decoder function prototypes & types
│   └── memory.h      # Memory subsystem prototypes
├── src/
│   ├── main.c        # Entry point, CLI parsing
│   ├── decoder.c     # Instruction decode logic
│   └── memory.c      # Hex file loading & memory ops
├── test/
│   ├── test_decoder.c
│   └── programs/
│       ├── r_type.hex
│       ├── i_type.hex
│       ├── branch.hex
│       └── mixed.hex
└── docs/
    └── DESIGN.md
```

---

## Build Instructions

```bash
# Build the main executable
make

# Build and run all tests
make test

# Build with debug symbols (for GDB)
make debug

# Run under Valgrind (memory leak check)
make valgrind

# Remove all build artifacts
make clean
```

---

## Usage

```bash
./bin/riscv-decoder <hexfile>
```

The hex file must contain one 32-bit instruction per line as 8 hex digits (no `0x` prefix). Lines starting with `#` are treated as comments and ignored.

---

## Sample Output

```
$ ./bin/riscv-decoder test/programs/mixed.hex
RISC-V RV32I Instruction Decoder
================================
Loaded 8 instructions from test/programs/mixed.hex

Addr       Hex        Assembly
---------- ---------- -------------------------
0x00000000 00500113   addi x2, x0, 5
0x00000004 00A00193   addi x3, x0, 10
0x00000008 003100B3   add x1, x2, x3
0x0000000C 40310133   sub x2, x2, x3
0x00000010 0020A023   sw x2, 0(x1)
0x00000014 0000A103   lw x2, 0(x1)
0x00000018 FE209CE3   bne x1, x2, -8
0x0000001C 004000EF   jal x1, 4

Decoded 8 instructions (8 valid, 0 unknown)
```

---

## Supported Instructions

| Format | Instructions |
|--------|-------------|
| R-type | ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU |
| I-type (arith) | ADDI, ANDI, ORI, XORI, SLTI, SLTIU, SLLI, SRLI, SRAI |
| I-type (load) | LB, LH, LW, LBU, LHU |
| S-type | SB, SH, SW |
| B-type | BEQ, BNE, BLT, BGE, BLTU, BGEU |
| U-type | LUI, AUIPC |
| J-type | JAL |
| I-type (jump) | JALR |

---

## Author

Ali Hassan — Roll No. 2024-EE-176 — MEDS Module 2, UET Lahore
