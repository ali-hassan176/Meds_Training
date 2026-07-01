/* decoder.c — RISC-V RV32I Instruction Decode Logic
   This file is the heart of the project.  It takes a raw 32-bit
   machine word, extracts every field using bitwise operations, and
   builds a human-readable assembly string.                        */

#include "../include/decoder.h"   /* DecodedInstr, enums, prototypes */
#include <stdio.h>                /* snprintf for building strings    */

/* ───────────────────────────────────────────────────────────────
   reg_name — return the RISC-V register name for index n.
   RISC-V uses x0–x31 as canonical names.
   ─────────────────────────────────────────────────────────────── */
const char *reg_name(uint8_t reg)
{
    /* Static table maps index 0–31 to its register name string.
       'static' means the array lives for the whole program life.  */
    static const char *names[REG_COUNT] = {
        "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
        "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"
    };

    /* Safety check: clamp to valid range */
    if (reg >= REG_COUNT) {
        return "??";   /* Should never happen with a valid instruction */
    }

    return names[reg];   /* Return the pointer to the name string */
}

/* ═══════════════════════════════════════════════════════════════
   FIELD-EXTRACTION HELPERS
   Each function isolates one field from the 32-bit instruction
   word using the EXTRACT_BITS macro from common.h.
   ═══════════════════════════════════════════════════════════════ */

/* get_opcode: bits [6:0] */
static uint32_t get_opcode(uint32_t raw) { return EXTRACT_BITS(raw, 6, 0);  }

/* get_rd: bits [11:7] — destination register */
static uint32_t get_rd    (uint32_t raw) { return EXTRACT_BITS(raw, 11, 7); }

/* get_funct3: bits [14:12] */
static uint32_t get_funct3(uint32_t raw) { return EXTRACT_BITS(raw, 14, 12); }

/* get_rs1: bits [19:15] — source register 1 */
static uint32_t get_rs1   (uint32_t raw) { return EXTRACT_BITS(raw, 19, 15); }

/* get_rs2: bits [24:20] — source register 2 */
static uint32_t get_rs2   (uint32_t raw) { return EXTRACT_BITS(raw, 24, 20); }

/* get_funct7: bits [31:25] */
static uint32_t get_funct7(uint32_t raw) { return EXTRACT_BITS(raw, 31, 25); }

/* ═══════════════════════════════════════════════════════════════
   IMMEDIATE EXTRACTION
   Each instruction format encodes the immediate differently.
   All immediates must be sign-extended to 32 bits.
   ═══════════════════════════════════════════════════════════════ */

/* I-type immediate: bits [31:12] → sign-extended 12-bit value
   Used by: ADDI, LW, JALR, etc.
   Encoding: imm[11:0] = raw[31:20]                              */
static int32_t imm_i(uint32_t raw)
{
    uint32_t imm = EXTRACT_BITS(raw, 31, 20);   /* Pull bits 31–20 (12 bits) */
    return SIGN_EXTEND(imm, 12);                /* Sign-extend from bit 11   */
}

/* S-type immediate: split across two fields, then reassembled
   Encoding: imm[11:5] = raw[31:25],  imm[4:0] = raw[11:7]      */
static int32_t imm_s(uint32_t raw)
{
    uint32_t hi  = EXTRACT_BITS(raw, 31, 25);   /* Upper 7 bits of immediate */
    uint32_t lo  = EXTRACT_BITS(raw, 11,  7);   /* Lower 5 bits of immediate */
    /* Reassemble: shift hi up to bits [11:5], OR in lo at bits [4:0] */
    uint32_t imm = (hi << 5) | lo;
    return SIGN_EXTEND(imm, 12);                /* Result is 12 bits wide    */
}

/* B-type immediate: even more scattered — bit positions encode
   multiples of 2 (branch offsets are always even).
   Encoding: imm[12]   = raw[31]
             imm[10:5] = raw[30:25]
             imm[4:1]  = raw[11:8]
             imm[11]   = raw[7]
             imm[0]    = 0 (always — branches are 2-byte aligned) */
