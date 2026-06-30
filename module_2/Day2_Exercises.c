/* ==========================================================================
 * Day2_Exercises.c
 * MEDS Module 2 - C Language for Hardware Engineers
 * Ali Hassan | 2024-EE-176 | UET Lahore
 * 
 * Menu-driven program containing all Day 2 exercise solutions.
 * Run the program, enter an exercise number (1-6) to execute it, or 0 to
 * exit.
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -o day2 Day2_Exercises.c
 * Run:
 *   ./day2
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

int main(void); /* forward declaration so exercise1 can take &main's address */

/* ==========================================================================
 * Exercise 1: Memory Layout Examination
 * ========================================================================== */
int global_initialized = 0xDEADBEEF;   /* DATA segment */
int global_uninitialized;              /* BSS segment */

void exercise1(void)
{
    printf("\n--- Exercise 1: Memory Layout Examination ---\n\n");

    int local_var = 42;                          /* STACK */
    static int static_var = 0x12345678;           /* DATA segment */
    int *heap_var = malloc(sizeof(int));          /* HEAP */
    if (heap_var) *heap_var = 0xCAFEBABE;

    printf("MEMORY LAYOUT EXAMINATION:\n");
    printf("==========================\n\n");

    printf("TEXT Segment (Code):\n");
    printf("  main function:          %p\n", (void *)&main);
    printf("\nDATA Segment (Initialized globals):\n");
    printf("  global_initialized:     %p (value: 0x%08X)\n",
           (void *)&global_initialized, global_initialized);
    printf("  static_var:             %p (value: 0x%08X)\n",
           (void *)&static_var, static_var);

    printf("\nBSS Segment (Uninitialized globals):\n");
    printf("  global_uninitialized:   %p (value: %d)\n",
           (void *)&global_uninitialized, global_uninitialized);

    printf("\nSTACK Segment (Local variables):\n");
    printf("  local_var:              %p (value: %d)\n",
           (void *)&local_var, local_var);

    printf("\nHEAP Segment (Dynamically allocated):\n");
    printf("  heap_var (pointer):     %p\n", (void *)heap_var);
    if (heap_var) {
        printf("  heap_var (address):     %p (value: 0x%08X)\n",
               (void *)heap_var, *heap_var);
    }

    printf("\nMemory order checks:\n");
    if ((uintptr_t)&main < (uintptr_t)&global_initialized)
        printf("  TEXT < DATA (typical)\n");
    if ((uintptr_t)&global_initialized < (uintptr_t)&global_uninitialized)
        printf("  DATA < BSS (typical)\n");
    if (heap_var && (uintptr_t)heap_var > (uintptr_t)&local_var)
        printf("  STACK > HEAP (typical)\n");

    free(heap_var);
}

/* ==========================================================================
 * Exercise 2: Register File with Pointer Functions
 * ========================================================================== */
void write_reg(uint32_t *regs, uint8_t rd, uint32_t value)
{
    if (rd == 0) return;        /* x0 always 0 */
    if (rd < 32) regs[rd] = value;
}

uint32_t read_reg(const uint32_t *regs, uint8_t rs)
{
    if (rs < 32) return regs[rs];
    return 0;
}

void print_registers(const uint32_t *regs)
{
    for (int i = 0; i < 32; i++) {
        printf("x%2d = 0x%08X", i, regs[i]);
        if ((i + 1) % 4 == 0) {
            printf("\n");
        } else {
            printf("  |  ");
        }
    }
}

void exercise2(void)
{
    printf("\n--- Exercise 2: Register File with Pointer Functions ---\n\n");

    uint32_t registers[32] = {0};

    printf("Initial state (all zeros):\n");
    print_registers(registers);
    printf("\n");

    write_reg(registers, 0, 0xDEADBEEF);   /* ignored */
    write_reg(registers, 1, 0x11111111);
    write_reg(registers, 2, 0x22222222);
    write_reg(registers, 10, 0xCAFEBABE);
    write_reg(registers, 31, 0xFFFFFFFF);

    printf("After writes:\n");
    print_registers(registers);
    printf("\n");

    printf("Read tests:\n");
    printf("  x0 = 0x%08X (should be 0)\n", read_reg(registers, 0));
    printf("  x1 = 0x%08X\n", read_reg(registers, 1));
    printf("  x2 = 0x%08X\n", read_reg(registers, 2));
    printf("  x10 = 0x%08X\n", read_reg(registers, 10));
    printf("  x31 = 0x%08X\n", read_reg(registers, 31));
}

