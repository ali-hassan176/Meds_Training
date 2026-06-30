/* ==========================================================================
 * Day3_Exercises.c
 * MEDS Module 2 - C Language for Hardware Engineers
 * Ali Hassan | 2024-EE-176 | UET Lahore
 *
 * Menu-driven program containing all Day 3 exercise solutions
 * (Structs, Unions, Enums).
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -o day3 Day3_Exercises.c
 * Run:
 *   ./day3
 * ==========================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define EXTRACT_BITS(val, hi, lo) \
    (((uint32_t)(val) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))

/* ==========================================================================
 * Exercise 1: decode_r_type Function
 * ========================================================================== */
typedef struct {
    uint32_t opcode;
    uint32_t rd;
    uint32_t funct3;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct7;
    int32_t  imm;
} decoded_instr_t;

void decode_r_type(uint32_t raw, decoded_instr_t *out)
{
    out->opcode = EXTRACT_BITS(raw, 6,  0);
    out->rd     = EXTRACT_BITS(raw, 11, 7);
    out->funct3 = EXTRACT_BITS(raw, 14, 12);
    out->rs1    = EXTRACT_BITS(raw, 19, 15);
    out->rs2    = EXTRACT_BITS(raw, 24, 20);
    out->funct7 = EXTRACT_BITS(raw, 31, 25);
    out->imm    = 0;
}

void exercise1(void)
{
    printf("\n--- Exercise 1: decode_r_type Function ---\n\n");

    decoded_instr_t instr;
    decode_r_type(0x00A28233, &instr);   /* ADD x4, x5, x10 */

    printf("Raw instruction: 0x%08X\n", 0x00A28233);
    printf("Opcode:  0x%02X (should be 0x33 = R-type)\n", instr.opcode);
    printf("rd:      x%u   (should be x4)\n", instr.rd);
    printf("funct3:  %u    (should be 0 = ADD/SUB)\n", instr.funct3);
    printf("rs1:     x%u   (should be x5)\n", instr.rs1);
    printf("rs2:     x%u  (should be x10)\n", instr.rs2);
    printf("funct7:  0x%02X (should be 0x00 = ADD not SUB)\n", instr.funct7);

    if (instr.funct3 == 0 && instr.funct7 == 0x00) {
        printf("Instruction: ADD x%u, x%u, x%u\n", instr.rd, instr.rs1, instr.rs2);
    } else if (instr.funct3 == 0 && instr.funct7 == 0x20) {
        printf("Instruction: SUB x%u, x%u, x%u\n", instr.rd, instr.rs1, instr.rs2);
    }
}

/* ==========================================================================
 * Exercise 2: opcode_t Enum and opcode_to_string
 * ========================================================================== */
typedef enum {
    OP_R_TYPE  = 0x33,
    OP_I_TYPE  = 0x13,
    OP_LOAD    = 0x03,
    OP_STORE   = 0x23,
    OP_BRANCH  = 0x63,
    OP_JAL     = 0x6F,
    OP_JALR    = 0x67,
    OP_LUI     = 0x37,
    OP_AUIPC   = 0x17,
    OP_SYSTEM  = 0x73
} opcode_t;

const char *opcode_to_string(opcode_t op)
{
    switch (op) {
        case OP_R_TYPE:  return "R-TYPE (ADD/SUB/AND/OR/XOR/SLL/SRL/SRA/SLT/SLTU)";
        case OP_I_TYPE:  return "I-TYPE (ADDI/SLTI/ANDI/ORI/XORI/SLLI/SRLI/SRAI)";
        case OP_LOAD:    return "LOAD   (LB/LH/LW/LBU/LHU)";
        case OP_STORE:   return "STORE  (SB/SH/SW)";
        case OP_BRANCH:  return "BRANCH (BEQ/BNE/BLT/BGE/BLTU/BGEU)";
        case OP_JAL:     return "JAL    (Jump And Link)";
        case OP_JALR:    return "JALR   (Jump And Link Register)";
        case OP_LUI:     return "LUI    (Load Upper Immediate)";
        case OP_AUIPC:   return "AUIPC  (Add Upper Immediate to PC)";
        case OP_SYSTEM:  return "SYSTEM (ECALL/EBREAK/CSR)";
        default:         return "UNKNOWN opcode";
    }
}

void exercise2(void)
{
    printf("\n--- Exercise 2: opcode_t Enum and opcode_to_string ---\n\n");

    uint32_t tests[] = {
        0x003100B3, 0x00500113, 0x0000A103, 0x0020A023,
        0x00108063, 0x004000EF, 0x000010B7
    };

    int n = (int)(sizeof(tests) / sizeof(tests[0]));

    printf("Opcode decoding test:\n");
    printf("%-12s  %-6s  %s\n", "Instruction", "Opcode", "Family");
    printf("%-12s  %-6s  %s\n", "-----------", "------", "------");

    for (int i = 0; i < n; i++) {
        uint32_t opcode_bits = tests[i] & 0x7F;
        opcode_t op = (opcode_t)opcode_bits;
        printf("0x%08X    0x%02X    %s\n", tests[i], opcode_bits, opcode_to_string(op));
    }
}