static int32_t imm_b(uint32_t raw)
{
    uint32_t bit12  = EXTRACT_BITS(raw, 31, 31);  /* MSB of immediate     */
    uint32_t bit11  = EXTRACT_BITS(raw,  7,  7);  /* Bit 11 hidden at [7] */
    uint32_t bits10_5 = EXTRACT_BITS(raw, 30, 25); /* Bits 10–5           */
    uint32_t bits4_1  = EXTRACT_BITS(raw, 11,  8); /* Bits 4–1            */

    /* Reassemble the 13-bit immediate (bit 0 is always 0) */
    uint32_t imm = (bit12   << 12) |
                   (bit11   << 11) |
                   (bits10_5 << 5) |
                   (bits4_1  << 1);  /* bit 0 implicit = 0 */

    return SIGN_EXTEND(imm, 13);   /* Sign-extend from bit 12 */
}

/* U-type immediate: upper 20 bits placed at [31:12], lower 12 = 0
   Encoding: imm[31:12] = raw[31:12]                              */
static int32_t imm_u(uint32_t raw)
{
    /* Simply mask off the lower 12 bits; the upper 20 are the immediate.
       We keep them in place (already at bits [31:12]).              */
    return (int32_t)(raw & 0xFFFFF000u);
}

/* J-type immediate: another scrambled layout for JAL.
   Encoding: imm[20]    = raw[31]
             imm[10:1]  = raw[30:21]
             imm[11]    = raw[20]
             imm[19:12] = raw[19:12]
             imm[0]     = 0  (jumps are 2-byte aligned)            */
static int32_t imm_j(uint32_t raw)
{
    uint32_t bit20    = EXTRACT_BITS(raw, 31, 31);  /* Sign bit             */
    uint32_t bits10_1 = EXTRACT_BITS(raw, 30, 21);  /* Bits 10–1 of offset  */
    uint32_t bit11    = EXTRACT_BITS(raw, 20, 20);  /* Bit 11               */
    uint32_t bits19_12= EXTRACT_BITS(raw, 19, 12);  /* Bits 19–12           */

    /* Reassemble 21-bit immediate */
    uint32_t imm = (bit20     << 20) |
                   (bits19_12 << 12) |
                   (bit11     << 11) |
                   (bits10_1  <<  1);   /* bit 0 = 0 (2-byte aligned) */

    return SIGN_EXTEND(imm, 21);   /* Sign-extend from bit 20 */
}

/* ═══════════════════════════════════════════════════════════════
   FORMAT-SPECIFIC DECODERS
   Each function handles one instruction format, fills in the
   mnemonic field of 'out', and returns SUCCESS or FAILURE.
   ═══════════════════════════════════════════════════════════════ */
