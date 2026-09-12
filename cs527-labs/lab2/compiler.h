#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <vector>

// Compile a source program into program.byte
// Returns true on success
bool compile(const std::string& sourceFile);

#endif
