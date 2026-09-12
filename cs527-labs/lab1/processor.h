#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <cstdint>

constexpr int NUM_REGS = 256;

extern int32_t Register[NUM_REGS];
extern int PC;
extern int opcode, dest, src1, src2;
extern int end_of_simulation;

void reset();
void fetch();
void decode();
void execute();

#endif
