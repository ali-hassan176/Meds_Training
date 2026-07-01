/* main.c — Entry point for the RISC-V RV32I Instruction Decoder
   Handles command-line argument parsing, orchestrates loading
   and decoding, and prints the final summary.                    */

#include "../include/common.h"    /* Shared constants (SUCCESS, FAILURE, etc.) */
#include "../include/memory.h"    /* Memory struct and loader                  */
#include "../include/decoder.h"   /* decode_instruction, print_instruction     */

/* ═══════════════════════════════════════════════════════════════
   print_usage — shows the user how to invoke the program
   ═══════════════════════════════════════════════════════════════ */
static void print_usage(const char *prog_name)
{
    /* Print to stderr so it doesn't pollute normal output */
    fprintf(stderr, "Usage: %s <hexfile>\n", prog_name);
    fprintf(stderr, "  hexfile  Path to a .hex file (one 32-bit hex word per line)\n");
    fprintf(stderr, "Example: %s test/programs/mixed.hex\n", prog_name);
}

/* ═══════════════════════════════════════════════════════════════
   main — program entry point
   argc = number of command-line arguments (including program name)
   argv = array of argument strings
   ═══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    Memory      mem;              /* Memory subsystem instance           */
    DecodedInstr instr;           /* Decoded instruction (reused per iter)*/
    uint32_t    i;                /* Loop index (unsigned, like addresses) */
    int         total   = 0;      /* Total instructions loaded           */
    int         valid   = 0;      /* Count of successfully decoded       */
    int         unknown = 0;      /* Count of UNKNOWN instructions       */
    uint32_t    pc;               /* Program counter for current instr   */

    /* ── Step 1: Validate command-line arguments ────────────────── */
    if (argc != 2) {
        /* Exactly one argument expected: the hex filename */
        fprintf(stderr, "Error: expected exactly 1 argument, got %d\n", argc - 1);
        print_usage(argv[0]);
        return EXIT_FAILURE;   /* Non-zero return = error to the shell */
    }

    /* ── Step 2: Print the program banner ───────────────────────── */
    printf("RISC-V RV32I Instruction Decoder\n");
    printf("================================\n");

    /* ── Step 3: Initialise memory and load the hex file ─────────── */
    mem_init(&mem);   /* Zero out the memory struct */

    total = mem_load_hex(&mem, argv[1]);   /* argv[1] = the filename argument */
    if (total == FAILURE) {
        /* mem_load_hex already printed an error message */
        return EXIT_FAILURE;
    }

    /* Report how many instructions we found */
    printf("Loaded %d instructions from %s\n\n", total, argv[1]);

    /* ── Step 4: Print the column header ────────────────────────── */
    print_header();

    /* ── Step 5: Decode and print each instruction ───────────────── */
    for (i = 0; i < (uint32_t)total; i++) {

        /* Compute the PC for instruction i:
           PC starts at base_addr and increments by WORD_SIZE (4) per instruction */
        pc = mem.base_addr + (i * WORD_SIZE);

        /* Read the raw 32-bit word from memory */
        uint32_t raw = mem_read_word(&mem, i);

        /* Decode the instruction — fills in the 'instr' struct */
        decode_instruction(raw, pc, &instr);

        /* Print one row of the output table */
        print_instruction(&instr);

        /* Count valid vs unknown for the summary line */
        if (instr.valid) {
            valid++;      /* Successfully decoded */
        } else {
            unknown++;    /* Unrecognised opcode */
        }
    }

    /* ── Step 6: Print the summary footer ───────────────────────── */
    printf("\nDecoded %d instructions (%d valid, %d unknown)\n",
           total, valid, unknown);

    return EXIT_SUCCESS;   /* 0 = success to the shell */
}
