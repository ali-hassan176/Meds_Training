/* ==========================================================================
 * Day4_Exercises.c
 * MEDS Module 2 - C Language for Hardware Engineers
 * Ali Hassan | 2024-EE-176 | UET Lahore
 *
 * Menu-driven program containing all Day 4 exercise solutions
 * (Dynamic Memory, File I/O, Debugging).
 *
 * Compile:
 *   gcc -g -O0 -std=c11 -Wall -Wextra -o day4 Day4_Exercises.c
 * Run:
 *   ./day4 
 *
 * NOTE: Exercise 1 and Exercise 4 expect a hex file. If you don't have one,
 *       choose to let the program auto-generate a sample file for you.
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* ==========================================================================
 * Exercise 1: Allocate, Load, Dump, and Free Memory
 * ========================================================================== */
#define MEM_SIZE  65536

int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("load_hex_file: cannot open file");
        return -1;
    }

    char     line[32];
    uint32_t addr = 0;
    int      count = 0;

    while (fgets(line, sizeof(line), fp) != NULL && (addr + 3) < mem_size) {
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') {
            continue;
        }
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);
        memory[addr + 0] = (uint8_t)((word >>  0) & 0xFF);
        memory[addr + 1] = (uint8_t)((word >>  8) & 0xFF);
        memory[addr + 2] = (uint8_t)((word >> 16) & 0xFF);
        memory[addr + 3] = (uint8_t)((word >> 24) & 0xFF);
        addr  += 4;
        count++;
    }

    fclose(fp);
    return count;
}

void hex_dump(const uint8_t *data, size_t count)
{
    printf("Address   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\n");
    printf("--------  -----------------------------------------------\n");

    for (size_t i = 0; i < count; i += 16) {
        printf("0x%04zX   ", i);
        for (size_t j = 0; j < 16; j++) {
            if (j == 8) printf(" ");
            if (i + j < count) {
                printf("%02X ", data[i + j]);
            } else {
                printf("   ");
            }
        }
        printf(" |");
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}

static void create_sample_hex(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) { perror("Cannot create sample hex file"); return; }
    fprintf(fp, "00A28233\n00428333\n01030333\nDEADBEEF\nCAFEBABE\n00500113\n");
    fclose(fp);
    printf("Created sample hex file: '%s'\n", filename);
}

void exercise1(void)
{
    printf("\n--- Exercise 1: Allocate, Load, Dump, and Free Memory ---\n\n");

    char filename[256];
    printf("Enter hex file path (or press Enter to auto-generate a sample): ");
    getchar(); /* consume leftover newline from previous scanf */
    if (fgets(filename, sizeof(filename), stdin) == NULL) return;
    filename[strcspn(filename, "\n")] = '\0';

    if (strlen(filename) == 0) {
        strcpy(filename, "sample.hex");
        create_sample_hex(filename);
    }

    uint8_t *memory = calloc(MEM_SIZE, sizeof(uint8_t));
    if (memory == NULL) {
        fprintf(stderr, "Failed to allocate %d bytes\n", MEM_SIZE);
        return;
    }

    printf("Allocated %d bytes (%d KB) of simulated memory\n", MEM_SIZE, MEM_SIZE / 1024);

    int words_loaded = load_hex_file(filename, memory, MEM_SIZE);
    if (words_loaded < 0) {
        free(memory);
        return;
    }

    printf("Loaded %d words (%d bytes) from '%s'\n\n", words_loaded, words_loaded * 4, filename);

    size_t dump_bytes = (size_t)(words_loaded * 4);
    if (dump_bytes > 64) dump_bytes = 64;

    printf("=== First %zu bytes of loaded memory ===\n", dump_bytes);
    hex_dump(memory, dump_bytes);

    free(memory);
    memory = NULL;

    printf("\nMemory freed. Clean exit.\n");
}

/* ==========================================================================
 * Exercise 2: The Four Memory Sins - Intentional Bugs for Valgrind
 * ========================================================================== */
void sin1_memory_leak(void)
{
    printf("Sin 1: Memory Leak\n");
    uint8_t *buf = malloc(1024);
    if (buf == NULL) return;
    memset(buf, 0xAB, 1024);
    printf("  Allocated 1024 bytes at %p\n", (void*)buf);
    printf("  Written pattern 0xAB to all bytes\n");
    printf("  Returning without freeing - 1024 bytes leaked!\n");
    /* Intentionally not freed */
}

void sin2_dangling_pointer(void)
{
    printf("\nSin 2: Dangling Pointer\n");
    uint32_t *p = malloc(sizeof(uint32_t));
    if (p == NULL) return;
    *p = 42;
    printf("  Before free: *p = %u (at address %p)\n", *p, (void*)p);
    free(p);
    printf("  After free: *p = %u (GARBAGE - reading freed memory!)\n", *p);
}

