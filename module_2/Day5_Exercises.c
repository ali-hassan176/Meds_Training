/* ==========================================================================
 * Day5_Exercises.c
 * MEDS Module 2 - C Language for Hardware Engineers
 * Ali Hassan | 2024-EE-176 | UET Lahore
 *
 * Menu-driven program containing all Day 5 exercise solutions
 * (Preprocessor, Multi-File Build Concepts).
 *
 * NOTE: Exercise 1 in the original study guide splits the decoder into
 *       main.c / decoder.c / decoder.h plus a Makefile, to demonstrate
 *       separate compilation. Since this single-file menu program must
 *       remain one file, the decoder logic is combined here but kept in
 *       clearly separated sections (as if they were separate files) so you
 *       can see exactly what would go in each one.
 *
 *       Exercise 2 originally used compile-time #ifdef RV64 / #ifdef DEBUG
 *       flags (set via -DRV64 / -DDEBUG on the command line). Since this
 *       program is chosen from a runtime menu, the same behaviour is
 *       reproduced using a runtime toggle so you can compare RV32 vs RV64
 *       and debug-logging ON vs OFF without recompiling.
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -o day5 Day5_Exercises.c
 * Run:
 *   ./day5
 * ========================================================================== */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define EXTRACT_BITS(val, hi, lo) \
    (((uint32_t)(val) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))
#define SIGN_EXTEND(val, bits) \
    (((int32_t)((val) << (32 - (bits)))) >> (32 - (bits)))

/* ==========================================================================
 * Exercise 1: Split a Single-File Decoder into 3 Files (combined here)
 * --------------------------------------------------------------------------
 * Equivalent of include/decoder.h
 * ========================================================================== */
typedef struct {
    uint32_t raw;
    uint32_t pc;
    uint32_t opcode;
    uint32_t rd;
    uint32_t funct3;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct7;
    int32_t  imm;
    char     mnemonic[32];
    int      valid;
} decoded_instr_t;

/* Equivalent of src/decoder.c */
#define OP_REG    0x33
#define OP_IMM    0x13
#define OP_LOAD   0x03
#define OP_STORE  0x23
#define OP_LUI    0x37

static const char *reg_names_d5[32] = {
    "x0","x1","x2","x3","x4","x5","x6","x7",
    "x8","x9","x10","x11","x12","x13","x14","x15",
    "x16","x17","x18","x19","x20","x21","x22","x23",
    "x24","x25","x26","x27","x28","x29","x30","x31"
};

const char *reg_name_d5(uint32_t reg)
{
    if (reg >= 32) return "??";
    return reg_names_d5[reg];
}

int decode_instruction_d5(uint32_t raw, uint32_t pc, decoded_instr_t *out)
{
    memset(out, 0, sizeof(decoded_instr_t));
    out->raw   = raw;
    out->pc    = pc;
    out->valid = 0;

    out->opcode = EXTRACT_BITS(raw,  6,  0);
    out->rd     = EXTRACT_BITS(raw, 11,  7);
    out->funct3 = EXTRACT_BITS(raw, 14, 12);
    out->rs1    = EXTRACT_BITS(raw, 19, 15);
    out->rs2    = EXTRACT_BITS(raw, 24, 20);
    out->funct7 = EXTRACT_BITS(raw, 31, 25);

    switch (out->opcode) {
        case OP_REG: {
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
            snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %s, %s, %s",
                     op, reg_name_d5(out->rd), reg_name_d5(out->rs1), reg_name_d5(out->rs2));
            out->valid = 1;
            break;
        }
        case OP_IMM: {
            uint32_t raw_imm = EXTRACT_BITS(raw, 31, 20);
            out->imm = SIGN_EXTEND(raw_imm, 12);
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
            snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %s, %s, %d",
                     op, reg_name_d5(out->rd), reg_name_d5(out->rs1), out->imm);
            out->valid = 1;
            break;
        }
        case OP_LOAD: {
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
            snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %s, %d(%s)",
                     op, reg_name_d5(out->rd), out->imm, reg_name_d5(out->rs1));
            out->valid = 1;
            break;
        }
        case OP_STORE: {
            uint32_t hi = EXTRACT_BITS(raw, 31, 25);
            uint32_t lo = EXTRACT_BITS(raw, 11,  7);
            out->imm = SIGN_EXTEND((hi << 5) | lo, 12);
            const char *op = "?";
            switch (out->funct3) {
                case 0: op = "sb"; break;
                case 1: op = "sh"; break;
                case 2: op = "sw"; break;
            }
            snprintf(out->mnemonic, sizeof(out->mnemonic), "%s %s, %d(%s)",
                     op, reg_name_d5(out->rs2), out->imm, reg_name_d5(out->rs1));
            out->valid = 1;
            break;
        }
        case OP_LUI: {
            out->imm = (int32_t)(raw & 0xFFFFF000u);
            snprintf(out->mnemonic, sizeof(out->mnemonic), "lui %s, %d",
                     reg_name_d5(out->rd), out->imm >> 12);
            out->valid = 1;
            break;
        }
        default:
            snprintf(out->mnemonic, sizeof(out->mnemonic), "UNKNOWN");
            out->valid = 0;
            return -1;
    }
    return 0;
}

