#include "processor.h"
#include "memory.h"
#include <cstring>
#include <iostream>
#include <cstdint>

int32_t Register[NUM_REGS];
int PC = 0;
int opcode = 0, dest = 0, src1 = 0, src2 = 0;
int end_of_simulation = 0;
int Z = 0, N = 0, C = 0, V = 0;

static void updateFlags(int32_t result, int32_t a, int32_t b, bool isAdd) {
    Z = (result == 0) ? 1 : 0;
    N = (result < 0) ? 1 : 0;          // MSB
    if (isAdd) {
        // C: unsigned overflow (result < either unsigned input)
        uint32_t ua = static_cast<uint32_t>(a);
        uint32_t ub = static_cast<uint32_t>(b);
        uint32_t ur = static_cast<uint32_t>(result);
        C = (ur < ua || ur < ub) ? 1 : 0;
        // V: signed overflow
        V = ((a > 0 && b > 0 && result < 0) || (a < 0 && b < 0 && result > 0)) ? 1 : 0;
    } else {
        // SUB: C if a >= b (unsigned)
        C = (static_cast<uint32_t>(a) >= static_cast<uint32_t>(b)) ? 1 : 0;
        // V for sub
        V = ((a >= 0 && b < 0 && result < 0) || (a < 0 && b >= 0 && result > 0)) ? 1 : 0;
    }
}

void reset() {
    std::memset(Register, 0, sizeof(Register));
    PC = 0;
    end_of_simulation = 0;
    opcode = dest = src1 = src2 = 0;
    Z = N = C = V = 0;
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

void decode() {}

void execute() {
    if (opcode == 0) {
        end_of_simulation = 1;
        return;
    }

    // Helper to read 32-bit from data memory (little-endian)
    auto load32 = [](int addr) -> int32_t {
        if (addr < 0 || addr + 3 >= DATA_MEM_SIZE) return 0;
        int32_t v = 0;
        v |= Data[addr];
        v |= Data[addr+1] << 8;
        v |= Data[addr+2] << 16;
        v |= Data[addr+3] << 24;
        return v;
    };
    auto store32 = [](int addr, int32_t val) {
        if (addr < 0 || addr + 3 >= DATA_MEM_SIZE) return;
        Data[addr]   = val & 0xFF;
        Data[addr+1] = (val >> 8) & 0xFF;
        Data[addr+2] = (val >> 16) & 0xFF;
        Data[addr+3] = (val >> 24) & 0xFF;
    };

    switch (opcode) {
        // Integer register-register
        case 0x01: { // ADD
            int32_t a = Register[src1], b = Register[src2];
            int32_t r = a + b;
            Register[dest] = r;
            updateFlags(r, a, b, true);
            break;
        }
        case 0x02: { // SUB
            int32_t a = Register[src1], b = Register[src2];
            int32_t r = a - b;
            Register[dest] = r;
            updateFlags(r, a, b, false);
            break;
        }
        case 0x03: // MUL
            Register[dest] = Register[src1] * Register[src2];
            break;
        case 0x04: // DIV
            Register[dest] = (Register[src2] != 0) ? Register[src1] / Register[src2] : 0;
            break;

        // Integer register-immediate
        case 0x09: { // ADD imm
            int32_t a = Register[src1], b = src2;
            int32_t r = a + b;
            Register[dest] = r;
            updateFlags(r, a, b, true);
            break;
        }
        case 0x0A: { // SUB imm
            int32_t a = Register[src1], b = src2;
            int32_t r = a - b;
            Register[dest] = r;
            updateFlags(r, a, b, false);
            break;
        }
        case 0x0B: // MUL imm
            Register[dest] = Register[src1] * src2;
            break;
        case 0x0C: // DIV imm  (also used for some mem in table, but we keep clear)
            Register[dest] = (src2 != 0) ? Register[src1] / src2 : 0;
            break;

        // Memory
        case 0x05: // integer load  dest = [src1]   (src1 is register holding address)
            Register[dest] = load32(Register[src1]);
            break;
        case 0x0D: // load from constant address (we use 0x0D to avoid conflict)
            Register[dest] = load32(src1);
            break;
        case 0x06: // store  [src1] = dest
            store32(Register[src1], Register[dest]);
            break;
        case 0x0E: // store to constant address
            store32(src1, Register[dest]);
            break;

        // Data movement
        case 0x07: // MOV reg = reg (rare)
            Register[dest] = Register[src1];
            break;
        case 0x0F: // MOV reg = imm
            Register[dest] = src1;   // according to Lab2 table operand1 is the constant
            break;

        // Branches: opcode = 0x10 + condition code
        // Relative offset is in src2 (signed 8-bit for simplicity, but we store as uint8)
        case 0x10: // BAL (always) – we use code 0 for AL
        case 0x1E: // some tables use different; we map common ones
        {
            // offset is signed
            int8_t off = static_cast<int8_t>(src2);
            PC += off * 4;   // because each instr is 4 bytes, offset is in instructions
            break;
        }
        // More precise branch handling below
        default:
            if (opcode >= 0x10 && opcode <= 0x1F) {
                int cond = opcode - 0x10;
                bool take = false;
                // Simple ARM-like condition codes (subset)
                // 0 = EQ, 1 = NE, 2 = CS/HS, 3 = CC/LO, 4 = MI, 5 = PL,
                // 6 = VS, 7 = VC, 8 = HI, 9 = LS, A = GE, B = LT, C = GT, D = LE, E = AL
                switch (cond) {
                    case 0x0: take = (Z == 1); break;               // EQ
                    case 0x1: take = (Z == 0); break;               // NE
                    case 0x2: take = (C == 1); break;               // CS
                    case 0x3: take = (C == 0); break;               // CC
                    case 0x4: take = (N == 1); break;               // MI
                    case 0x5: take = (N == 0); break;               // PL
                    case 0x6: take = (V == 1); break;               // VS
                    case 0x7: take = (V == 0); break;               // VC
                    case 0xA: take = (N == V); break;               // GE
                    case 0xB: take = (N != V); break;               // LT
                    case 0xC: take = (Z == 0 && N == V); break;     // GT
                    case 0xD: take = (Z == 1 || N != V); break;     // LE
                    case 0xE: take = true; break;                   // AL
                    default: take = false; break;
                }
                if (take) {
                    int8_t off = static_cast<int8_t>(src2);
                    PC += off * 4;
                }
            } else {
                std::cerr << "Unknown opcode 0x" << std::hex << opcode << " at PC=" << (PC-4) << "\n";
                end_of_simulation = 1;
            }
            break;
    }
}
