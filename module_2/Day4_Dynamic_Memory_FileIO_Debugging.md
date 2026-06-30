# Day 4 — Dynamic Memory, File I/O & Debugging

**MEDS Module 2 | C Language for Hardware Engineers**
**Ali Hassan | Roll No. 2024-EE-176 | UET Lahore**

---

## What Is This Day About?

Day 3 taught you how to **structure** data. Day 4 teaches you how to:

1. **Allocate memory at runtime** — you don't always know at compile time how much memory you need. `malloc` and `calloc` let you ask the OS for memory while the program is running.
2. **Read and write files** — load machine code from `.hex` files into simulated memory, just like `$readmemh()` does in Verilog.
3. **Parse command-line arguments** — make your tool configurable: `./decoder program.hex --verbose`.
4. **Debug with GDB** — step through code line by line, inspect variables, find bugs.
5. **Check memory with Valgrind** — catch leaks, bad reads, and bad writes that C silently ignores.

---

## 7.1 Dynamic Memory Allocation

### The Stack vs The Heap

When you write `int x = 5;` or `Memory mem;`, the variable goes on the **stack** — a fixed-size region automatically managed by the CPU. Stack memory is fast but limited (~1–8 MB on most systems). It disappears when the function returns.

When you write `malloc(...)`, memory comes from the **heap** — a large region managed by your program. Heap memory persists until you explicitly free it.

```
Program memory layout:
┌─────────────────────┐ high addresses
│       Stack         │ ← grows down; local variables, function frames
│          ↓          │
│    (free space)     │
│          ↑          │
│       Heap          │ ← grows up; malloc/calloc/realloc
├─────────────────────┤
│  Global/Static data │
├─────────────────────┤
│  Code (read-only)   │
└─────────────────────┘ low addresses (0x00000000)
```

### The Four Allocation Functions

```c
#include <stdlib.h>   /* malloc, calloc, realloc, free are all here         */
#include <string.h>   /* memset (used to zero malloc'd memory)              */

/* ── malloc ─────────────────────────────────────────────────────────────── */
/* malloc(bytes) — allocate 'bytes' bytes of UNINITIALISED memory.
   Returns: void* (a generic pointer) on success, NULL on failure.
   The memory contains GARBAGE — whatever was there before.
   Cast to the correct type.                                                  */
uint32_t *memory = malloc(4096 * sizeof(uint32_t));
/*                        ^^^^ number of elements
                                ^^^^^^^^^^^^^^^^^ bytes per element
   4096 * 4 = 16384 bytes = 16 KB total                                       */

/* ALWAYS check for NULL — malloc fails if system is out of memory            */
if (memory == NULL) {
    fprintf(stderr, "Memory allocation failed!\n");   /* Print error to stderr */
    exit(EXIT_FAILURE);   /* EXIT_FAILURE = 1 — tells shell something went wrong */
}

/* ── calloc ─────────────────────────────────────────────────────────────── */
/* calloc(count, size) — allocate count*size bytes, INITIALISED TO ZERO.
   "c" in calloc stands for "cleared".
   Prefer this for arrays — avoids bugs from uninitialised data.              */
uint8_t *mem = calloc(65536, sizeof(uint8_t));
/*                    ^^^^^  number of elements (65536 = 64K)
                              ^^^^^^^^^^^^^^^^^^^^^ size of each (1 byte)     */
/* 65536 bytes, all zeros. Simulates a clean power-on memory.                 */

/* ── realloc ────────────────────────────────────────────────────────────── */
/* realloc(ptr, new_size) — resize an existing allocation.
   May move the data to a new location (old pointer becomes invalid!).
   Returns: new pointer (may differ from old), or NULL if failed.
   NEVER do: ptr = realloc(ptr, new_size); — if realloc returns NULL,
   you lose the original pointer and create a leak.                           */
uint32_t *bigger = realloc(memory, 8192 * sizeof(uint32_t));
if (bigger == NULL) {
    /* realloc failed — 'memory' is still valid! Do not free 'memory' here
       unless you are done with it.                                            */
    fprintf(stderr, "Realloc failed — keeping original size\n");
} else {
    memory = bigger;   /* Safe to update only if realloc succeeded            */
}

/* ── free ───────────────────────────────────────────────────────────────── */
/* free(ptr) — return memory to the OS.
   Call exactly ONCE per allocation.
   After free, never use the pointer — it points to invalid (freed) memory.  */
free(memory);
memory = NULL;   /* Set to NULL immediately — catching bugs: NULL dereference
                    crashes immediately; dangling pointer crashes mysteriously later */

free(mem);
mem = NULL;
```

---

### The Four Deadly Memory Sins

Every C memory bug falls into one of four categories. Understanding them helps you recognise Valgrind's error messages.

#### Sin 1: Memory Leak — `malloc` Without `free`

```c
void leaky_function(void)
{
    uint8_t *buf = malloc(1024);   /* Allocate 1 KB                          */
    if (buf == NULL) return;

    /* ... use buf ... */

    /* BUG: return without free(buf) — 1 KB is leaked every call!
       The OS reclaims it when the process ends, but a loop calling this
       function 1000 times wastes 1 MB.
       In an embedded system with no OS, you can never get it back.          */
}   /* buf goes out of scope — original malloc address is lost forever        */

/* FIX: always free before returning */
void fixed_function(void)
{
    uint8_t *buf = malloc(1024);
    if (buf == NULL) return;
    /* ... use buf ... */
    free(buf);   /* Return memory before function exits                       */
    buf = NULL;
}
```

