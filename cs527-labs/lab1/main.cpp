#include "compiler.h"
#include "processor.h"
#include "memory.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.txt>\n";
        return 1;
    }

    std::string src = argv[1];

    if (!compile(src)) {
        std::cerr << "Compilation failed\n";
        return 1;
    }

    initialize("data.byte");
    reset();

    while (!end_of_simulation) {
        fetch();
        decode();
        execute();
    }

    finalize("data.out");
    std::cout << "Simulation finished. Final data memory written to data.out / data.byte\n";
    return 0;
}
