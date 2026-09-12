#include "memory.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>

uint8_t Instruction[INSTR_MEM_SIZE];
uint8_t Data[DATA_MEM_SIZE];

void initialize(const std::string& dataFile) {
    std::memset(Instruction, 0, sizeof(Instruction));
    std::memset(Data, 0, sizeof(Data));

    // Load program.byte
    std::ifstream prog("program.byte");
    if (!prog) {
        std::cerr << "Error: cannot open program.byte\n";
        return;
    }
    int addr = 0;
    unsigned int b0, b1, b2, b3;
    while (prog >> std::hex >> b0 >> b1 >> b2 >> b3 && addr + 3 < INSTR_MEM_SIZE) {
        Instruction[addr++] = static_cast<uint8_t>(b0);
        Instruction[addr++] = static_cast<uint8_t>(b1);
        Instruction[addr++] = static_cast<uint8_t>(b2);
        Instruction[addr++] = static_cast<uint8_t>(b3);
    }
    prog.close();

    // Load data.byte if present
    std::ifstream data(dataFile);
    if (data) {
        addr = 0;
        while (data >> std::hex >> b0 >> b1 >> b2 >> b3 && addr + 3 < DATA_MEM_SIZE) {
            Data[addr++] = static_cast<uint8_t>(b0);
            Data[addr++] = static_cast<uint8_t>(b1);
            Data[addr++] = static_cast<uint8_t>(b2);
            Data[addr++] = static_cast<uint8_t>(b3);
        }
        data.close();
    }
}

void finalize(const std::string& outFile) {
    std::ofstream out(outFile);
    if (!out) {
        std::cerr << "Error: cannot write " << outFile << "\n";
        return;
    }
    for (int i = 0; i < DATA_MEM_SIZE; i += 4) {
        out << std::hex << std::uppercase << std::setfill('0')
            << std::setw(2) << static_cast<int>(Data[i])   << " "
            << std::setw(2) << static_cast<int>(Data[i+1]) << " "
            << std::setw(2) << static_cast<int>(Data[i+2]) << " "
            << std::setw(2) << static_cast<int>(Data[i+3]) << "\n";
    }
    out.close();

    // Also overwrite data.byte for convenience
    std::ofstream db("data.byte");
    for (int i = 0; i < DATA_MEM_SIZE; i += 4) {
        db << std::hex << std::uppercase << std::setfill('0')
           << std::setw(2) << static_cast<int>(Data[i])   << " "
           << std::setw(2) << static_cast<int>(Data[i+1]) << " "
           << std::setw(2) << static_cast<int>(Data[i+2]) << " "
           << std::setw(2) << static_cast<int>(Data[i+3]) << "\n";
    }
}