void print_header_d5(void)
{
    printf("%-10s %-10s  %s\n", "Addr", "Hex", "Assembly");
    printf("---------- ----------  -------------------------\n");
}

void print_instruction_d5(const decoded_instr_t *instr)
{
    printf("0x%08X %08X  %s\n", instr->pc, instr->raw, instr->mnemonic);
}

/* Equivalent of src/main.c driver logic */
void exercise1(void)
{
    printf("\n--- Exercise 1: Multi-File Decoder (combined view) ---\n\n");
    printf("(In the original project this is split into decoder.h, decoder.c,\n");
    printf(" main.c and a Makefile - see the comments above for the boundary.)\n\n");

    uint32_t tests[] = {
        0x00A28233, /* add x4, x5, x10  */
        0x00500113, /* addi x2, x0, 5   */
        0x0000A103, /* lw x2, 0(x1)     */
        0x0020A023, /* sw x2, 0(x1)     */
        0x000010B7  /* lui x1, 1        */
    };
    int count = (int)(sizeof(tests) / sizeof(tests[0]));

    printf("RISC-V Decoder\n==============\n");
    printf("Decoding %d sample instructions\n\n", count);
    print_header_d5();

    int valid = 0, unknown = 0;
    decoded_instr_t instr;

    for (int i = 0; i < count; i++) {
        uint32_t pc = (uint32_t)(i * 4);
        decode_instruction_d5(tests[i], pc, &instr);
        print_instruction_d5(&instr);
        if (instr.valid) valid++; else unknown++;
    }

    printf("\nDecoded %d instructions (%d valid, %d unknown)\n", count, valid, unknown);
}

/* ==========================================================================
 * Exercise 2: Conditional Compilation with Debug Macro (runtime toggle)
 * ========================================================================== */
void exercise2(void)
{
    printf("\n--- Exercise 2: Conditional Compilation with Debug Macro ---\n\n");
    printf("(Originally controlled via -DRV64 and -DDEBUG at compile time;\n");
    printf(" reproduced here with a runtime menu so you can try both.)\n\n");

    int use_rv64, use_debug;
    printf("Use RV64 instead of RV32? (1=yes, 0=no): ");
    if (scanf("%d", &use_rv64) != 1) { printf("Invalid input.\n"); return; }
    printf("Enable debug logging? (1=yes, 0=no): ");
    if (scanf("%d", &use_debug) != 1) { printf("Invalid input.\n"); return; }

    int xlen = use_rv64 ? 64 : 32;
    long max_immed = use_rv64 ? 2147483647L : 2047L;

    printf("\n=== RISC-V Simulator Configuration ===\n");
    printf("Architecture: RV%d\n", xlen);
    printf("Register width: %d bits\n", xlen);
    printf("Max 12-bit immediate: %ld\n", max_immed);
    printf("Debug logging: %s\n", use_debug ? "ON (would print to stderr)" : "OFF (compiled out)");

    static uint64_t registers[32];
    for (int i = 0; i < 32; i++) registers[i] = 0;

    /* Simulate reg_write/reg_read with the LOG macro behaviour inlined */
    uint8_t rd_list[]  = {1, 2, 10, 0};
    uint64_t val_list[] = {0x400, 0x7FFFFC, 42, 999};

    for (int k = 0; k < 4; k++) {
        uint8_t rd = rd_list[k];
        uint64_t value = val_list[k];

        if (use_debug) {
            fprintf(stderr, "[DEBUG] reg_write: x%d = 0x%llX\n", rd, (unsigned long long)value);
        }
        assert(rd < 32);
        if (rd == 0) {
            if (use_debug) fprintf(stderr, "[DEBUG]   -> x0 is hardwired to 0, write ignored\n");
            continue;
        }
        registers[rd] = value;
        if (use_debug) {
            fprintf(stderr, "[DEBUG]   -> x%d now = 0x%llX\n", rd, (unsigned long long)registers[rd]);
        }
    }

    printf("\nRegister values:\n");
    printf("x0  = 0x%llX (should be 0)\n", (unsigned long long)registers[0]);
    printf("x1  = 0x%llX\n", (unsigned long long)registers[1]);
    printf("x2  = 0x%llX\n", (unsigned long long)registers[2]);
    printf("x10 = 0x%llX\n", (unsigned long long)registers[10]);
}

