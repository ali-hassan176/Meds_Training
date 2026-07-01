/* test/test_decoder.c — Unit tests for the RISC-V decoder
   Each test checks one instruction encoding and verifies
   the decoded mnemonic matches the expected output.
   Run with: make test                                             */

#include "../include/common.h"    /* SUCCESS, FAILURE, etc.        */
#include "../include/decoder.h"   /* decode_instruction            */
#include <string.h>               /* strcmp for string comparison  */

/* ─────────────────────────────────────────────────────────────
   Test framework — simple macros for pass/fail tracking.
   ───────────────────────────────────────────────────────────── */

/* Global counters for test results */
static int tests_passed = 0;   /* Incremented on each PASS */
static int tests_failed = 0;   /* Incremented on each FAIL */

/* CHECK macro: evaluates condition, prints PASS or FAIL */
#define CHECK(condition, test_name) \
    do { \
        if (condition) { \
            printf("  PASS: %s\n", test_name); \
            tests_passed++; \
        } else { \
            printf("  FAIL: %s\n", test_name); \
            tests_failed++; \
        } \
    } while (0)

/* ─────────────────────────────────────────────────────────────
   Helper: decode a raw word and return the mnemonic string.
   This is just a convenience wrapper for tests.
   ───────────────────────────────────────────────────────────── */
static const char *decode_mnemonic(uint32_t raw)
{
    /* Static struct reused across calls — safe for sequential tests */
    static DecodedInstr instr;
    decode_instruction(raw, 0x00000000, &instr);
    return instr.mnemonic;
}

/* ─────────────────────────────────────────────────────────────
   TEST GROUPS — one function per instruction format
   ───────────────────────────────────────────────────────────── */

/* test_r_type: verifies R-type instructions (ADD, SUB, AND, OR, …) */
static void test_r_type(void)
{
    printf("\n[R-type Tests]\n");

    /* ADD x1, x2, x3 = 0x003100B3
       opcode=0x33, funct3=0x0, funct7=0x00, rd=1, rs1=2, rs2=3  */
    CHECK(strcmp(decode_mnemonic(0x003100B3), "add x1, x2, x3") == 0,
          "ADD x1, x2, x3");

    /* SUB x2, x2, x3 = 0x40310133
       opcode=0x33, funct3=0x0, funct7=0x20, rd=2, rs1=2, rs2=3  */
    CHECK(strcmp(decode_mnemonic(0x40310133), "sub x2, x2, x3") == 0,
          "SUB x2, x2, x3");

    /* AND x4, x5, x6 = 0x0062F233
       opcode=0x33, funct3=0x7, funct7=0x00                       */
    CHECK(strcmp(decode_mnemonic(0x0062F233), "and x4, x5, x6") == 0,
          "AND x4, x5, x6");

    /* OR x7, x8, x9 = 0x009463B3
       opcode=0x33, funct3=0x6, funct7=0x00                       */
    CHECK(strcmp(decode_mnemonic(0x009463B3), "or x7, x8, x9") == 0,
          "OR x7, x8, x9");

    /* XOR x10, x11, x12 = 0x00C5C533
       opcode=0x33, funct3=0x4, funct7=0x00                       */
    CHECK(strcmp(decode_mnemonic(0x00C5C533), "xor x10, x11, x12") == 0,
          "XOR x10, x11, x12");

    /* SLL x1, x2, x3 = 0x00311033
       funct3=0x1, funct7=0x00                                     */
    CHECK(strcmp(decode_mnemonic(0x00311033), "sll x0, x2, x3") == 0,
          "SLL encoding check");

    /* SRL x1, x2, x3 = 0x003150B3 — funct3=0x5, funct7=0x00      */
    CHECK(strcmp(decode_mnemonic(0x003150B3), "srl x1, x2, x3") == 0,
          "SRL x1, x2, x3");

    /* SRA x1, x2, x3 = 0x403150B3 — funct3=0x5, funct7=0x20      */
    CHECK(strcmp(decode_mnemonic(0x403150B3), "sra x1, x2, x3") == 0,
          "SRA x1, x2, x3");
}

/* test_i_type: verifies I-type arithmetic instructions */
static void test_i_type(void)
{
    printf("\n[I-type Arithmetic Tests]\n");

    /* ADDI x2, x0, 5 = 0x00500113
       imm=5, rs1=x0, rd=x2                                        */
    CHECK(strcmp(decode_mnemonic(0x00500113), "addi x2, x0, 5") == 0,
          "ADDI x2, x0, 5");

    /* ADDI x3, x0, 10 = 0x00A00193
       imm=10, rs1=x0, rd=x3                                       */
    CHECK(strcmp(decode_mnemonic(0x00A00193), "addi x3, x0, 10") == 0,
          "ADDI x3, x0, 10");

    /* ADDI x1, x1, -1 = 0xFFF08093 — tests sign extension! */
    CHECK(strcmp(decode_mnemonic(0xFFF08093), "addi x1, x1, -1") == 0,
          "ADDI x1, x1, -1 (sign extension)");

    /* ANDI x1, x2, 15 = 0x00F17093 */
    CHECK(strcmp(decode_mnemonic(0x00F17093), "andi x1, x2, 15") == 0,
          "ANDI x1, x2, 15");

    /* ORI x1, x2, 7 = 0x00716093 */
    CHECK(strcmp(decode_mnemonic(0x00716093), "ori x1, x2, 7") == 0,
          "ORI x1, x2, 7");

    /* SLLI x1, x2, 3 = 0x00311093 — shift amount = 3 */
    CHECK(strcmp(decode_mnemonic(0x00311093), "slli x1, x2, 3") == 0,
          "SLLI x1, x2, 3");

    /* SRLI x1, x2, 2 = 0x00215093 — logical right shift */
    CHECK(strcmp(decode_mnemonic(0x00215093), "srli x1, x2, 2") == 0,
          "SRLI x1, x2, 2");

    /* SRAI x1, x2, 1 = 0x40115093 — arithmetic right shift */
    CHECK(strcmp(decode_mnemonic(0x40115093), "srai x1, x2, 1") == 0,
          "SRAI x1, x2, 1");
}

