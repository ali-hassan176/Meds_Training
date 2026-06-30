#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
int main(){
    uint32_t instruction = 0x00C58533;
    printf("Hello Everyone\n");
    printf("Welcome to C Programming\n");
    printf("This is not 1st C program\n");
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x07;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    return 0;
}
