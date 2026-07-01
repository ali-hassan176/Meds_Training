/* memory.c — Hex file loading and memory read operations
   This module owns the Memory struct and all I/O for loading
   machine code from .hex text files.                             */

#include "../include/memory.h"   /* Our own header — Memory struct, prototypes */
#include <ctype.h>               /* isspace() — needed to trim whitespace       */

/* ───────────────────────────────────────────────────────────────
   mem_init
   Zeroes the Memory struct so we start from a clean state.
   Always call this before mem_load_hex.
   ─────────────────────────────────────────────────────────────── */
void mem_init(Memory *mem)
{
    /* memset fills every byte of the struct with 0 */
    memset(mem, 0, sizeof(Memory));

    /* Explicitly set base address to 0x00000000 (standard RISC-V reset vector) */
    mem->base_addr = 0x00000000;
}

/* ───────────────────────────────────────────────────────────────
   mem_load_hex
   Opens a .hex file where each line contains exactly one 32-bit
   instruction encoded as 8 hexadecimal characters (no prefix).
   Example line: "00500113"

   Returns: number of instructions loaded on success, FAILURE on error.
   ─────────────────────────────────────────────────────────────── */
int mem_load_hex(Memory *mem, const char *filename)
{
    FILE   *fp;           /* File pointer for the hex file             */
    char    line[64];     /* Buffer for one line of text (64 chars max) */
    uint32_t word;        /* Holds the parsed 32-bit instruction word  */
    int     count = 0;    /* Running total of instructions loaded      */

    /* Try to open the file for reading */
    fp = fopen(filename, "r");
    if (fp == NULL) {
        /* Print error to stderr so it doesn't mix with decoded output */
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        return FAILURE;  /* Signal that loading failed */
    }

    /* Read the file one line at a time */
    while (fgets(line, sizeof(line), fp) != NULL) {

        /* Skip blank lines — fgets keeps the '\n', so a blank line is "\n" */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;  /* Skip to the next iteration */
        }

        /* Skip comment lines that start with '#' or '//' */
        if (line[0] == '#' || (line[0] == '/' && line[1] == '/')) {
            continue;
        }

        /* Trim leading whitespace so indented lines also work */
        {
            char *p = line;                /* Pointer to walk the line buffer */
            while (*p && isspace((unsigned char)*p)) {
                p++;   /* Advance past space/tab characters */
            }
            /* If we hit end-of-string, the line was all whitespace — skip it */
            if (*p == '\0') {
                continue;
            }
        }

        /* Check we haven't exceeded our fixed-size array */
        if (count >= MAX_INSTRUCTIONS) {
            fprintf(stderr, "Warning: file has more than %d instructions; truncating.\n",
                    MAX_INSTRUCTIONS);
            break;  /* Stop reading; we're full */
        }

        /* Parse the hex string into a 32-bit unsigned integer.
           "%8x" reads up to 8 hex digits; sscanf returns 1 on success.   */
        if (sscanf(line, "%8x", &word) == 1) {
            mem->data[count] = word;   /* Store the word in memory array  */
            count++;                   /* Increment instruction count      */
        }
        /* Lines that don't parse (e.g., non-hex text) are silently skipped */
    }

    fclose(fp);        /* Always close the file when done */
    mem->count = count; /* Record how many we loaded        */

    return count;   /* Return the number of instructions successfully loaded */
}

/* ───────────────────────────────────────────────────────────────
   mem_read_word
   Returns the 32-bit instruction stored at slot 'index'.
   If index is out of range, prints a warning and returns
   0xDEADBEEF (a recognisable sentinel value).
   ─────────────────────────────────────────────────────────────── */
uint32_t mem_read_word(const Memory *mem, uint32_t index)
{
    /* Bounds check: make sure index is within the loaded range */
    if (index >= mem->count) {
        fprintf(stderr, "Warning: memory read out of bounds (index=%u, count=%u)\n",
                index, mem->count);
        return 0xDEADBEEF;   /* Return a sentinel; caller must handle UNKNOWN */
    }

    return mem->data[index];   /* Return the stored instruction word */
}