/* test_load: verifies load instructions */
static void test_load(void)
{
    printf("\n[Load Instruction Tests]\n");

    /* LW x2, 0(x1) = 0x0000A103
       funct3=0x2, imm=0, rs1=x1, rd=x2                           */
    CHECK(strcmp(decode_mnemonic(0x0000A103), "lw x2, 0(x1)") == 0,
          "LW x2, 0(x1)");

    /* LB x3, 4(x1) = 0x00408183
       funct3=0x0, imm=4                                           */
    CHECK(strcmp(decode_mnemonic(0x00408183), "lb x3, 4(x1)") == 0,
          "LB x3, 4(x1)");

    /* LBU x4, 0(x2) = 0x00014203
       funct3=0x4 (unsigned)                                       */
    CHECK(strcmp(decode_mnemonic(0x00014203), "lbu x4, 0(x2)") == 0,
          "LBU x4, 0(x2)");
}

/* test_store: verifies store instructions */
static void test_store(void)
{
    printf("\n[Store Instruction Tests]\n");

    /* SW x2, 0(x1) = 0x0020A023
       funct3=0x2, imm=0, rs1=x1, rs2=x2                          */
    CHECK(strcmp(decode_mnemonic(0x0020A023), "sw x2, 0(x1)") == 0,
          "SW x2, 0(x1)");

    /* SB x3, 4(x1) = 0x0030822 3 — store byte at offset 4 */
    CHECK(strcmp(decode_mnemonic(0x00308223), "sb x3, 4(x1)") == 0,
          "SB x3, 4(x1)");
}

/* test_branch: verifies branch instructions and sign-extended offsets */
static void test_branch(void)
{
    printf("\n[Branch Instruction Tests]\n");

    /* BEQ x1, x1, 0 = 0x00108063
       funct3=0x0, imm=0                                           */
    CHECK(strcmp(decode_mnemonic(0x00108063), "beq x1, x1, 0") == 0,
          "BEQ x1, x1, 0");

    /* BNE x1, x2, -8 = 0xFE209CE3 — negative offset, tests sign extension */
    CHECK(strcmp(decode_mnemonic(0xFE209CE3), "bne x1, x2, -8") == 0,
          "BNE x1, x2, -8 (negative branch)");

    /* BLT x1, x2, 4 */
    CHECK(strcmp(decode_mnemonic(0x0020C263), "blt x1, x2, 4") == 0,
          "BLT x1, x2, 4");
}

/* test_u_j_type: verifies LUI, AUIPC, JAL */
static void test_u_j_type(void)
{
    printf("\n[U-type and J-type Tests]\n");

    /* LUI x1, 1 = 0x000010B7
       imm[31:12] = 1, so the upper 20 bits = 1, displayed as 1   */
    CHECK(strcmp(decode_mnemonic(0x000010B7), "lui x1, 1") == 0,
          "LUI x1, 1");

    /* AUIPC x1, 0 = 0x00000017 */
    CHECK(strcmp(decode_mnemonic(0x00000017), "auipc x0, 0") == 0,
          "AUIPC x0, 0");

    /* JAL x1, 4 = 0x004000EF
       rd=x1, imm=4 (jump forward 4 bytes)                        */
    CHECK(strcmp(decode_mnemonic(0x004000EF), "jal x1, 4") == 0,
          "JAL x1, 4");

    /* JALR x0, x1, 0 = 0x00008067 — often used as 'ret' pseudo */
    CHECK(strcmp(decode_mnemonic(0x00008067), "jalr x0, x1, 0") == 0,
          "JALR x0, x1, 0 (ret)");
}

/* test_unknown: verifies that invalid opcodes produce UNKNOWN */
static void test_unknown(void)
{
    printf("\n[UNKNOWN Instruction Tests]\n");

    /* 0xDEADBEEF — opcode bits [6:0] = 0x6F would be JAL,
       but let's try a truly unknown opcode like 0x7F (all ones) */
    /* 0xFFFFFFFF — opcode = 0x7F, not a valid RV32I opcode      */
    DecodedInstr instr;
    int result = decode_instruction(0xFFFFFFFF, 0, &instr);

    CHECK(result == FAILURE, "0xFFFFFFFF returns FAILURE");
    CHECK(strcmp(instr.mnemonic, "UNKNOWN") == 0, "0xFFFFFFFF mnemonic is UNKNOWN");
    CHECK(instr.valid == 0, "0xFFFFFFFF valid flag is 0");
}

/* ═══════════════════════════════════════════════════════════════
   main — run all tests and report results
   ═══════════════════════════════════════════════════════════════ */
int main(void)
{
    printf("=== RISC-V Decoder Unit Tests ===\n");

    /* Run all test groups */
    test_r_type();
    test_i_type();
    test_load();
    test_store();
    test_branch();
    test_u_j_type();
    test_unknown();

    /* Print summary */
    printf("\n=================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("=================================\n");

    /* Return 0 only if all tests pass */
    return (tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