#### Sin 2: Dangling Pointer — Using Memory After `free`

```c
uint32_t *p = malloc(sizeof(uint32_t));
*p = 42;          /* Write 42 into the allocated memory                       */
free(p);          /* Memory is returned to the OS                             */

/* BUG: p still holds the old address, but that memory is now freed!
   The OS may have given it to another part of the program.
   Reading it gives garbage. Writing to it corrupts other data.              */
printf("%u\n", *p);   /* UNDEFINED BEHAVIOUR — could print anything or crash */
*p = 100;             /* UNDEFINED BEHAVIOUR — may corrupt unrelated data     */

/* FIX: set to NULL immediately after free */
free(p);
p = NULL;
/* Now any accidental use of p will crash immediately (NULL dereference)
   which is MUCH better than silent corruption.                               */
```

#### Sin 3: Double Free — Calling `free` Twice

```c
uint32_t *p = malloc(4);
free(p);   /* First free — OK                                                 */
free(p);   /* SECOND FREE on same pointer — corrupts heap metadata!
              The heap allocator keeps its own data structures about what is
              free/allocated. A double free corrupts those structures and can
              allow an attacker to execute arbitrary code (security bug).     */

/* FIX: set to NULL after free — calling free(NULL) is always safe */
free(p);
p = NULL;
free(p);   /* free(NULL) does nothing — no crash, no corruption              */
```

#### Sin 4: Buffer Overflow — Writing Past Allocated Boundary

```c
uint8_t *buf = malloc(4);   /* Allocate 4 bytes                              */
buf[0] = 0xAA;   /* OK — index 0 is within bounds (0 to 3)                  */
buf[3] = 0xBB;   /* OK — index 3 is the last valid byte                     */

/* BUG: index 4 is one past the end — writes into the next heap block's data */
buf[4] = 0xFF;   /* BUFFER OVERFLOW — undefined behaviour
                    May corrupt malloc's internal bookkeeping.
                    May corrupt the next variable on the heap.
                    In C, this does NOT cause an immediate crash.
                    The crash (if any) happens much later, far from this line. */

/* FIX: always stay within bounds; use constants/sizeof to track sizes       */
size_t size = 4;
for (size_t i = 0; i < size; i++) {
    buf[i] = (uint8_t)i;   /* Safe: i is always 0, 1, 2, or 3               */
}
free(buf);
buf = NULL;
```

---

## 7.2 File I/O

### Why File I/O Matters for Hardware Engineers

In Verilog, you use `$readmemh("program.hex", memory)` to load a hex file into a simulation memory. In your C simulator, you write `load_hex_file()`. Both do the same thing: read hex-encoded numbers from a text file into an array.

### Reading Text Files Line by Line

```c
/* Read a file one line at a time.
   Pattern: fopen → loop with fgets → fclose.
   Always check fopen for NULL (file not found, permission denied, etc.)     */

FILE *fp = fopen("test_results.log", "r");
/*          ^^^^^^ function: open a file
                   ^^^^^^^^^^^^^^^^^^^ path to the file (relative or absolute)
                                        ^^^ mode: "r" = read-only
   Returns: FILE* (a pointer to an internal file structure), or NULL on error */

if (fp == NULL) {
    /* perror prints "Failed to open file: <system error message>"
       The system error (e.g., "No such file or directory") is added automatically. */
    perror("Failed to open file");
    return -1;   /* Signal error to caller                                    */
}

char line[256];   /* Buffer to hold one line of text. 256 chars is generous.
                    fgets will not overflow this buffer (we pass sizeof(line)).*/

while (fgets(line, sizeof(line), fp) != NULL) {
    /*   ^^^^^ reads one line (up to sizeof(line)-1 chars) from fp into line.
               Returns NULL at end-of-file or on error.
               Keeps the '\n' newline character at the end of line[].         */

    /* strstr(haystack, needle) — find 'needle' inside 'haystack'.
       Returns a pointer to the first match, or NULL if not found.            */
    if (strstr(line, "FAIL") != NULL) {
        printf("Found failure: %s", line);   /* 'line' already has '\n' at end */
    }
}

fclose(fp);   /* ALWAYS close — flushes buffers and releases the file handle  */

/* Writing to a file */
FILE *out = fopen("report.txt", "w");   /* "w" = write (creates or overwrites) */
if (out == NULL) {
    perror("Cannot create report.txt");
    return -1;
}
fprintf(out, "Test Results:\n");                          /* Write to FILE*    */
fprintf(out, "Pass: %d, Fail: %d\n", pass_count, fail_count);
fclose(out);   /* Close after writing — ensures data is flushed to disk        */
```

### Loading a Hex File into Simulated Memory

This is the exact technique used in hardware simulation — both in C simulators and in Verilog with `$readmemh`.

