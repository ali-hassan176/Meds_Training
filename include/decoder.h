#ifndef DECODER_H   /* Include guard */
#define DECODER_H

#include "common.h"   /* Pull in shared types and macros */

/* ═══════════════════════════════════════════════════════════════
   ENUMS — we use enums instead of raw numbers so the code reads
   like English rather than magic hex values.
   ═══════════════════════════════════════════════════════════════ */

/* RV32I 7-bit opcode field (bits [6:0] of every instruction).
   These are the standard RISC-V opcode values from the spec.    */
typedef enum {
    OP_LOAD     = 0x03,   /* LB, LH, LW, LBU, LHU               */
    OP_IMM      = 0x13,   /* ADDI, SLTI, ANDI, ORI, XORI, etc.   */
    OP_AUIPC    = 0x17,   /* AUIPC                               */
    OP_STORE    = 0x23,   /* SB, SH, SW                          */
    OP_REG      = 0x33,   /* ADD, SUB, AND, OR, XOR, SLL, etc.   */
    OP_LUI      = 0x37,   /* LUI                                 */
    OP_BRANCH   = 0x63,   /* BEQ, BNE, BLT, BGE, BLTU, BGEU     */
    OP_JALR     = 0x67,   /* JALR                                */
    OP_JAL      = 0x6F    /* JAL                                 */
} Opcode;

/* funct3 field (bits [14:12]) — distinguishes instructions within
   the same opcode group.                                        */
typedef enum {
    /* --- R-type / I-type arithmetic funct3 codes --- */
    F3_ADD_SUB  = 0x0,   /* ADD/SUB (R), ADDI (I)               */
    F3_SLL      = 0x1,   /* SLL, SLLI                           */
    F3_SLT      = 0x2,   /* SLT, SLTI                           */
    F3_SLTU     = 0x3,   /* SLTU, SLTIU                         */
    F3_XOR      = 0x4,   /* XOR, XORI                           */
    F3_SRL_SRA  = 0x5,   /* SRL/SRA, SRLI/SRAI                  */
    F3_OR       = 0x6,   /* OR, ORI                             */
    F3_AND      = 0x7,   /* AND, ANDI                           */

    /* --- Load funct3 codes --- */
    F3_LB       = 0x0,   /* Load Byte (signed)                  */
    F3_LH       = 0x1,   /* Load Halfword (signed)              */
    F3_LW       = 0x2,   /* Load Word                           */
    F3_LBU      = 0x4,   /* Load Byte Unsigned                  */
    F3_LHU      = 0x5,   /* Load Halfword Unsigned              */

    /* --- Store funct3 codes --- */
    F3_SB       = 0x0,   /* Store Byte                          */
    F3_SH       = 0x1,   /* Store Halfword                      */
    F3_SW       = 0x2,   /* Store Word                          */

    /* --- Branch funct3 codes --- */
    F3_BEQ      = 0x0,   /* Branch if Equal                     */
    F3_BNE      = 0x1,   /* Branch if Not Equal                 */
    F3_BLT      = 0x4,   /* Branch if Less Than (signed)        */
    F3_BGE      = 0x5,   /* Branch if Greater or Equal (signed) */
    F3_BLTU     = 0x6,   /* Branch if Less Than Unsigned        */
    F3_BGEU     = 0x7    /* Branch if Greater or Equal Unsigned */
} Funct3;

/* funct7 field (bits [31:25]) — used in R-type to tell apart
   ADD vs SUB, SRL vs SRA, etc.                                  */
typedef enum {
    F7_NORMAL   = 0x00,   /* Standard operation (e.g., ADD, SRL) */
    F7_ALT      = 0x20    /* Alternate operation (e.g., SUB, SRA) */
} Funct7;

/* Instruction format types — each RISC-V instruction falls into
   one of these six encoding formats.                            */
typedef enum {
    FMT_R,        /* Register-Register: ADD, SUB, AND, …         */
    FMT_I,        /* Immediate: ADDI, LW, JALR, …               */
    FMT_S,        /* Store: SB, SH, SW                          */
    FMT_B,        /* Branch: BEQ, BNE, …                        */
    FMT_U,        /* Upper Immediate: LUI, AUIPC                 */
    FMT_J,        /* Jump: JAL                                   */
    FMT_UNKNOWN   /* Unrecognised opcode                         */
} InstrFormat;

/* ═══════════════════════════════════════════════════════════════
   STRUCT — holds all fields extracted from one 32-bit instruction
   ═══════════════════════════════════════════════════════════════ */
typedef struct {
    uint32_t     raw;        /* The original 32-bit machine word             */
    uint32_t     pc;         /* Program counter (address) of this instruction */
    Opcode       opcode;     /* 7-bit opcode field                           */
    InstrFormat  format;     /* Which encoding format this instruction uses   */
    uint8_t      rd;         /* Destination register index (0–31)            */
    uint8_t      rs1;        /* Source register 1 index (0–31)               */
    uint8_t      rs2;        /* Source register 2 index (0–31)               */
    uint8_t      funct3;     /* 3-bit function discriminator                 */
    uint8_t      funct7;     /* 7-bit function discriminator (R-type only)   */
    int32_t      imm;        /* Sign-extended immediate value                */
    char         mnemonic[32]; /* Human-readable instruction name, e.g. "addi" */
    int          valid;      /* 1 = successfully decoded, 0 = UNKNOWN        */
} DecodedInstr;

/* ═══════════════════════════════════════════════════════════════
   FUNCTION PROTOTYPES — defined in decoder.c
   ═══════════════════════════════════════════════════════════════ */

/* decode_instruction: takes a 32-bit word and its PC address,
   fills in a DecodedInstr struct, returns SUCCESS or FAILURE.   */
int decode_instruction(uint32_t raw, uint32_t pc, DecodedInstr *out);

/* print_instruction: prints one decoded instruction in the
   required output format to stdout.                             */
void print_instruction(const DecodedInstr *instr);

/* print_header: prints the table header line.                   */
void print_header(void);

/* reg_name: returns the ABI name string for register index n
   (e.g., reg_name(1) → "x1", reg_name(0) → "x0").             */
const char *reg_name(uint8_t reg);

#endif /* DECODER_H */