/* ==========================================================================
 * Exercise 3: Full CPU State with Register Dump
 * ========================================================================== */
#define REG_COUNT 32

typedef struct {
    uint32_t x[REG_COUNT];
    uint32_t pc;
    uint8_t *memory;
    size_t   mem_size;
    uint64_t instr_count;
    uint64_t cycle_count;
} cpu_state_t;

static const char *abi_names[REG_COUNT] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void cpu_init(cpu_state_t *cpu, size_t mem_size)
{
    for (int i = 0; i < REG_COUNT; i++) {
        cpu->x[i] = 0;
    }
    cpu->pc          = 0x00000000;
    cpu->memory      = calloc(mem_size, 1);
    cpu->mem_size    = mem_size;
    cpu->instr_count = 0;
    cpu->cycle_count = 0;

    if (cpu->memory == NULL) {
        fprintf(stderr, "FATAL: failed to allocate %zu bytes of memory\n", mem_size);
    }
}

void reg_write_cpu(cpu_state_t *cpu, uint8_t rd, uint32_t value)
{
    if (rd == 0) return;
    if (rd >= REG_COUNT) {
        fprintf(stderr, "Error: register index %u out of range\n", rd);
        return;
    }
    cpu->x[rd] = value;
}

uint32_t reg_read_cpu(const cpu_state_t *cpu, uint8_t rs)
{
    if (rs >= REG_COUNT) {
        fprintf(stderr, "Error: register index %u out of range\n", rs);
        return 0;
    }
    return cpu->x[rs];
}

void dump_registers(const cpu_state_t *cpu)
{
    printf("\n=== CPU Register Dump ===\n");
    printf("%-4s  %-5s  %10s\n", "Reg", "ABI", "Value (hex)");
    printf("%-4s  %-5s  %10s\n", "---", "---", "-----------");

    for (int i = 0; i < REG_COUNT; i++) {
        printf("x%-3d  %-5s  0x%08X", i, abi_names[i], cpu->x[i]);
        if (cpu->x[i] != 0) {
            printf("  (%d)", (int32_t)cpu->x[i]);
        }
        printf("\n");
    }
    printf("PC:             0x%08X\n", cpu->pc);
    printf("instr_count:    %llu\n", (unsigned long long)cpu->instr_count);
}

void exercise3(void)
{
    printf("\n--- Exercise 3: Full CPU State with Register Dump ---\n");

    cpu_state_t cpu;
    cpu_init(&cpu, 4096);

    reg_write_cpu(&cpu, 1,  0x00400000);
    reg_write_cpu(&cpu, 2,  0x7FFFFF00);
    reg_write_cpu(&cpu, 10, 42);
    reg_write_cpu(&cpu, 0,  999);

    cpu.pc          = 0x00000008;
    cpu.instr_count = 3;

    printf("reg_read(x10) = %u (should be 42)\n", reg_read_cpu(&cpu, 10));
    printf("reg_read(x0)  = %u (should be 0)\n",  reg_read_cpu(&cpu, 0));

    dump_registers(&cpu);

    free(cpu.memory);
    cpu.memory = NULL;
}

/* ==========================================================================
 * Exercise 4: Struct Padding - Predict and Verify
 * ========================================================================== */
struct bad_order {
    uint8_t  a;
    uint32_t b;
    uint8_t  c;
    uint16_t d;
    uint8_t  e;
};

struct good_order {
    uint32_t b;
    uint16_t d;
    uint8_t  a;
    uint8_t  c;
    uint8_t  e;
};

struct uart_registers {
    uint32_t control;
    uint32_t status;
    uint8_t  tx_data;
    uint32_t rx_data;
};

void exercise4(void)
{
    printf("\n--- Exercise 4: Struct Padding - Predict and Verify ---\n\n");

    printf("struct bad_order:\n");
    printf("  sizeof = %zu bytes (we predicted 16)\n", sizeof(struct bad_order));
    printf("  Wasted padding: %zu bytes\n",
           sizeof(struct bad_order) - (1 + 4 + 1 + 2 + 1));

    printf("\nstruct good_order:\n");
    printf("  sizeof = %zu bytes (we predicted 12)\n", sizeof(struct good_order));
    printf("  Wasted padding: %zu bytes\n",
           sizeof(struct good_order) - (4 + 2 + 1 + 1 + 1));

    printf("\nMemory saved by reordering: %zu bytes per instance\n",
           sizeof(struct bad_order) - sizeof(struct good_order));

    printf("\nstruct uart_registers:\n");
    printf("  sizeof = %zu bytes\n", sizeof(struct uart_registers));

    struct uart_registers uart;
    printf("  offset of control: %zu\n", (size_t)((char*)&uart.control - (char*)&uart));
    printf("  offset of status:  %zu\n", (size_t)((char*)&uart.status  - (char*)&uart));
    printf("  offset of tx_data: %zu\n", (size_t)((char*)&uart.tx_data - (char*)&uart));
    printf("  offset of rx_data: %zu\n", (size_t)((char*)&uart.rx_data - (char*)&uart));
}