```c
/* load_hex_file — load a .hex file (one 32-bit word per line) into a byte array.
   RISC-V is little-endian: the LEAST significant byte goes at the lowest address.
   For word 0xDEADBEEF:
     memory[addr+0] = 0xEF  (least significant byte — bits [7:0])
     memory[addr+1] = 0xBE  (next byte         — bits [15:8])
     memory[addr+2] = 0xAD  (next byte         — bits [23:16])
     memory[addr+3] = 0xDE  (most significant byte — bits [31:24])

   Parameters:
     filename — path to the .hex file
     memory   — byte array to load into
     mem_size — total size of the byte array (for bounds checking)
   Returns: number of words loaded, or -1 on error                            */
int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size)
{
    FILE *fp = fopen(filename, "r");   /* Open for reading                    */
    if (!fp) {                          /* !fp is same as (fp == NULL)         */
        perror("Cannot open hex file");
        return -1;
    }

    char     line[32];    /* Buffer for one line — 8 hex digits + newline = 9 chars */
    uint32_t addr = 0;    /* Current byte address in simulated memory        */

    while (fgets(line, sizeof(line), fp) && addr + 3 < mem_size) {
        /* 'addr + 3 < mem_size' ensures we have room for 4 bytes            */

        /* strtoul(str, endptr, base) — convert string to unsigned long.
           str    = the line of text
           NULL   = we don't need to know where parsing stopped
           16     = base 16 (hexadecimal)
           Returns: the numeric value                                         */
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);

        /* Store word as 4 bytes in little-endian order.
           0xFF masking ensures we only take 8 bits each time.
           '>>' right-shifts to bring the next byte to the low 8 bits.       */
        memory[addr + 0] = (word >>  0) & 0xFF;   /* bits [7:0]   */
        memory[addr + 1] = (word >>  8) & 0xFF;   /* bits [15:8]  */
        memory[addr + 2] = (word >> 16) & 0xFF;   /* bits [23:16] */
        memory[addr + 3] = (word >> 24) & 0xFF;   /* bits [31:24] */

        addr += 4;   /* Advance to next word's byte address                  */
    }

    fclose(fp);
    return (int)(addr / 4);   /* Number of words = bytes / 4                 */
}
```

---

## 7.3 Command-Line Arguments

```c
/* main receives:
   argc — argument count: number of strings in argv (always >= 1)
   argv — argument vector: array of C strings
     argv[0] = the program name (e.g., "./bin/riscv-decoder")
     argv[1] = first argument  (e.g., "program.hex")
     argv[2] = second argument (e.g., "--verbose")
     ...                                                                      */
int main(int argc, char *argv[])
{
    /* Check minimum required arguments */
    if (argc < 2) {
        /* argc is 1 — only program name, no file argument                    */
        fprintf(stderr, "Usage: %s <hex_file> [options]\n", argv[0]);
        return EXIT_FAILURE;   /* Non-zero return signals error to the shell  */
    }

    /* argv[1] is the hex file path */
    const char *hex_file = argv[1];   /* 'const' — we won't modify the string */
    int verbose  = 0;                  /* Flag: 0=silent, 1=verbose            */
    int mem_size = 65536;              /* Default: 64 KB                       */

    /* Parse optional flags starting from argv[2]                             */
    int i;
    for (i = 2; i < argc; i++) {
        /* strcmp returns 0 if strings are identical                          */
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = 1;   /* User passed --verbose or -v                    */

        } else if (strcmp(argv[i], "--mem-size") == 0 && i + 1 < argc) {
            /* '--mem-size' takes the next argument as its value.
               'i + 1 < argc' ensures there IS a next argument.              */
            mem_size = atoi(argv[i + 1]);   /* atoi: ASCII-to-integer conversion */
            i++;   /* Skip the value argument in the next loop iteration     */
        }
    }

    printf("Loading: %s (verbose=%d, mem_size=%d)\n",
           hex_file, verbose, mem_size);

    return EXIT_SUCCESS;   /* 0 = success                                     */
}
```

---

## 7.4 Debugging with GDB

GDB (GNU Debugger) lets you pause your program at any line, inspect variables, and step through code one statement at a time.

### Essential GDB Workflow

```bash
# Step 1: compile with -g (debug info) and -O0 (no optimisation)
# -g: embed variable names, line numbers, types in the binary
# -O0: disable optimisation so code runs exactly as written (optimisation reorders code)
gcc -g -O0 -o decoder main.c decoder.c memory.c

# Step 2: start GDB
gdb ./decoder

# Step 3: inside GDB prompt (gdb)
(gdb) break main              # Pause at the first line of main()
(gdb) break decoder.c:42      # Pause at line 42 of decoder.c
(gdb) run test/programs/mixed.hex   # Start the program with arguments

# Navigation
(gdb) next        # Execute current line; step OVER function calls
(gdb) step        # Execute current line; step INTO function calls
(gdb) continue    # Run until next breakpoint or program ends
(gdb) finish      # Run until current function returns, then pause

# Inspecting data
(gdb) print x              # Print variable x (in its natural format)
(gdb) print/x x            # Print x in hexadecimal
(gdb) print/d x            # Print x as signed decimal
(gdb) print/t x            # Print x in binary (t = two's complement)
(gdb) print cpu.x[5]       # Print element 5 of array inside struct
(gdb) print instr->funct3  # Print field of pointer-to-struct
(gdb) info locals          # Print ALL local variables of current function
(gdb) backtrace            # Show call stack (which function called which)
(gdb) watch variable       # Pause whenever 'variable' changes (watchpoint)
(gdb) quit                 # Exit GDB
```