/* ==========================================================================
 * Exercise 3: Memory Dump Function
 * ========================================================================== */
void memory_dump(const uint8_t *mem, size_t size)
{
    printf("Address  | Hex Data                         | ASCII\n");
    printf("---------+----------------------------------+------------------\n");

    for (size_t i = 0; i < size; i += 16) {
        printf("0x%06zX | ", i);

        for (int j = 0; j < 16; j++) {
            if (i + (size_t)j < size) {
                printf("%02X ", mem[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" ");
        }

        printf("| ");

        for (int j = 0; j < 16 && i + (size_t)j < size; j++) {
            char c = (char)mem[i + j];
            printf("%c", isprint((unsigned char)c) ? c : '.');
        }
        printf("\n");
    }
}

void exercise3(void)
{
    printf("\n--- Exercise 3: Memory Dump Function ---\n\n");

    uint8_t test_memory[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        'H',  'e',  'l',  'l',  'o',  ',',  ' ',  'W',
        'o',  'r',  'l',  'd',  '!',  0x00, 0x00, 0x00,
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
    };

    printf("Memory dump of test data:\n\n");
    memory_dump(test_memory, sizeof(test_memory));
}

/* ==========================================================================
 * Exercise 4: In-Place Array Reversal with Pointers
 * ========================================================================== */
void reverse_array(uint32_t *arr, size_t size)
{
    uint32_t *left = arr;
    uint32_t *right = arr + size - 1;

    while (left < right) {
        uint32_t temp = *left;
        *left = *right;
        *right = temp;
        left++;
        right--;
    }
}

void print_array(const uint32_t *arr, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        printf("%u ", *(arr + i));
    }
    printf("\n");
}

void exercise4(void)
{
    printf("\n--- Exercise 4: In-Place Array Reversal with Pointers ---\n\n");

    uint32_t values[] = {1, 2, 3, 4, 5};
    size_t count = sizeof(values) / sizeof(values[0]);

    printf("Original array:\n");
    print_array(values, count);

    printf("After reversal:\n");
    reverse_array(values, count);
    print_array(values, count);
}

/* ==========================================================================
 * Exercise 5: Safe String Concatenation
 * ========================================================================== */
int strcat_safe(char *dest, size_t dest_size, const char *src)
{
    if (!dest || !src || dest_size == 0) {
        return -1;
    }

    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len + src_len >= dest_size) {
        printf("Error: Buffer too small (need %zu bytes, have %zu)\n",
               dest_len + src_len + 1, dest_size);
        return -1;
    }

    strncpy(&dest[dest_len], src, dest_size - dest_len - 1);
    dest[dest_size - 1] = '\0';

    return 0;
}

void exercise5(void)
{
    printf("\n--- Exercise 5: Safe String Concatenation ---\n\n");

    char buffer[32] = {0};

    printf("Test 1: Safe concatenation\n");
    strcat_safe(buffer, sizeof(buffer), "Hello");
    printf("After \"Hello\": \"%s\"\n", buffer);

    strcat_safe(buffer, sizeof(buffer), " ");
    strcat_safe(buffer, sizeof(buffer), "World");
    printf("After \" World\": \"%s\"\n\n", buffer);

    printf("Test 2: Buffer overflow attempt\n");
    char small_buffer[10] = "Start";
    if (strcat_safe(small_buffer, sizeof(small_buffer),
                    " this is a very long string that wont fit") == -1) {
        printf("Concatenation prevented (buffer protected)\n");
    }
    printf("Buffer still contains: \"%s\"\n", small_buffer);
}

