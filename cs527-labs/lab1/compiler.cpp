#include "compiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

static int parseReg(const std::string& tok) {
    if (tok.size() < 2 || (tok[0] != 'x' && tok[0] != 'X')) return -1;
    try {
        int n = std::stoi(tok.substr(1));
        if (n < 0 || n > 255) return -1;
        return n;
    } catch (...) {
        return -1;
    }
}

static bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i)
        if (!std::isdigit(s[i])) return false;
    return true;
}

struct Instr {
    int op, d, s1, s2;
};

bool compile(const std::string& sourceFile) {
    std::ifstream in(sourceFile);
    if (!in) {
        std::cerr << "Cannot open source file: " << sourceFile << "\n";
        return false;
    }

    std::vector<Instr> code;
    std::string line;
    int lineno = 0;

    while (std::getline(in, line)) {
        ++lineno;
        // strip comments (none in Lab1, but keep simple)
        auto pos = line.find('%');
        if (pos != std::string::npos) line = line.substr(0, pos);

        // trim
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string tok1, tok2, tok3, tok4;
        iss >> tok1;

        // Read <var> <addr>
        if (tok1 == "Read" || tok1 == "read") {
            iss >> tok2 >> tok3;
            int reg = parseReg(tok2);
            if (reg < 0 || !isNumber(tok3)) {
                std::cerr << "Line " << lineno << ": bad Read\n";
                return false;
            }
            int addr = std::stoi(tok3);
            code.push_back({5, reg, addr, 0});
            continue;
        }

        // Write <var> <addr>
        if (tok1 == "Write" || tok1 == "write") {
            iss >> tok2 >> tok3;
            int reg = parseReg(tok2);
            if (reg < 0 || !isNumber(tok3)) {
                std::cerr << "Line " << lineno << ": bad Write\n";
                return false;
            }
            int addr = std::stoi(tok3);
            code.push_back({6, reg, addr, 0});
            continue;
        }

        // Assignment forms: xD = xS1 op xS2   or   xD = const
        // We expect the first token to be the destination register
        int dest = parseReg(tok1);
        if (dest < 0) {
            std::cerr << "Line " << lineno << ": expected register or Read/Write\n";
            return false;
        }

        std::string eq;
        iss >> eq;
        if (eq != "=") {
            std::cerr << "Line " << lineno << ": expected '='\n";
            return false;
        }

        iss >> tok2;
        if (tok2.empty()) {
            std::cerr << "Line " << lineno << ": missing RHS\n";
            return false;
        }

        // Constant move: xD = number
        if (isNumber(tok2)) {
            int val = std::stoi(tok2);
            if (val < 0 || val > 255) {
                std::cerr << "Line " << lineno << ": constant out of range\n";
                return false;
            }
            code.push_back({7, dest, val, 0});
            continue;
        }

        // Binary op: xD = xS1 op xS2
        int s1 = parseReg(tok2);
        if (s1 < 0) {
            std::cerr << "Line " << lineno << ": expected register on RHS\n";
            return false;
        }
        std::string op;
        iss >> op >> tok4;
        int s2 = parseReg(tok4);
        if (s2 < 0) {
            std::cerr << "Line " << lineno << ": expected second register\n";
            return false;
        }

        int opcode = 0;
        if (op == "+") opcode = 1;
        else if (op == "-") opcode = 2;
        else if (op == "*") opcode = 3;
        else if (op == "/") opcode = 4;
        else {
            std::cerr << "Line " << lineno << ": unknown operator " << op << "\n";
            return false;
        }
        code.push_back({opcode, dest, s1, s2});
    }

    // Write program.byte
    std::ofstream out("program.byte");
    if (!out) {
        std::cerr << "Cannot write program.byte\n";
        return false;
    }
    for (auto& i : code) {
        out << std::hex << std::uppercase
            << i.op << " " << i.d << " " << i.s1 << " " << i.s2 << "\n";
    }
    // terminator
    out << "0 0 0 0\n";
    out.close();
    std::cout << "Compiled " << code.size() << " instructions → program.byte\n";
    return true;
}