### GDB Tip: Print a RISC-V Instruction in Binary

```bash
(gdb) print/t raw_instruction
# prints something like: 0b00000000010100000000000100010011
# You can now manually check which bit fields are set
```

---

## 7.5 Memory Checking with Valgrind

Valgrind runs your program in a controlled sandbox and reports every memory error.

```bash
# Basic usage
valgrind --leak-check=full ./decoder test/programs/mixed.hex

# Full output options:
valgrind \
  --leak-check=full \       # Show all leaks in detail (not just totals)
  --show-leak-kinds=all \   # Show definitely/indirectly/possibly lost memory
  --track-origins=yes \     # Show WHERE uninitialised values came from
  --error-exitcode=1 \      # Return non-zero if errors found (useful in make)
  ./decoder test/programs/mixed.hex
```

### Reading Valgrind Output

```
==12345== Invalid read of size 4        ← reading 4 bytes from invalid memory
==12345==    at 0x401234: decode (decoder.c:45)  ← in function 'decode', line 45
==12345==    by 0x401500: main (main.c:23)        ← called from main, line 23
==12345==  Address 0x5204040 is 0 bytes after a block of size 16 alloc'd
          ← writing/reading exactly 1 element past a 16-byte allocation

==12345== LEAK SUMMARY:
==12345==    definitely lost: 1,024 bytes in 1 blocks   ← malloc without free
==12345==    indirectly lost: 0 bytes in 0 blocks
==12345==      possibly lost: 0 bytes in 0 blocks

==12345== ERROR SUMMARY: 1 errors from 1 contexts  ← total error count
```

**Clean run (what you want):**
```
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

---

## Day 4 Exercises — Fully Solved

---

### Exercise 1: Allocate, Load, Dump, and Free Memory

**Task:** Allocate 64 KB, load a hex file, dump the first 64 bytes, free everything. Verify zero leaks with Valgrind.

```c
/* exercise1_day4.c — Allocate memory, load hex, hex-dump, and free cleanly */

#include <stdio.h>    /* printf, fprintf, fopen, fgets, fclose, perror       */
#include <stdlib.h>   /* malloc, calloc, free, strtoul, exit                 */
#include <stdint.h>   /* uint8_t, uint32_t                                   */

#define MEM_SIZE  65536   /* 64 KB = 65536 bytes                             */

/* load_hex_file — loads a .hex file (one 32-bit word per line, no prefix)
   into the byte array in little-endian order.
   Returns: number of words loaded, or -1 on error.                          */
int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size)
{
    FILE *fp = fopen(filename, "r");   /* Open file for reading              */
    if (fp == NULL) {
        perror("load_hex_file: cannot open file");
        return -1;
    }

    char     line[32];    /* Buffer for one line (max 9 chars + null)        */
    uint32_t addr = 0;    /* Current byte address                            */
    int      count = 0;   /* Word count                                      */

    /* Read lines until EOF or memory is full (need 4 bytes per word)        */
    while (fgets(line, sizeof(line), fp) != NULL && (addr + 3) < mem_size) {

        /* Skip blank lines and comment lines starting with '#'              */
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') {
            continue;
        }

        /* Parse hex string to 32-bit unsigned integer, base 16              */
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);

        /* Store in little-endian byte order (RISC-V convention)             */
        memory[addr + 0] = (uint8_t)((word >>  0) & 0xFF);   /* LSB first  */
        memory[addr + 1] = (uint8_t)((word >>  8) & 0xFF);
        memory[addr + 2] = (uint8_t)((word >> 16) & 0xFF);
        memory[addr + 3] = (uint8_t)((word >> 24) & 0xFF);   /* MSB last   */

        addr  += 4;   /* Move to next word's address                         */
        count++;      /* Increment word count                                */
    }

    fclose(fp);
    return count;
}

/* hex_dump — print 'count' bytes from 'data' in classic hex dump format:
   address | 16 bytes as hex | 16 bytes as printable ASCII                  */
