#include "processor.h"
#include "memory.h"
#include <cstring>
#include <iostream>

int32_t Register[NUM_REGS];
int PC = 0;
int opcode = 0, dest = 0, src1 = 0, src2 = 0;
int end_of_simulation = 0;

void reset() {
    std::memset(Register, 0, sizeof(Register));
    PC = 0;
    end_of_simulation = 0;
    opcode = dest = src1 = src2 = 0;
}

void fetch() {
    if (PC + 3 >= INSTR_MEM_SIZE) {
        end_of_simulation = 1;
        return;
    }
    opcode = Instruction[PC];
    dest   = Instruction[PC + 1];
    src1   = Instruction[PC + 2];
    src2   = Instruction[PC + 3];
    PC += 4;
}

void decode() {
    // empty for Lab 1
}

void execute() {
    if (opcode == 0) {
        end_of_simulation = 1;
        return;
    }

    switch (opcode) {
        case 1: // ADD  dest = src1 + src2
            Register[dest] = Register[src1] + Register[src2];
            break;
        case 2: // SUB
            Register[dest] = Register[src1] - Register[src2];
            break;
        case 3: // MUL
            Register[dest] = Register[src1] * Register[src2];
            break;
        case 4: // DIV
            if (Register[src2] != 0)
                Register[dest] = Register[src1] / Register[src2];
            else
                Register[dest] = 0;
            break;
        case 5: // Memory Read  dest = Data[src1]  (src2==0)
            // address is src1 (constant in original Lab1, but we treat as register index for simplicity)
            // According to Lab1: Read x1, 0  → opcode 5, dest=1, src1=0, src2=0
            // We interpret src1 as the address.
            {
                int addr = src1;
                if (addr + 3 < DATA_MEM_SIZE) {
                    int32_t val = 0;
                    val |= Data[addr];
                    val |= Data[addr+1] << 8;
                    val |= Data[addr+2] << 16;
                    val |= Data[addr+3] << 24;
                    Register[dest] = val;
                }
            }
            break;
        case 6: // Memory Write  Data[src1] = Register[dest]  (src2==0)
            {
                int addr = src1;
                if (addr + 3 < DATA_MEM_SIZE) {
                    int32_t val = Register[dest];
                    Data[addr]   = val & 0xFF;
                    Data[addr+1] = (val >> 8) & 0xFF;
                    Data[addr+2] = (val >> 16) & 0xFF;
                    Data[addr+3] = (val >> 24) & 0xFF;
                }
            }
            break;
        case 7: // Data movement  dest = constant (src1)
            Register[dest] = src1;
            break;
        default:
            std::cerr << "Unknown opcode " << opcode << " at PC=" << (PC-4) << "\n";
            end_of_simulation = 1;
            break;
    }
}
