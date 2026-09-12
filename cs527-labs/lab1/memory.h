#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <string>

constexpr int INSTR_MEM_SIZE = 256;
constexpr int DATA_MEM_SIZE  = 256;

extern uint8_t Instruction[INSTR_MEM_SIZE];
extern uint8_t Data[DATA_MEM_SIZE];

void initialize(const std::string& dataFile = "data.byte");
void finalize(const std::string& outFile = "data.out");

#endif