void hex_dump(const uint8_t *data, size_t count)
{
    size_t i;
    printf("Address   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\n");
    printf("--------  -----------------------------------------------\n");

    for (i = 0; i < count; i += 16) {
        /* Print the address of this row                                      */
        printf("0x%04zX   ", i);

        /* Print 16 bytes as hex, with a gap after byte 7 for readability    */
        size_t j;
        for (j = 0; j < 16; j++) {
            if (j == 8) printf(" ");          /* Gap in the middle            */
            if (i + j < count) {
                printf("%02X ", data[i + j]); /* 2-digit uppercase hex        */
            } else {
                printf("   ");                /* Pad if fewer than 16 bytes   */
            }
        }

        /* Print ASCII representation: printable chars as-is, others as '.' */
        printf(" |");
        for (j = 0; j < 16 && i + j < count; j++) {
            uint8_t c = data[i + j];
            /* isprint() checks if character is printable (space to ~)       */
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}

int main(int argc, char *argv[])
{
    /* Validate arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hexfile>\n", argv[0]);
        return 1;
    }

    /* calloc: allocate MEM_SIZE bytes, all initialised to 0.
       Using uint8_t* because memory is byte-addressable.                    */
    uint8_t *memory = calloc(MEM_SIZE, sizeof(uint8_t));
    if (memory == NULL) {
        fprintf(stderr, "Failed to allocate %d bytes\n", MEM_SIZE);
        return 1;
    }

    printf("Allocated %d bytes (%d KB) of simulated memory\n",
           MEM_SIZE, MEM_SIZE / 1024);

    /* Load the hex file */
    int words_loaded = load_hex_file(argv[1], memory, MEM_SIZE);
    if (words_loaded < 0) {
        free(memory);   /* Must free even on error path!                     */
        memory = NULL;
        return 1;
    }

    printf("Loaded %d words (%d bytes) from '%s'\n\n",
           words_loaded, words_loaded * 4, argv[1]);

    /* Dump the first 64 bytes (first 16 instructions) */
    size_t dump_bytes = (size_t)(words_loaded * 4);
    if (dump_bytes > 64) dump_bytes = 64;   /* Cap at 64 bytes for display   */

    printf("=== First %zu bytes of loaded memory ===\n", dump_bytes);
    hex_dump(memory, dump_bytes);

    /* FREE — Valgrind will report a leak if this is missing! */
    free(memory);
    memory = NULL;   /* NULL after free — safety habit                       */

    printf("\nMemory freed. Clean exit.\n");
    return 0;   /* EXIT_SUCCESS                                               */
}
```

---

### Exercise 2: The Four Memory Sins — Intentional Bugs for Valgrind

**Task:** Write each sin in a separate function. Run each under Valgrind and observe the error message.

```c
/* exercise2_day4.c — Demonstrate all 4 memory sins for Valgrind observation
   Compile: gcc -g -O0 -o mem_sins exercise2_day4.c
   Run:     valgrind --leak-check=full ./mem_sins 1  (or 2, 3, 4)           */

#include <stdio.h>    /* printf, fprintf                                      */
#include <stdlib.h>   /* malloc, free, exit, atoi                            */
#include <string.h>   /* memset                                               */
#include <stdint.h>   /* uint8_t, uint32_t                                   */

/* Sin 1: Memory Leak — malloc without matching free */
void sin1_memory_leak(void)
{
    printf("Sin 1: Memory Leak\n");

    /* Allocate 1 KB — but never free it! */
    uint8_t *buf = malloc(1024);
    if (buf == NULL) return;

    memset(buf, 0xAB, 1024);   /* Write a pattern to prove it was used      */
    printf("  Allocated 1024 bytes at %p\n", (void*)buf);
    printf("  Written pattern 0xAB to all bytes\n");

    /* BUG: function returns WITHOUT calling free(buf)                        */
    /* Valgrind will report: "definitely lost: 1,024 bytes in 1 blocks"      */
    printf("  Returning without freeing — 1024 bytes leaked!\n");
}

/* Sin 2: Dangling Pointer — using pointer after free */
void sin2_dangling_pointer(void)
{
    printf("\nSin 2: Dangling Pointer\n");

    uint32_t *p = malloc(sizeof(uint32_t));   /* Allocate 4 bytes            */
    if (p == NULL) return;

    *p = 42;   /* Write a value — fine                                        */
    printf("  Before free: *p = %u (at address %p)\n", *p, (void*)p);

    free(p);   /* Return the 4 bytes to the heap                             */
    /* p now points to freed (invalid) memory!                               */

    /* BUG: reading freed memory — undefined behaviour!                       */
    /* Valgrind reports: "Invalid read of size 4"                            */
    printf("  After free: *p = %u (GARBAGE — reading freed memory!)\n", *p);
    /* DO NOT write to p here — corrupts heap metadata                        */
}

/* Sin 3: Double Free */
void sin3_double_free(void)
{
    printf("\nSin 3: Double Free\n");

    uint32_t *p = malloc(4);
    if (p == NULL) return;

    *p = 100;
    printf("  Allocated: *p = %u at %p\n", *p, (void*)p);

    free(p);   /* First free — correct                                        */
    printf("  First free — OK\n");

    /* BUG: calling free again on the same pointer!                           */
    /* Valgrind reports: "Invalid free() / delete / delete[] / realloc()"    */
    /* In some C library implementations this corrupts heap and causes
       a crash or security vulnerability (heap exploitation technique)        */
    /* free(p); */   /* COMMENTED OUT — uncomment to see Valgrind error      */
    printf("  (Second free commented out to prevent actual crash)\n");
    printf("  Valgrind error: 'Invalid free() ... at address ... already freed'\n");
}

/* Sin 4: Buffer Overflow — writing past the end of an allocation */
void sin4_buffer_overflow(void)
{
    printf("\nSin 4: Buffer Overflow\n");

    uint8_t *buf = malloc(4);   /* Allocate exactly 4 bytes (indices 0–3)    */
    if (buf == NULL) return;

    buf[0] = 0xAA;   /* OK — index 0                                         */
    buf[1] = 0xBB;   /* OK — index 1                                         */
    buf[2] = 0xCC;   /* OK — index 2                                         */
    buf[3] = 0xDD;   /* OK — index 3 (last valid byte)                       */

    /* BUG: index 4 is 1 byte past the end of our 4-byte allocation          */
    /* Valgrind reports: "Invalid write of size 1" at 1 byte past the block  */
    buf[4] = 0xEE;   /* OVERFLOW — corrupts whatever is at address (buf+4)!  */

    printf("  Allocated 4 bytes, wrote to index 4 (one past end)\n");
    printf("  Valgrind will report: Invalid write of size 1\n");

    free(buf);
    buf = NULL;
}

int main(int argc, char *argv[])
{
    int sin_number = 0;

    if (argc >= 2) {
        sin_number = atoi(argv[1]);   /* Convert argument "1"/"2"/"3"/"4" to int */
    }

    printf("=== Memory Sin Demonstration ===\n");
    printf("Run with Valgrind to see errors:\n");
    printf("  valgrind --leak-check=full ./mem_sins <1|2|3|4>\n\n");

    switch (sin_number) {
        case 1: sin1_memory_leak();     break;
        case 2: sin2_dangling_pointer(); break;
        case 3: sin3_double_free();     break;
        case 4: sin4_buffer_overflow(); break;
        default:
            printf("Running all sins (safe versions):\n");
            sin1_memory_leak();
            sin3_double_free();
            /* Note: sin2 and sin4 have actual undefined behaviour; only
               demonstrating 1 and 3 in the default run for safety          */
            break;
    }

    return 0;
}
```

---

### Exercise 3: Log File Parser

**Task:** Read a simulation log file, count PASS/FAIL/SKIP, handle file-not-found gracefully.

```c
/* exercise3_day4.c — Parse a simulation log and report results */

#include <stdio.h>    /* printf, fprintf, fopen, fgets, fclose, perror       */
#include <string.h>   /* strstr, strncmp                                     */
#include <stdlib.h>   /* EXIT_FAILURE, EXIT_SUCCESS                          */

/* Counts for each result type */
typedef struct {
    int pass;   /* Lines containing "PASS"                                   */
    int fail;   /* Lines containing "FAIL"                                   */
    int skip;   /* Lines containing "SKIP"                                   */
    int other;  /* Lines with none of the above                              */
    int total;  /* Total lines processed                                     */
} log_summary_t;

/* parse_log — read the log file and count outcomes.
   Parameters:
     filename — path to the log file
     summary  — pointer to struct where counts are written                   */
int parse_log(const char *filename, log_summary_t *summary)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        /* perror prints the OS error message — much more informative than just
           "error". Example: "Cannot open log: No such file or directory"    */
        fprintf(stderr, "Cannot open log '%s': ", filename);
        perror("");   /* perror with empty string appends the OS error       */
        return -1;
    }

    char line[512];   /* Buffer for one log line — 512 is generous           */

    /* Zero-initialise the summary struct */
    summary->pass = summary->fail = summary->skip = summary->other = 0;
    summary->total = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        summary->total++;   /* Count every line                              */

        /* strstr returns non-NULL if the substring is found                 */
        if (strstr(line, "PASS") != NULL) {
            summary->pass++;
        } else if (strstr(line, "FAIL") != NULL) {
            summary->fail++;
        } else if (strstr(line, "SKIP") != NULL) {
            summary->skip++;
        } else {
            summary->other++;   /* Comment lines, blank lines, headers, etc. */
        }
    }

    fclose(fp);   /* Close after reading — release file handle               */
    return 0;     /* 0 = success                                             */
}