/* ==========================================================================
 * Exercise 6 (Bonus): Memory Simulation with Load/Store
 * ========================================================================== */
#define MEMORY_SIZE 256

static inline int is_aligned(uint32_t addr)
{
    return (addr & 0x3) == 0;
}

int load_word(const uint8_t *mem, uint32_t addr, uint32_t *value)
{
    if (addr + 4 > MEMORY_SIZE) {
        printf("Error: Address 0x%08X out of bounds\n", addr);
        return -1;
    }
    if (!is_aligned(addr)) {
        printf("Error: Address 0x%08X not aligned (must be 4-byte aligned)\n", addr);
        return -1;
    }
    *value = mem[addr + 0] |
             (mem[addr + 1] << 8) |
             (mem[addr + 2] << 16) |
             (mem[addr + 3] << 24);
    return 0;
}

int store_word(uint8_t *mem, uint32_t addr, uint32_t value)
{
    if (addr + 4 > MEMORY_SIZE) {
        printf("Error: Address 0x%08X out of bounds\n", addr);
        return -1;
    }
    if (!is_aligned(addr)) {
        printf("Error: Address 0x%08X not aligned\n", addr);
        return -1;
    }
    mem[addr + 0] = (uint8_t)(value & 0x000000FF);
    mem[addr + 1] = (uint8_t)((value & 0x0000FF00) >> 8);
    mem[addr + 2] = (uint8_t)((value & 0x00FF0000) >> 16);
    mem[addr + 3] = (uint8_t)((value & 0xFF000000) >> 24);
    return 0;
}

void exercise6(void)
{
    printf("\n--- Exercise 6 (Bonus): Memory Simulation with Load/Store ---\n\n");

    uint8_t memory[MEMORY_SIZE] = {0};
    uint32_t value;

    printf("Simulated 256-byte Memory Test\n");
    printf("================================\n\n");

    printf("Test 1: Aligned 4-byte operations\n");
    if (store_word(memory, 0x00, 0xDEADBEEF) == 0) {
        printf("Stored 0xDEADBEEF at address 0x00\n");
    }
    if (load_word(memory, 0x00, &value) == 0) {
        printf("Loaded value: 0x%08X\n\n", value);
    }

    printf("Test 2: Multiple stores\n");
    store_word(memory, 0x04, 0xCAFEBABE);
    store_word(memory, 0x08, 0x12345678);
    store_word(memory, 0x0C, 0xFFFFFFFF);
    printf("Stored values at offsets 0x04, 0x08, 0x0C\n\n");

    printf("Test 3: Load back\n");
    for (uint32_t addr = 0x00; addr < 0x10; addr += 4) {
        if (load_word(memory, addr, &value) == 0) {
            printf("  Memory[0x%02X] = 0x%08X\n", addr, value);
        }
    }
    printf("\n");

    printf("Test 4: Unaligned address (should fail)\n");
    if (load_word(memory, 0x01, &value) == -1) {
        printf("Correctly rejected unaligned address\n\n");
    }

    printf("Test 5: Out of bounds access (should fail)\n");
    if (store_word(memory, 0xFE, 0x11223344) == -1) {
        printf("Correctly rejected out-of-bounds access\n");
    }
}

/* ==========================================================================
 * Menu / Driver
 * ========================================================================== */
void print_menu(void)
{
    printf("\n=====================================================\n");
    printf(" Day 2 Exercises - MEDS Module 2 (C for Hardware Eng.)\n");
    printf("=====================================================\n");
    printf(" 1. Memory Layout Examination\n");
    printf(" 2. Register File with Pointer Functions\n");
    printf(" 3. Memory Dump Function\n");
    printf(" 4. In-Place Array Reversal with Pointers\n");
    printf(" 5. Safe String Concatenation\n");
    printf(" 6. (Bonus) Memory Simulation with Load/Store\n");
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
            case 5: exercise5(); break;
            case 6: exercise6(); break;
            case 0: printf("\nExiting. Goodbye!\n"); break;
            default: printf("\nInvalid choice. Please enter 0-6.\n"); break;
        }
    } while (choice != 0);

    return 0;
}