void sin3_double_free(void)
{
    printf("\nSin 3: Double Free\n");
    uint32_t *p = malloc(4);
    if (p == NULL) return;
    *p = 100;
    printf("  Allocated: *p = %u at %p\n", *p, (void*)p);
    free(p);
    printf("  First free - OK\n");
    printf("  (Second free commented out to prevent actual crash)\n");
    printf("  Valgrind error: 'Invalid free() ... at address ... already freed'\n");
}

void sin4_buffer_overflow(void)
{
    printf("\nSin 4: Buffer Overflow\n");
    uint8_t *buf = malloc(4);
    if (buf == NULL) return;
    buf[0] = 0xAA; buf[1] = 0xBB; buf[2] = 0xCC; buf[3] = 0xDD;
    buf[4] = 0xEE; /* OVERFLOW */
    printf("  Allocated 4 bytes, wrote to index 4 (one past end)\n");
    printf("  Valgrind will report: Invalid write of size 1\n");
    free(buf);
    buf = NULL;
}

void exercise2(void)
{
    printf("\n--- Exercise 2: The Four Memory Sins ---\n\n");
    printf("Run this program under Valgrind to see errors:\n");
    printf("  valgrind --leak-check=full ./day4\n\n");

    int sin_number;
    printf("Which sin to demonstrate (1-4, or 0 for safe ones 1 and 3)? ");
    if (scanf("%d", &sin_number) != 1) {
        printf("Invalid input.\n");
        return;
    }

    switch (sin_number) {
        case 1: sin1_memory_leak();      break;
        case 2: sin2_dangling_pointer(); break;
        case 3: sin3_double_free();      break;
        case 4: sin4_buffer_overflow();  break;
        default:
            printf("Running safe versions (1 and 3):\n");
            sin1_memory_leak();
            sin3_double_free();
            break;
    }
}

/* ==========================================================================
 * Exercise 3: Log File Parser
 * ========================================================================== */
typedef struct {
    int pass;
    int fail;
    int skip;
    int other;
    int total;
} log_summary_t;

int parse_log(const char *filename, log_summary_t *summary)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open log '%s': ", filename);
        perror("");
        return -1;
    }

    char line[512];
    summary->pass = summary->fail = summary->skip = summary->other = 0;
    summary->total = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        summary->total++;
        if (strstr(line, "PASS") != NULL) {
            summary->pass++;
        } else if (strstr(line, "FAIL") != NULL) {
            summary->fail++;
        } else if (strstr(line, "SKIP") != NULL) {
            summary->skip++;
        } else {
            summary->other++;
        }
    }

    fclose(fp);
    return 0;
}

void create_sample_log(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Cannot create sample log");
        return;
    }

    fprintf(fp, "# RISC-V Decoder Test Log\n");
    fprintf(fp, "# Generated by test runner\n\n");
    fprintf(fp, "[PASS] ADD x1, x2, x3 -> 0x003100B3\n");
    fprintf(fp, "[PASS] SUB x2, x2, x3 -> 0x40310133\n");
    fprintf(fp, "[FAIL] ADDI x2, x0, 5 -> mismatch: expected 'addi x2, x0, 5' got 'addi x2, x0, 0'\n");
    fprintf(fp, "[PASS] LW x2, 0(x1)   -> 0x0000A103\n");
    fprintf(fp, "[SKIP] FENCE - not implemented in this version\n");
    fprintf(fp, "[PASS] BNE x1, x2, -8 -> 0xFE209CE3\n");
    fprintf(fp, "[FAIL] JAL x1, 4 - wrong immediate sign extension\n");
    fprintf(fp, "[PASS] LUI x1, 1 -> 0x000010B7\n");

    fclose(fp);
    printf("Created sample log: '%s'\n", filename);
}

void exercise3(void)
{
    printf("\n--- Exercise 3: Log File Parser ---\n\n");

    char logfile[256];
    printf("Enter log file path (or press Enter to auto-generate a sample): ");
    getchar();
    if (fgets(logfile, sizeof(logfile), stdin) == NULL) return;
    logfile[strcspn(logfile, "\n")] = '\0';

    if (strlen(logfile) == 0) {
        strcpy(logfile, "sample_test.log");
        create_sample_log(logfile);
    }

    log_summary_t summary;
    int result = parse_log(logfile, &summary);
    if (result != 0) {
        fprintf(stderr, "Failed to parse log file.\n");
        return;
    }

    printf("\n=== Test Log Summary: '%s' ===\n", logfile);
    printf("Total lines:  %d\n", summary.total);
    printf("PASS:         %d\n", summary.pass);
    printf("FAIL:         %d\n", summary.fail);
    printf("SKIP:         %d\n", summary.skip);
    printf("Other:        %d\n\n", summary.other);

    int tested = summary.pass + summary.fail;
    if (tested > 0) {
        printf("Pass rate: %.1f%% (%d/%d)\n",
               100.0 * summary.pass / tested, summary.pass, tested);
    }
}

