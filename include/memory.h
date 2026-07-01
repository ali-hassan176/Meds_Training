#ifndef MEMORY_H   /* Include guard */
#define MEMORY_H

#include "common.h"   /* Pull in shared types and constants */

/* ═══════════════════════════════════════════════════════════════
   Memory subsystem — responsible for loading a .hex file and
   providing word-level access to stored instructions.
   ═══════════════════════════════════════════════════════════════ */

/* Memory structure: a simple flat array of 32-bit words.
   'count' tracks how many instructions were loaded.             */
typedef struct {
    uint32_t data[MAX_INSTRUCTIONS];   /* Instruction words, one per slot     */
    uint32_t count;                    /* Number of words actually loaded     */
    uint32_t base_addr;                /* Starting PC address (usually 0)     */
} Memory;

/* ═══════════════════════════════════════════════════════════════
   FUNCTION PROTOTYPES — defined in memory.c
   ═══════════════════════════════════════════════════════════════ */

/* mem_init: zeroes out the Memory struct so it starts clean.    */
void mem_init(Memory *mem);

/* mem_load_hex: reads 'filename' (one 8-digit hex word per line)
   into mem->data[].  Returns number of instructions loaded,
   or FAILURE on error.                                          */
int mem_load_hex(Memory *mem, const char *filename);

/* mem_read_word: returns the 32-bit word at word-index 'index'.
   Returns 0xDEADBEEF and prints a warning if out of range.     */
uint32_t mem_read_word(const Memory *mem, uint32_t index);

#endif /* MEMORY_H */