/* Create a sample log file for testing */
void create_sample_log(const char *filename)
{
    FILE *fp = fopen(filename, "w");   /* "w" = write, creates or overwrites */
    if (fp == NULL) {
        perror("Cannot create sample log");
        return;
    }

    fprintf(fp, "# RISC-V Decoder Test Log\n");   /* Comment line           */
    fprintf(fp, "# Generated by test runner\n\n"); /* Comment line           */
    fprintf(fp, "[PASS] ADD x1, x2, x3 → 0x003100B3\n");
    fprintf(fp, "[PASS] SUB x2, x2, x3 → 0x40310133\n");
    fprintf(fp, "[FAIL] ADDI x2, x0, 5 → mismatch: expected 'addi x2, x0, 5' got 'addi x2, x0, 0'\n");
    fprintf(fp, "[PASS] LW x2, 0(x1)   → 0x0000A103\n");
    fprintf(fp, "[SKIP] FENCE — not implemented in this version\n");
    fprintf(fp, "[PASS] BNE x1, x2, -8 → 0xFE209CE3\n");
    fprintf(fp, "[FAIL] JAL x1, 4 — wrong immediate sign extension\n");
    fprintf(fp, "[PASS] LUI x1, 1 → 0x000010B7\n");

    fclose(fp);
    printf("Created sample log: '%s'\n", filename);
}