/* ==========================================================================
 * Exercise 4: Little-Endian Storage - Load and Verify 0xDEADBEEF
 * ========================================================================== */
void exercise4(void)
{
    printf("\n--- Exercise 4: Little-Endian Storage Verification ---\n\n");

    const char *testfile = "test_deadbeef.hex";
    FILE *fp = fopen(testfile, "w");
    if (fp == NULL) { perror("Cannot create test file"); return; }
    fputs("DEADBEEF\n", fp);
    fclose(fp);

    uint8_t *mem = calloc(4, sizeof(uint8_t));
    if (mem == NULL) { fprintf(stderr, "calloc failed\n"); return; }

    int words = load_hex_file(testfile, mem, 4);
    printf("Loaded %d word(s)\n", words);

    printf("\nLittle-endian storage of 0xDEADBEEF:\n");
    printf("  mem[0] = 0x%02X (expected 0xEF - %s)\n", mem[0], mem[0] == 0xEF ? "PASS" : "FAIL");
    printf("  mem[1] = 0x%02X (expected 0xBE - %s)\n", mem[1], mem[1] == 0xBE ? "PASS" : "FAIL");
    printf("  mem[2] = 0x%02X (expected 0xAD - %s)\n", mem[2], mem[2] == 0xAD ? "PASS" : "FAIL");
    printf("  mem[3] = 0x%02X (expected 0xDE - %s)\n", mem[3], mem[3] == 0xDE ? "PASS" : "FAIL");

    uint32_t reconstructed =
        ((uint32_t)mem[3] << 24) |
        ((uint32_t)mem[2] << 16) |
        ((uint32_t)mem[1] <<  8) |
        ((uint32_t)mem[0] <<  0);

    printf("\nReconstructed: 0x%08X (expected 0xDEADBEEF - %s)\n",
           reconstructed, reconstructed == 0xDEADBEEF ? "PASS" : "FAIL");

    free(mem);
    mem = NULL;

    printf("\nValgrind should show: 0 errors, no leaks.\n");
}

/* ==========================================================================
 * Exercise 6 (Bonus): Full Argument Parsing
 * ========================================================================== */
typedef struct {
    const char *hex_file;
    int         mem_size;
    uint32_t    start_addr;
    int         trace;
    int         verbose;
} config_t;

void exercise6(void)
{
    printf("\n--- Exercise 6 (Bonus): Full Argument Parsing ---\n\n");
    printf("This exercise normally parses command-line flags such as\n");
    printf("--mem-size, --start-addr, --trace and --verbose.\n");
    printf("Since this program runs interactively, enter the equivalent values:\n\n");

    config_t cfg;
    char hexfile[256];

    printf("Hex file name: ");
    getchar();
    fgets(hexfile, sizeof(hexfile), stdin);
    hexfile[strcspn(hexfile, "\n")] = '\0';
    cfg.hex_file = hexfile;

    printf("Memory size in bytes (default 65536): ");
    if (scanf("%d", &cfg.mem_size) != 1 || cfg.mem_size <= 0) cfg.mem_size = 65536;

    char addr_str[32];
    printf("Start address in hex (default 0): ");
    scanf("%31s", addr_str);
    cfg.start_addr = (uint32_t)strtoul(addr_str, NULL, 0);

    printf("Enable trace? (1=yes, 0=no): ");
    scanf("%d", &cfg.trace);

    printf("Enable verbose? (1=yes, 0=no): ");
    scanf("%d", &cfg.verbose);

    printf("\n=== Configuration ===\n");
    printf("Hex file:    %s\n", cfg.hex_file);
    printf("Memory size: %d bytes (%d KB)\n", cfg.mem_size, cfg.mem_size / 1024);
    printf("Start addr:  0x%08X\n", cfg.start_addr);
    printf("Trace:       %s\n", cfg.trace ? "ON" : "OFF");
    printf("Verbose:     %s\n", cfg.verbose ? "ON" : "OFF");

    printf("\n(Simulation would start here with the above settings)\n");
}

/* ==========================================================================
 * Menu / Driver
 * ========================================================================== */
void print_menu(void)
{
    printf("\n=====================================================\n");
    printf(" Day 4 Exercises - Dynamic Memory, File I/O, Debugging\n");
    printf("=====================================================\n");
    printf(" 1. Allocate, Load, Dump, and Free Memory\n");
    printf(" 2. The Four Memory Sins (run with Valgrind)\n");
    printf(" 3. Log File Parser\n");
    printf(" 4. Little-Endian Storage Verification\n");
    printf(" 6. (Bonus) Full Argument Parsing\n");
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
            case 4: exercise4(); break;
            case 6: exercise6(); break;
            case 0: printf("\nExiting. Goodbye!\n"); break;
            default: printf("\nInvalid choice. Please enter 0,1,2,3,4 or 6.\n"); break;
        }
    } while (choice != 0);

    return 0;
}