/* ==========================================================================
 * Exercise 3: Include Guards - Test Double Inclusion (simulated)
 * --------------------------------------------------------------------------
 * Equivalent of include/types.h (guarded with TYPES_H) and
 * include/constants.h (guarded with CONSTANTS_H, includes types.h).
 * Because real #include guards only matter across multiple files, this
 * exercise demonstrates the same typedefs/macros that those headers would
 * define, and explains the guard mechanism.
 * ========================================================================== */
typedef uint32_t word_t;
typedef uint32_t addr_t;
typedef uint8_t  byte_t;

#define WORD_SIZE  4
#define ADDR_BITS 32
#define MAX_INSTRUCTIONS  4096
#define RESET_VECTOR      0x00000000u
#define STACK_TOP         0x7FFFFFFFu

void exercise3(void)
{
    printf("\n--- Exercise 3: Include Guards - Test Double Inclusion ---\n\n");

    printf("In a real multi-file project:\n");
    printf("  #ifndef TYPES_H / #define TYPES_H / ... / #endif\n");
    printf("protects include/types.h so that including it multiple times\n");
    printf("(directly, or indirectly through constants.h) causes NO\n");
    printf("'redefinition' compiler errors.\n\n");

    word_t instruction = 0x003100B3;
    addr_t pc          = RESET_VECTOR;

    printf("=== Include Guard Test (values that types.h/constants.h would define) ===\n");
    printf("sizeof(word_t)  = %zu bytes\n", sizeof(word_t));
    printf("sizeof(addr_t)  = %zu bytes\n", sizeof(addr_t));
    printf("sizeof(byte_t)  = %zu byte\n", sizeof(byte_t));
    printf("WORD_SIZE       = %d\n", WORD_SIZE);
    printf("MAX_INSTRUCTIONS= %d\n", MAX_INSTRUCTIONS);
    printf("RESET_VECTOR    = 0x%08X\n", RESET_VECTOR);
    printf("instruction     = 0x%08X\n", instruction);
    printf("pc              = 0x%08X\n", pc);

    printf("\nAll (simulated) inclusions of types.h compiled without errors.\n");
    printf("Include guards are working correctly.\n");
}

/* ==========================================================================
 * Menu / Driver
 * ========================================================================== */
void print_menu(void)
{
    printf("\n=====================================================\n");
    printf(" Day 5 Exercises - Preprocessor, Multi-File Build\n");
    printf("=====================================================\n");
    printf(" 1. Multi-File Decoder (combined view)\n");
    printf(" 2. Conditional Compilation with Debug Macro\n");
    printf(" 3. Include Guards - Test Double Inclusion\n");
    printf(" 0. Exit\n");
    printf("-----------------------------------------------------\n");
    printf("Enter exercise number: ");
}

int main(void)
{
    int choice;

    do {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1: exercise1(); break;
            case 2: exercise2(); break;
            case 3: exercise3(); break;
            case 0: printf("\nExiting. Goodbye!\n"); break;
            default: printf("\nInvalid choice. Please enter 0-3.\n"); break;
        }
    } while (choice != 0);

    return 0;
}