int main(int argc, char *argv[])
{
    const char *logfile;

    if (argc < 2) {
        /* No argument — create and use a sample log file for demonstration  */
        create_sample_log("sample_test.log");
        logfile = "sample_test.log";
    } else {
        logfile = argv[1];   /* Use the provided log file                    */
    }

    log_summary_t summary;   /* Declare the result struct                    */
    int result = parse_log(logfile, &summary);

    if (result != 0) {
        fprintf(stderr, "Failed to parse log file. Exiting.\n");
        return EXIT_FAILURE;
    }

    /* Print the summary report */
    printf("\n=== Test Log Summary: '%s' ===\n", logfile);
    printf("Total lines:  %d\n",   summary.total);
    printf("PASS:         %d\n",   summary.pass);
    printf("FAIL:         %d\n",   summary.fail);
    printf("SKIP:         %d\n",   summary.skip);
    printf("Other:        %d\n\n", summary.other);

    /* Calculate pass rate (avoid division by zero) */
    int tested = summary.pass + summary.fail;
    if (tested > 0) {
        printf("Pass rate: %.1f%% (%d/%d)\n",
               100.0 * summary.pass / tested,
               summary.pass, tested);
    }

    /* Return non-zero if any tests failed (useful in Makefiles)             */
    return (summary.fail > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
```

---

### Exercise 4: Little-Endian Storage — Load and Verify `0xDEADBEEF`

**Task:** Load `0xDEADBEEF` via `load_hex_file`. Verify individual bytes: `EF`, `BE`, `AD`, `DE`.

```c
/* exercise4_day4.c — Verify little-endian byte storage of 0xDEADBEEF */

#include <stdio.h>    /* printf, fopen, fputs, fclose, fprintf               */
#include <stdlib.h>   /* calloc, free, strtoul                               */
#include <stdint.h>   /* uint8_t, uint32_t                                   */
#include <string.h>   /* memset                                               */

int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("load_hex_file"); return -1; }

    char line[32];
    uint32_t addr = 0;

    while (fgets(line, sizeof(line), fp) != NULL && (addr + 3) < mem_size) {
        if (line[0] == '#' || line[0] == '\n') continue;   /* Skip comments */
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);
        memory[addr + 0] = (uint8_t)((word >>  0) & 0xFF);   /* Byte 0: LSB */
        memory[addr + 1] = (uint8_t)((word >>  8) & 0xFF);   /* Byte 1      */
        memory[addr + 2] = (uint8_t)((word >> 16) & 0xFF);   /* Byte 2      */
        memory[addr + 3] = (uint8_t)((word >> 24) & 0xFF);   /* Byte 3: MSB */
        addr += 4;
    }
    fclose(fp);
    return (int)(addr / 4);
}

int main(void)
{
    /* Create a hex file containing exactly one instruction word: DEADBEEF   */
    const char *testfile = "/tmp/test_deadbeef.hex";
    FILE *fp = fopen(testfile, "w");
    if (fp == NULL) { perror("Cannot create test file"); return 1; }
    fputs("DEADBEEF\n", fp);   /* Write one line: DEADBEEF (no 0x prefix)   */
    fclose(fp);

    /* Allocate 4 bytes of memory to hold our one word                       */
    uint8_t *mem = calloc(4, sizeof(uint8_t));
    if (mem == NULL) { fprintf(stderr, "calloc failed\n"); return 1; }

    /* Load the hex file */
    int words = load_hex_file(testfile, mem, 4);
    printf("Loaded %d word(s)\n", words);

    /* Verify little-endian storage.
       0xDEADBEEF in little-endian:
         mem[0] = 0xEF  (bits [7:0]   — least significant byte first)
         mem[1] = 0xBE  (bits [15:8])
         mem[2] = 0xAD  (bits [23:16])
         mem[3] = 0xDE  (bits [31:24] — most significant byte last)          */
    printf("\nLittle-endian storage of 0xDEADBEEF:\n");
    printf("  mem[0] = 0x%02X (expected 0xEF — %s)\n",
           mem[0], mem[0] == 0xEF ? "PASS" : "FAIL");
    printf("  mem[1] = 0x%02X (expected 0xBE — %s)\n",
           mem[1], mem[1] == 0xBE ? "PASS" : "FAIL");
    printf("  mem[2] = 0x%02X (expected 0xAD — %s)\n",
           mem[2], mem[2] == 0xAD ? "PASS" : "FAIL");
    printf("  mem[3] = 0x%02X (expected 0xDE — %s)\n",
           mem[3], mem[3] == 0xDE ? "PASS" : "FAIL");

    /* Reconstruct the original word from bytes to double-check              */
    uint32_t reconstructed =
        ((uint32_t)mem[3] << 24) |   /* MSB goes to bits [31:24]            */
        ((uint32_t)mem[2] << 16) |   /* Byte 2 goes to bits [23:16]         */
        ((uint32_t)mem[1] <<  8) |   /* Byte 1 goes to bits [15:8]          */
        ((uint32_t)mem[0] <<  0);    /* LSB goes to bits [7:0]              */

    printf("\nReconstructed: 0x%08X (expected 0xDEADBEEF — %s)\n",
           reconstructed,
           reconstructed == 0xDEADBEEF ? "PASS" : "FAIL");

    free(mem);
    mem = NULL;

    printf("\nValgrind should show: 0 errors, no leaks.\n");
    return 0;
}
```

---

### Exercise 6 (Bonus): Full Argument Parsing

**Task:** Parse `--mem-size`, `--start-addr`, and `--trace` flags.

```c
/* exercise6_day4.c — Full command-line flag parsing */

#include <stdio.h>    /* printf, fprintf                                      */
#include <stdlib.h>   /* atoi, EXIT_FAILURE, EXIT_SUCCESS                    */
#include <string.h>   /* strcmp                                               */
#include <stdint.h>   /* uint32_t                                             */

