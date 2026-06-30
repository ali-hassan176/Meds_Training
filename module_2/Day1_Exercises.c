/* ==========================================================================
 * Day1_Exercises.c
 * MEDS Module 2 - C Language for Hardware Engineers
 * Ali Hassan | 2024-EE-176 | UET Lahore
 *
 * Menu-driven program containing all Day 1 exercise solutions.
 * Run the program, enter an exercise number (1-6) to execute it, or 0 to
 * exit.
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -o day1 Day1_Exercises.c
 * Run:
 *   ./day1
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------
 * Shared helper: extract bits [high:low] from a 32-bit value
 * ------------------------------------------------------------------------ */
uint32_t extract_field(uint32_t instruction, int high, int low)
{
    int width = high - low + 1;
    return (instruction >> low) & ((1U << width) - 1);
}

/* ==========================================================================
 * Exercise 1: Hex to Binary, Decimal, Hex Converter
 * ========================================================================== */
void exercise1(void)
{
    char input[64];

    printf("\n--- Exercise 1: Hex to Binary/Decimal/Hex Converter ---\n");
    printf("Enter a 32-bit hex value (e.g. DEADBEEF): ");
    if (scanf("%63s", input) != 1) {
        printf("Invalid input.\n");
        return;
    }

    uint32_t value = (uint32_t)strtoul(input, NULL, 16);

    printf("Input (hex): %s\n", input);
    printf("Unsigned decimal: %u\n", value);
    printf("Signed decimal: %d\n", (int32_t)value);
    printf("Hex: 0x%08X\n", value);

    printf("Binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%u", (value >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n");
}

/* ==========================================================================
 * Exercise 2: Examine Compilation Stages
 * ========================================================================== */
void exercise2(void)
{
    printf("\n--- Exercise 2: Examine Compilation Stages ---\n");

    FILE *fp = fopen("simple.c", "w");
    if (fp == NULL) {
        perror("fopen simple.c");
        return;
    }
    fprintf(fp, "#include <stdio.h>\n\n");
    fprintf(fp, "int main(void) {\n");
    fprintf(fp, "    int x = 42;\n");
    fprintf(fp, "    printf(\"x = %%d\\n\", x);\n");
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);
    printf("Created simple.c\n\n");

    printf("Stage 1: Preprocessing (gcc -E simple.c -o simple.i)\n");
    system("gcc -E simple.c -o simple.i");

    printf("Stage 2: Compilation to assembly (gcc -S simple.c -o simple.s)\n");
    system("gcc -S simple.c -o simple.s");

    printf("Stage 3: Assembly to object file (gcc -c simple.c -o simple.o)\n");
    system("gcc -c simple.c -o simple.o");

    printf("Stage 4: Linking (gcc simple.o -o simple)\n");
    system("gcc simple.o -o simple");

    printf("\nComparing file sizes:\n");
    system("ls -lh simple.c simple.i simple.s simple.o simple 2>/dev/null");

    printf("\nObservations:\n");
    printf("  simple.i is much larger (includes all of stdio.h)\n");
    printf("  simple.s is human-readable assembly\n");
    printf("  simple.o is binary (platform-specific)\n");
    printf("  simple   is the final executable\n");
}

/* ==========================================================================
 * Exercise 3: Extract Instruction Fields
 * ========================================================================== */
void exercise3(void)
{
    printf("\n--- Exercise 3: Extract Instruction Fields ---\n");

    uint32_t instruction = 0x00A28233; /* add x4, x5, x10 */

    uint32_t opcode = extract_field(instruction, 6, 0);
    uint32_t rd     = extract_field(instruction, 11, 7);
    uint32_t funct3 = extract_field(instruction, 14, 12);
    uint32_t rs1    = extract_field(instruction, 19, 15);
    uint32_t rs2    = extract_field(instruction, 24, 20);
    uint32_t funct7 = extract_field(instruction, 31, 25);

    printf("Instruction: 0x%08X\n", instruction);
    printf("opcode [6:0]:   0x%02X\n", opcode);
    printf("rd [11:7]:      x%u\n", rd);
    printf("funct3 [14:12]: %u\n", funct3);
    printf("rs1 [19:15]:    x%u\n", rs1);
    printf("rs2 [24:20]:    x%u\n", rs2);
    printf("funct7 [31:25]: 0x%02X\n", funct7);
}

/* ==========================================================================
 * Exercise 4: RISC-V Instruction Decoder
 * ========================================================================== */
void decode_r_type_ex4(uint32_t instruction)
{
    uint32_t opcode = extract_field(instruction, 6, 0);
    uint32_t rd     = extract_field(instruction, 11, 7);
    uint32_t funct3 = extract_field(instruction, 14, 12);
    uint32_t rs1    = extract_field(instruction, 19, 15);
    uint32_t rs2    = extract_field(instruction, 24, 20);
    uint32_t funct7 = extract_field(instruction, 31, 25);

    printf("R-type Instruction: 0x%08X\n", instruction);
    printf("  opcode:  0x%02X\n", opcode);
    printf("  rd:      x%u\n", rd);
    printf("  funct3:  %u\n", funct3);
    printf("  rs1:     x%u\n", rs1);
    printf("  rs2:     x%u\n", rs2);
    printf("  funct7:  0x%02X\n", funct7);
    printf("\n");
}

void exercise4(void)
{
    printf("\n--- Exercise 4: RISC-V Instruction Decoder ---\n");

    int n;
    printf("How many hex instructions do you want to decode? ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Invalid count.\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        char buf[32];
        printf("Enter hex instruction #%d: ", i + 1);
        if (scanf("%31s", buf) != 1) {
            printf("Invalid input.\n");
            return;
        }
        uint32_t instruction = (uint32_t)strtoul(buf, NULL, 16);
        decode_r_type_ex4(instruction);
    }
}

/* ==========================================================================
 * Exercise 5: Sign Extension
 * ========================================================================== */
int32_t sign_extend(uint32_t val, int bit_width)
{
    uint32_t sign_bit = 1U << (bit_width - 1);
    return (int32_t)((val ^ sign_bit) - sign_bit);
}

void exercise5(void)
{
    printf("\n--- Exercise 5: Sign Extension ---\n\n");

    int32_t result = sign_extend(0xFFF, 12);
    printf("sign_extend(0xFFF, 12):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: -1 (0xFFFFFFFF)\n\n");

    result = sign_extend(0x7FF, 12);
    printf("sign_extend(0x7FF, 12):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: 2047 (0x000007FF)\n\n");

    result = sign_extend(0xFF, 8);
    printf("sign_extend(0xFF, 8):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: -1 (0xFFFFFFFF)\n\n");

    result = sign_extend(0x7F, 8);
    printf("sign_extend(0x7F, 8):\n");
    printf("  Result as int32: %d\n", result);
    printf("  Result as hex:   0x%08X\n", (uint32_t)result);
    printf("  Expected: 127 (0x0000007F)\n");
}

/* ==========================================================================
 * Exercise 6 (Bonus): Pack R-type Instruction
 * ========================================================================== */
uint32_t pack_r_type(uint32_t funct7, uint32_t rs2, uint32_t rs1,
                      uint32_t funct3, uint32_t rd, uint32_t opcode)
{
    uint32_t instruction = 0;
    instruction |= (opcode & 0x7F);
    instruction |= ((rd & 0x1F) << 7);
    instruction |= ((funct3 & 0x07) << 12);
    instruction |= ((rs1 & 0x1F) << 15);
    instruction |= ((rs2 & 0x1F) << 20);
    instruction |= ((funct7 & 0x7F) << 25);
    return instruction;
}

void exercise6(void)
{
    printf("\n--- Exercise 6 (Bonus): Pack R-type Instruction ---\n\n");

    uint32_t original = 0x00A28233; /* add x4, x5, x10 */

    printf("Original instruction: 0x%08X\n", original);
    printf("\nExtracting fields:\n");

    uint32_t opcode = extract_field(original, 6, 0);
    uint32_t rd     = extract_field(original, 11, 7);
    uint32_t funct3 = extract_field(original, 14, 12);
    uint32_t rs1    = extract_field(original, 19, 15);
    uint32_t rs2    = extract_field(original, 24, 20);
    uint32_t funct7 = extract_field(original, 31, 25);

    printf("  opcode:  0x%02X\n", opcode);
    printf("  rd:      x%u\n", rd);
    printf("  funct3:  %u\n", funct3);
    printf("  rs1:     x%u\n", rs1);
    printf("  rs2:     x%u\n", rs2);
    printf("  funct7:  0x%02X\n", funct7);

    printf("\nPacking back into instruction:\n");
    uint32_t packed = pack_r_type(funct7, rs2, rs1, funct3, rd, opcode);
    printf("Packed instruction: 0x%08X\n", packed);

    printf("\nVerification:\n");
    if (packed == original) {
        printf("SUCCESS: Packed instruction matches original!\n");
    } else {
        printf("FAILED: Packed instruction doesn't match\n");
    }
}

/* ==========================================================================
 * Menu / Driver
 * ========================================================================== */
void print_menu(void)
{
    printf("\n=====================================================\n");
    printf(" Day 1 Exercises - MEDS Module 2 (C for Hardware Eng.)\n");
    printf("=====================================================\n");
    printf(" 1. Hex to Binary/Decimal/Hex Converter\n");
    printf(" 2. Examine Compilation Stages\n");
    printf(" 3. Extract Instruction Fields\n");
    printf(" 4. RISC-V Instruction Decoder\n");
    printf(" 5. Sign Extension\n");
    printf(" 6. (Bonus) Pack R-type Instruction\n");
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
            /* Clear bad input from stdin */
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
