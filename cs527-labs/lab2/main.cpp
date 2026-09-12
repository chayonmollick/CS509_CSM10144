#include "compiler.h"
#include "processor.h"
#include "memory.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.txt> [data.byte]\n";
        return 1;
    }
    std::string src = argv[1];
    std::string dataf = (argc >= 3) ? argv[2] : "data.byte";

    if (!compile(src)) return 1;

    initialize(dataf);
    reset();

    while (!end_of_simulation) {
        fetch();
        decode();
        execute();
    }

    finalize("data.out");
    std::cout << "Simulation finished.\n";
    return 0;
}