/* Configuration struct — one place for all program settings                  */
typedef struct {
    const char *hex_file;     /* Required: path to hex file                  */
    int         mem_size;     /* Optional: memory size in bytes (default 64K)*/
    uint32_t    start_addr;   /* Optional: starting PC address (default 0)   */
    int         trace;        /* Optional: print each instruction (0/1)      */
    int         verbose;      /* Optional: print extra info (0/1)            */
} config_t;

/* print_usage — explains how to use the program                              */
void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <hexfile> [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --mem-size  <bytes>    Memory size (default: 65536)\n");
    fprintf(stderr, "  --start-addr <hex>     Start address in hex (default: 0)\n");
    fprintf(stderr, "  --trace                Print each instruction as executed\n");
    fprintf(stderr, "  --verbose, -v          Print extra information\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "  %s program.hex --mem-size 4096 --start-addr 0x1000 --trace\n", prog);
}

/* parse_args — fill in 'cfg' from argc/argv.
   Returns 0 on success, -1 on error.                                         */
int parse_args(int argc, char *argv[], config_t *cfg)
{
    /* Set defaults */
    cfg->hex_file   = NULL;    /* Must be provided — no default               */
    cfg->mem_size   = 65536;   /* 64 KB default                              */
    cfg->start_addr = 0;       /* Start at address 0 by default              */
    cfg->trace      = 0;       /* Tracing off by default                     */
    cfg->verbose    = 0;       /* Verbose off by default                     */

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    /* argv[1] is the hex file — required positional argument                 */
    cfg->hex_file = argv[1];

    /* Parse optional flags from argv[2] onward */
    int i;
    for (i = 2; i < argc; i++) {

        if (strcmp(argv[i], "--mem-size") == 0) {
            if (i + 1 >= argc) {   /* Check there IS a next argument         */
                fprintf(stderr, "Error: --mem-size requires a value\n");
                return -1;
            }
            cfg->mem_size = atoi(argv[i + 1]);   /* Convert "4096" → 4096    */
            if (cfg->mem_size <= 0) {
                fprintf(stderr, "Error: mem-size must be positive\n");
                return -1;
            }
            i++;   /* Consume the value argument — skip it in next iteration  */

        } else if (strcmp(argv[i], "--start-addr") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --start-addr requires a value\n");
                return -1;
            }
            /* strtoul with base 0 auto-detects: 0x prefix → hex, else decimal */
            cfg->start_addr = (uint32_t)strtoul(argv[i + 1], NULL, 0);
            i++;   /* Consume value argument                                  */

        } else if (strcmp(argv[i], "--trace") == 0) {
            cfg->trace = 1;   /* Flag: no value needed, presence = enabled   */

        } else if (strcmp(argv[i], "--verbose") == 0 ||
                   strcmp(argv[i], "-v") == 0) {
            cfg->verbose = 1;

        } else {
            fprintf(stderr, "Warning: unknown option '%s' (ignored)\n", argv[i]);
        }
    }

    return 0;   /* Success                                                    */
}

int main(int argc, char *argv[])
{
    config_t cfg;   /* All settings in one struct                             */

    if (parse_args(argc, argv, &cfg) != 0) {
        return EXIT_FAILURE;   /* parse_args already printed the error        */
    }

    /* Print what was parsed */
    printf("=== Configuration ===\n");
    printf("Hex file:    %s\n",    cfg.hex_file);
    printf("Memory size: %d bytes (%d KB)\n", cfg.mem_size, cfg.mem_size / 1024);
    printf("Start addr:  0x%08X\n", cfg.start_addr);
    printf("Trace:       %s\n",    cfg.trace   ? "ON" : "OFF");
    printf("Verbose:     %s\n",    cfg.verbose ? "ON" : "OFF");

    /* Here you would: allocate memory, load hex file, run simulator          */
    printf("\n(Simulation would start here with the above settings)\n");

    return EXIT_SUCCESS;
}
```

---

## Summary — What You Learned on Day 4

| Concept          | Key Point                                                               |
|------------------|-------------------------------------------------------------------------|
| `malloc`         | Allocate raw (garbage) heap memory; always check for NULL              |
| `calloc`         | Allocate zero-initialised heap memory; preferred for arrays            |
| `realloc`        | Resize an allocation; use a temp pointer, never lose the original      |
| `free`           | Return memory; always call exactly once; set pointer to NULL after     |
| Memory Leak      | `malloc` without `free` — Valgrind: "definitely lost"                 |
| Dangling Pointer | Using memory after `free` — Valgrind: "Invalid read/write"            |
| Double Free      | Calling `free` twice — Valgrind: "Invalid free()"                     |
| Buffer Overflow  | Writing past allocation — Valgrind: "Invalid write of size N"         |
| `fopen/fclose`   | Always check for NULL; always close                                    |
| `fgets`          | Read one line safely with a size limit                                 |
| `fprintf`        | Print formatted text to a FILE* (or stderr)                           |
| `strtoul`        | Convert hex string to integer (better than `atoi` for hex)            |
| Little-endian    | LSB at lowest address — RISC-V and x86 convention                     |
| GDB              | `-g -O0` to compile; `break/run/next/step/print` to debug             |
| Valgrind         | `--leak-check=full` to find all four memory sins                      |

---

*Day 4 Complete — MEDS Module 2 | UET Lahore*