/* ==========================================================================
 * Exercise 5: Union - Raw and Bit-Field Views Together
 * ========================================================================== */
typedef union {
    uint32_t raw;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t funct7 : 7;
    } r;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t imm    : 12;
    } i;
} instruction_t;

void exercise5(void)
{
    printf("\n--- Exercise 5: Union - Raw and Bit-Field Views Together ---\n\n");

    instruction_t inst;

    inst.raw = 0x00A28233;   /* ADD x4, x5, x10 */
    printf("=== R-type decode: 0x00A28233 (ADD x4, x5, x10) ===\n");
    printf("opcode = 0x%02X (should be 0x33)\n", inst.r.opcode);
    printf("rd     = x%u  (should be x4)\n", inst.r.rd);
    printf("funct3 = %u   (should be 0)\n", inst.r.funct3);
    printf("rs1    = x%u  (should be x5)\n", inst.r.rs1);
    printf("rs2    = x%u (should be x10)\n", inst.r.rs2);
    printf("funct7 = 0x%02X (should be 0x00)\n", inst.r.funct7);

    inst.raw = 0x00500113;   /* ADDI x2, x0, 5 */
    printf("\n=== I-type decode: 0x00500113 (ADDI x2, x0, 5) ===\n");
    printf("opcode = 0x%02X (should be 0x13 = I-type arith)\n", inst.i.opcode);
    printf("rd     = x%u  (should be x2)\n", inst.i.rd);
    printf("funct3 = %u   (should be 0 = ADDI)\n", inst.i.funct3);
    printf("rs1    = x%u  (should be x0)\n", inst.i.rs1);
    printf("imm    = %u   (should be 5)\n", inst.i.imm);

    printf("\nSize of instruction_t union: %zu bytes (always 4)\n", sizeof(instruction_t));
}

/* ==========================================================================
 * Exercise 6 (Bonus): UART Peripheral Model
 * ========================================================================== */
#define UART_TX_READY   (1u << 0)
#define UART_RX_VALID   (1u << 1)
#define UART_ENABLE     (1u << 0)

typedef struct {
    uint32_t control;
    uint32_t status;
    uint8_t  tx_data;
    uint8_t  rx_data;
    uint32_t baud_rate;
} uart_t;

static uart_t simulated_uart;

void uart_init(uart_t *uart, uint32_t baud)
{
    uart->control   = UART_ENABLE;
    uart->status    = UART_TX_READY;
    uart->tx_data   = 0;
    uart->rx_data   = 0;
    uart->baud_rate = baud;
}

void uart_putchar(uart_t *uart, char c)
{
    if (!(uart->status & UART_TX_READY)) {
        fprintf(stderr, "UART: TX not ready, dropping character '%c'\n", c);
        return;
    }
    uart->tx_data  = (uint8_t)c;
    uart->status  &= ~UART_TX_READY;
    putchar(c);
    uart->status |= UART_TX_READY;
}

char uart_getchar(uart_t *uart)
{
    if (!(uart->status & UART_RX_VALID)) {
        return 0;
    }
    char c = (char)uart->rx_data;
    uart->status &= ~UART_RX_VALID;
    return c;
}

void uart_puts(uart_t *uart, const char *str)
{
    while (*str != '\0') {
        uart_putchar(uart, *str);
        str++;
    }
}

void exercise6(void)
{
    printf("\n--- Exercise 6 (Bonus): UART Peripheral Model ---\n\n");

    uart_init(&simulated_uart, 9600);

    printf("UART control register:  0x%08X\n", simulated_uart.control);
    printf("UART status register:   0x%08X\n", simulated_uart.status);
    printf("UART baud rate:         %u\n\n", simulated_uart.baud_rate);

    printf("Sending via UART: ");
    uart_puts(&simulated_uart, "Hello from RISC-V!\n");

    simulated_uart.rx_data  = 'A';
    simulated_uart.status  |= UART_RX_VALID;

    char received = uart_getchar(&simulated_uart);
    printf("Received via UART: '%c'\n", received);

    printf("\nSizeof uart_t: %zu bytes\n", sizeof(uart_t));
}

/* ==========================================================================
 * Menu / Driver
 * ========================================================================== */
void print_menu(void)
{
    printf("\n=====================================================\n");
    printf(" Day 3 Exercises - Structs, Unions, Enums\n");
    printf("=====================================================\n");
    printf(" 1. decode_r_type Function\n");
    printf(" 2. opcode_t Enum and opcode_to_string\n");
    printf(" 3. Full CPU State with Register Dump\n");
    printf(" 4. Struct Padding - Predict and Verify\n");
    printf(" 5. Union - Raw and Bit-Field Views Together\n");
    printf(" 6. (Bonus) UART Peripheral Model\n");
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