/* R-type decoder — handles all register-to-register operations */
/* decode_r_type — handles ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU */
static int decode_r_type(DecodedInstr *out)
{
    out->format = FMT_R;          /* Record format for the caller */
    out->imm    = 0;              /* R-type has no immediate field */

    /* Use funct3 and funct7 together to identify the exact instruction */
    switch (out->funct3) {

        case F3_ADD_SUB:   /* funct3 = 0b000 */
            if (out->funct7 == F7_NORMAL) {
                /* funct7 = 0x00 → ADD: rd = rs1 + rs2 */
                snprintf(out->mnemonic, sizeof(out->mnemonic),
                         "add %s, %s, %s",
                         reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            } else if (out->funct7 == F7_ALT) {
                /* funct7 = 0x20 → SUB: rd = rs1 - rs2 */
                snprintf(out->mnemonic, sizeof(out->mnemonic),
                         "sub %s, %s, %s",
                         reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            } else {
                return FAILURE;   /* Unknown funct7 */
            }
            break;

        case F3_SLL:   /* funct3 = 0b001 → SLL: rd = rs1 << rs2[4:0] */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sll %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        case F3_SLT:   /* funct3 = 0b010 → SLT: rd = (rs1 < rs2) ? 1 : 0 (signed) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "slt %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        case F3_SLTU:   /* funct3 = 0b011 → SLTU: unsigned version of SLT */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sltu %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        case F3_XOR:   /* funct3 = 0b100 → XOR: rd = rs1 ^ rs2 */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "xor %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        case F3_SRL_SRA:   /* funct3 = 0b101 */
            if (out->funct7 == F7_NORMAL) {
                /* SRL: logical right shift (fills with zeros) */
                snprintf(out->mnemonic, sizeof(out->mnemonic),
                         "srl %s, %s, %s",
                         reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            } else if (out->funct7 == F7_ALT) {
                /* SRA: arithmetic right shift (fills with sign bit) */
                snprintf(out->mnemonic, sizeof(out->mnemonic),
                         "sra %s, %s, %s",
                         reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            } else {
                return FAILURE;
            }
            break;

        case F3_OR:   /* funct3 = 0b110 → OR: rd = rs1 | rs2 */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "or %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        case F3_AND:   /* funct3 = 0b111 → AND: rd = rs1 & rs2 */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "and %s, %s, %s",
                     reg_name(out->rd), reg_name(out->rs1), reg_name(out->rs2));
            break;

        default:
            return FAILURE;   /* Unknown funct3 */
    }

    return SUCCESS;
}

/* decode_i_arith — handles ADDI, SLTI, SLTIU, ANDI, ORI, XORI, SLLI, SRLI, SRAI */
static int decode_i_arith(DecodedInstr *out)
{
    out->format = FMT_I;    /* I-type format */
    out->imm    = imm_i(out->raw);   /* Extract sign-extended 12-bit immediate */

    switch (out->funct3) {

        case F3_ADD_SUB:   /* ADDI: rd = rs1 + imm */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "addi %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_SLT:   /* SLTI: rd = (rs1 < imm) ? 1 : 0 (signed) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "slti %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_SLTU:   /* SLTIU: unsigned comparison with sign-extended imm */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sltiu %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_XOR:   /* XORI: rd = rs1 ^ imm */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "xori %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_OR:   /* ORI: rd = rs1 | imm */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "ori %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_AND:   /* ANDI: rd = rs1 & imm */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "andi %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            break;

        case F3_SLL:   /* SLLI: rd = rs1 << shamt (shamt = imm[4:0]) */
            {
                /* For shift immediates, only the low 5 bits matter (shamt) */
                uint32_t shamt = EXTRACT_BITS(out->raw, 24, 20);
                snprintf(out->mnemonic, sizeof(out->mnemonic),
                         "slli %s, %s, %u",
                         reg_name(out->rd), reg_name(out->rs1), shamt);
            }
            break;

        case F3_SRL_SRA:   /* SRLI or SRAI — distinguished by funct7 bit 30 */
            {
                uint32_t shamt   = EXTRACT_BITS(out->raw, 24, 20);  /* Shift amount bits [24:20] */
                uint32_t is_arith = EXTRACT_BITS(out->raw, 30, 30); /* Bit 30 = 1 → SRAI        */
                if (is_arith) {
                    /* SRAI: arithmetic shift (preserves sign bit) */
                    snprintf(out->mnemonic, sizeof(out->mnemonic),
                             "srai %s, %s, %u",
                             reg_name(out->rd), reg_name(out->rs1), shamt);
                } else {
                    /* SRLI: logical shift (fills with zeros) */
                    snprintf(out->mnemonic, sizeof(out->mnemonic),
                             "srli %s, %s, %u",
                             reg_name(out->rd), reg_name(out->rs1), shamt);
                }
            }
            break;

        default:
            return FAILURE;
    }

    return SUCCESS;
}

/* decode_load — handles LB, LH, LW, LBU, LHU */
/* Load decoder — LB/LH/LW/LBU/LHU with sign-extended 12-bit offset */
static int decode_load(DecodedInstr *out)
{
    out->format = FMT_I;              /* Loads use I-type encoding */
    out->imm    = imm_i(out->raw);   /* Offset from base register rs1 */

    /* Load assembly syntax: mnemonic rd, imm(rs1) */
    switch (out->funct3) {

        case F3_LB:    /* Load Byte (sign-extended) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lb %s, %d(%s)",
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            break;

        case F3_LH:    /* Load Halfword (sign-extended) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lh %s, %d(%s)",
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            break;

        case F3_LW:    /* Load Word (32-bit) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lw %s, %d(%s)",
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            break;

        case F3_LBU:   /* Load Byte Unsigned (zero-extended) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lbu %s, %d(%s)",
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            break;

        case F3_LHU:   /* Load Halfword Unsigned (zero-extended) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lhu %s, %d(%s)",
                     reg_name(out->rd), out->imm, reg_name(out->rs1));
            break;

        default:
            return FAILURE;
    }

    return SUCCESS;
}

/* decode_store — handles SB, SH, SW */
/* Store decoder — SB/SH/SW with S-type split immediate */
static int decode_store(DecodedInstr *out)
{
    out->format = FMT_S;              /* S-type encoding */
    out->imm    = imm_s(out->raw);   /* 12-bit signed offset */

    /* Store assembly syntax: mnemonic rs2, imm(rs1)
       rs2 = register whose value is stored
       rs1 = base address register                                */
    switch (out->funct3) {

        case F3_SB:   /* Store Byte: mem[rs1+imm] = rs2[7:0] */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sb %s, %d(%s)",
                     reg_name(out->rs2), out->imm, reg_name(out->rs1));
            break;

        case F3_SH:   /* Store Halfword: mem[rs1+imm] = rs2[15:0] */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sh %s, %d(%s)",
                     reg_name(out->rs2), out->imm, reg_name(out->rs1));
            break;

        case F3_SW:   /* Store Word: mem[rs1+imm] = rs2[31:0] */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "sw %s, %d(%s)",
                     reg_name(out->rs2), out->imm, reg_name(out->rs1));
            break;

        default:
            return FAILURE;
    }

    return SUCCESS;
}

/* decode_branch — handles BEQ, BNE, BLT, BGE, BLTU, BGEU */
static int decode_branch(DecodedInstr *out)
{
    out->format = FMT_B;              /* B-type encoding */
    out->imm    = imm_b(out->raw);   /* 13-bit signed PC-relative offset */

    /* Branch syntax: mnemonic rs1, rs2, imm
       The CPU jumps to PC+imm if the condition is true.         */
    switch (out->funct3) {

        case F3_BEQ:    /* Branch if rs1 == rs2 */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "beq %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        case F3_BNE:    /* Branch if rs1 != rs2 */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "bne %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        case F3_BLT:    /* Branch if rs1 < rs2 (signed) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "blt %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        case F3_BGE:    /* Branch if rs1 >= rs2 (signed) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "bge %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        case F3_BLTU:   /* Branch if rs1 < rs2 (unsigned) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "bltu %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        case F3_BGEU:   /* Branch if rs1 >= rs2 (unsigned) */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "bgeu %s, %s, %d",
                     reg_name(out->rs1), reg_name(out->rs2), out->imm);
            break;

        default:
            return FAILURE;
    }

    return SUCCESS;
}

/* ═══════════════════════════════════════════════════════════════
   decode_instruction — MAIN DECODE ENTRY POINT
   Takes a raw 32-bit word and its PC, fills out DecodedInstr.
   Returns SUCCESS if instruction is known, FAILURE otherwise.
   ═══════════════════════════════════════════════════════════════ */
int decode_instruction(uint32_t raw, uint32_t pc, DecodedInstr *out)
{
    /* Zero-initialise the output struct so no garbage fields remain */
    memset(out, 0, sizeof(DecodedInstr));

    /* Record raw word and address for display later */
    out->raw    = raw;
    out->pc     = pc;
    out->valid  = 0;   /* Assume UNKNOWN until we successfully decode */

    /* ── Step 1: Extract all common fields from the 32-bit word ─── */
    out->opcode = (Opcode)get_opcode(raw);   /* bits [6:0]  */
    out->rd     = (uint8_t)get_rd(raw);      /* bits [11:7] */
    out->funct3 = (uint8_t)get_funct3(raw);  /* bits [14:12]*/
    out->rs1    = (uint8_t)get_rs1(raw);     /* bits [19:15]*/
    out->rs2    = (uint8_t)get_rs2(raw);     /* bits [24:20]*/
    out->funct7 = (uint8_t)get_funct7(raw);  /* bits [31:25]*/

    /* ── Step 2: Dispatch to the appropriate format decoder ──────── */
    int result = FAILURE;   /* Default: unknown instruction */

    switch (out->opcode) {

        case OP_REG:    /* R-type: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU */
            result = decode_r_type(out);
            break;

        case OP_IMM:    /* I-type arithmetic: ADDI, SLTI, ANDI, ORI, XORI, SLLI, SRLI, SRAI */
            result = decode_i_arith(out);
            break;

        case OP_LOAD:   /* I-type load: LB, LH, LW, LBU, LHU */
            result = decode_load(out);
            break;

        case OP_STORE:  /* S-type: SB, SH, SW */
            result = decode_store(out);
            break;

        case OP_BRANCH: /* B-type: BEQ, BNE, BLT, BGE, BLTU, BGEU */
            result = decode_branch(out);
            break;

        case OP_LUI:    /* U-type: LUI rd, imm  — Load Upper Immediate */
            out->format = FMT_U;
            out->imm    = imm_u(raw);
            /* imm already in upper 20 bits; display as right-shifted value */
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "lui %s, %d",
                     reg_name(out->rd), (out->imm >> 12));
            result = SUCCESS;
            break;

        case OP_AUIPC:  /* U-type: AUIPC rd, imm  — Add Upper Immediate to PC */
            out->format = FMT_U;
            out->imm    = imm_u(raw);
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "auipc %s, %d",
                     reg_name(out->rd), (out->imm >> 12));
            result = SUCCESS;
            break;

        case OP_JAL:    /* J-type: JAL rd, imm  — Jump And Link */
            out->format = FMT_J;
            out->imm    = imm_j(raw);
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "jal %s, %d",
                     reg_name(out->rd), out->imm);
            result = SUCCESS;
            break;

        case OP_JALR:   /* I-type: JALR rd, rs1, imm  — Jump And Link Register */
            out->format = FMT_I;
            out->imm    = imm_i(raw);
            snprintf(out->mnemonic, sizeof(out->mnemonic),
                     "jalr %s, %s, %d",
                     reg_name(out->rd), reg_name(out->rs1), out->imm);
            result = SUCCESS;
            break;

        default:
            /* Opcode not in RV32I base ISA — mark as UNKNOWN */
            out->format = FMT_UNKNOWN;
            snprintf(out->mnemonic, sizeof(out->mnemonic), "UNKNOWN");
            result = FAILURE;
            break;
    }

    /* ── Step 3: Mark validity based on decode result ────────────── */
    out->valid = (result == SUCCESS) ? 1 : 0;

    /* For UNKNOWN instructions, override mnemonic to required format */
    if (!out->valid) {
        snprintf(out->mnemonic, sizeof(out->mnemonic), "UNKNOWN");
    }

    return result;
}

/* ═══════════════════════════════════════════════════════════════
   print_header — prints the column header for the output table
   ═══════════════════════════════════════════════════════════════ */
void print_header(void)
{
    /* Fixed-width columns: Addr (10), Hex (10), Assembly (25+) */
    printf("%-10s %-10s %s\n", "Addr", "Hex", "Assembly");
    printf("---------- ---------- -------------------------\n");
}

/* ═══════════════════════════════════════════════════════════════
   print_instruction — prints one decoded instruction row
   ═══════════════════════════════════════════════════════════════ */
void print_instruction(const DecodedInstr *instr)
{
    /* Format: 0x00000000 DEADBEEF  mnemonic_here
       %-10s → left-aligned in 10-char field
       %08X  → uppercase hex, zero-padded to 8 digits              */
    printf("0x%08X %08X  %s\n",
           instr->pc,       /* Program counter address */
           instr->raw,      /* Original hex word       */
           instr->mnemonic  /* Decoded assembly text   */
    );
}
