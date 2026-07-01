#ifndef COMMON_H   /* Include guard: prevents this file from being included more than once */
#define COMMON_H

/* ─────────────────────────────────────────────
   Standard library includes used across files
   ───────────────────────────────────────────── */
#include <stdint.h>   /* Provides uint32_t, int32_t, etc. — exact-width integers */
#include <stdio.h>    /* Provides printf, fprintf, FILE, etc. */
#include <stdlib.h>   /* Provides malloc, free, exit, etc. */
#include <string.h>   /* Provides memset, memcpy, strlen, etc. */

/* ─────────────────────────────────────────────
   Project-wide constants  (no magic numbers!)
   ───────────────────────────────────────────── */
#define MAX_INSTRUCTIONS   4096   /* Maximum number of instructions we can hold in memory    */
#define HEX_LINE_LEN       16     /* Max characters expected on one line of a .hex file      */
#define WORD_SIZE          4      /* RISC-V is 32-bit: each instruction is 4 bytes           */
#define REG_COUNT          32     /* RISC-V RV32I has 32 integer registers (x0 – x31)       */

/* ─────────────────────────────────────────────
   Bit-manipulation helper macros
   ───────────────────────────────────────────── */

/* EXTRACT_BITS(value, hi, lo)
   Pulls out bits [hi:lo] from 'value' and returns them right-aligned (shifted to bit-0).
   Example: EXTRACT_BITS(0b11010100, 6, 4) → 0b101
   How it works:
     1. Shift right by 'lo' to bring the target bits down to position 0.
     2. Build a mask of (hi - lo + 1) ones using (1 << width) - 1.
     3. AND with the mask to zero out any higher bits.                         */
#define EXTRACT_BITS(value, hi, lo) \
    (((uint32_t)(value) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))

/* SIGN_EXTEND(value, bits)
   Sign-extends a value that is 'bits' wide into a full 32-bit signed integer.
   This is needed for RISC-V immediates (12-bit, 13-bit, 21-bit, etc.).
   How it works:
     If bit [bits-1] of 'value' is 1 (negative in two's complement):
       fill all upper bits with 1s by OR-ing with a mask of leading ones.
     If bit [bits-1] is 0 (positive):
       the value is already correct (upper bits are 0).                        */
#define SIGN_EXTEND(value, bits) \
    (((int32_t)((value) << (32 - (bits)))) >> (32 - (bits)))

/* ─────────────────────────────────────────────
   Return codes used throughout the project
   ───────────────────────────────────────────── */
#define SUCCESS   0   /* Operation completed without errors  */
#define FAILURE  -1   /* Operation encountered an error      */

#endif /* COMMON_H */
