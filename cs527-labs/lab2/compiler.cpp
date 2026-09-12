#include "compiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cctype>
#include <algorithm>
#include <cstdint>

struct Instr {
    int op, d, s1, s2;
    std::string labelTarget; // for branches, resolved later
    bool isBranch = false;
};

static int parseReg(const std::string& tok) {
    if (tok.size() < 2 || (tok[0] != 'x' && tok[0] != 'X')) return -1;
    try {
        int n = std::stoi(tok.substr(1));
        return (n >= 0 && n <= 255) ? n : -1;
    } catch (...) { return -1; }
}

static bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0]=='-'||s[0]=='+') ? 1 : 0;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

static std::string trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

bool compile(const std::string& sourceFile) {
    std::ifstream in(sourceFile);
    if (!in) {
        std::cerr << "Cannot open " << sourceFile << "\n";
        return false;
    }

    std::vector<Instr> code;
    std::map<std::string, int> labels; // label -> instruction index
    std::string line;
    int lineno = 0;

    while (std::getline(in, line)) {
        ++lineno;
        // comments
        auto p = line.find('%');
        if (p != std::string::npos) line = line.substr(0, p);
        line = trim(line);
        if (line.empty()) continue;

        // Label definition: starts with .
        if (line[0] == '.') {
            std::string lab = line;
            // remove possible trailing junk
            auto sp = lab.find_first_of(" \t");
            if (sp != std::string::npos) lab = lab.substr(0, sp);
            labels[lab] = static_cast<int>(code.size());
            continue;
        }

        std::istringstream iss(line);
        std::string t1;
        iss >> t1;

        // Branch: Bxx .label
        if (t1.size() >= 2 && (t1[0]=='B' || t1[0]=='b')) {
            std::string target;
            iss >> target;
            if (target.empty() || target[0] != '.') {
                std::cerr << "Line " << lineno << ": bad branch target\n";
                return false;
            }
            int cond = 0x0E; // AL default
            std::string suf = t1.substr(1);
            for (auto& c : suf) c = toupper(c);
            if (suf == "EQ") cond = 0x00;
            else if (suf == "NE") cond = 0x01;
            else if (suf == "CS" || suf == "HS") cond = 0x02;
            else if (suf == "CC" || suf == "LO") cond = 0x03;
            else if (suf == "MI") cond = 0x04;
            else if (suf == "PL") cond = 0x05;
            else if (suf == "VS") cond = 0x06;
            else if (suf == "VC") cond = 0x07;
            else if (suf == "GE") cond = 0x0A;
            else if (suf == "LT") cond = 0x0B;
            else if (suf == "GT") cond = 0x0C;
            else if (suf == "LE") cond = 0x0D;
            else if (suf == "AL" || suf == "") cond = 0x0E;
            else {
                std::cerr << "Line " << lineno << ": unknown branch suffix " << suf << "\n";
                return false;
            }
            Instr i;
            i.op = 0x10 + cond;
            i.d = 0; i.s1 = 0; i.s2 = 0;
            i.isBranch = true;
            i.labelTarget = target;
            code.push_back(i);
            continue;
        }

        // Legacy Read / Write
        if (t1 == "Read" || t1 == "read") {
            std::string r, a; iss >> r >> a;
            int reg = parseReg(r);
            if (reg < 0 || !isNumber(a)) { std::cerr << "Line " << lineno << ": bad Read\n"; return false; }
            code.push_back({0x05, reg, std::stoi(a), 0});
            continue;
        }
        if (t1 == "Write" || t1 == "write") {
            std::string r, a; iss >> r >> a;
            int reg = parseReg(r);
            if (reg < 0 || !isNumber(a)) { std::cerr << "Line " << lineno << ": bad Write\n"; return false; }
            code.push_back({0x06, reg, std::stoi(a), 0});
            continue;
        }

        // Memory forms with [] : x1 = [x2]  or  [x2] = x1  or  x1 = [0]
        // First handle store: [dest] = src
        if (t1[0] == '[') {
            // [addr] = value
            auto close = t1.find(']');
            if (close == std::string::npos) { std::cerr << "Line " << lineno << ": bad []\n"; return false; }
            std::string addrTok = t1.substr(1, close-1);
            std::string eq, valTok;
            iss >> eq >> valTok;
            if (eq != "=") { std::cerr << "Line " << lineno << ": expected =\n"; return false; }
            int valReg = parseReg(valTok);
            if (valReg < 0) { std::cerr << "Line " << lineno << ": store needs register\n"; return false; }
            if (isNumber(addrTok)) {
                code.push_back({0x0E, valReg, std::stoi(addrTok), 0});
            } else {
                int areg = parseReg(addrTok);
                if (areg < 0) { std::cerr << "Line " << lineno << ": bad address\n"; return false; }
                code.push_back({0x06, valReg, areg, 0});
            }
            continue;
        }

        // Destination register
        int dest = parseReg(t1);
        if (dest < 0) {
            std::cerr << "Line " << lineno << ": expected register\n";
            return false;
        }
        std::string eq; iss >> eq;
        if (eq != "=") { std::cerr << "Line " << lineno << ": expected =\n"; return false; }

        std::string rhs1; iss >> rhs1;
        if (rhs1.empty()) { std::cerr << "Line " << lineno << ": missing RHS\n"; return false; }

        // Load form: xD = [addr]
        if (rhs1[0] == '[') {
            auto close = rhs1.find(']');
            std::string addrTok = rhs1.substr(1, close-1);
            if (isNumber(addrTok)) {
                code.push_back({0x0D, dest, std::stoi(addrTok), 0});
            } else {
                int areg = parseReg(addrTok);
                if (areg < 0) { std::cerr << "Line " << lineno << ": bad load addr\n"; return false; }
                code.push_back({0x05, dest, areg, 0});
            }
            continue;
        }

        // Constant move
        if (isNumber(rhs1)) {
            int v = std::stoi(rhs1);
            if (v < 0 || v > 255) { std::cerr << "Line " << lineno << ": const out of range\n"; return false; }
            code.push_back({0x0F, dest, v, 0});
            continue;
        }

        // Binary: xD = xS1 op xS2   or   xD = xS1 op imm
        int s1 = parseReg(rhs1);
        if (s1 < 0) { std::cerr << "Line " << lineno << ": bad operand\n"; return false; }
        std::string op, rhs2; iss >> op >> rhs2;
        if (op.empty() || rhs2.empty()) { std::cerr << "Line " << lineno << ": incomplete expression\n"; return false; }

        int opcode = 0;
        bool imm = isNumber(rhs2);
        int s2 = imm ? std::stoi(rhs2) : parseReg(rhs2);
        if (!imm && s2 < 0) { std::cerr << "Line " << lineno << ": bad second operand\n"; return false; }
        if (imm && (s2 < 0 || s2 > 255)) { std::cerr << "Line " << lineno << ": imm out of range\n"; return false; }

        if (op == "+") opcode = imm ? 0x09 : 0x01;
        else if (op == "-") opcode = imm ? 0x0A : 0x02;
        else if (op == "*") opcode = imm ? 0x0B : 0x03;
        else if (op == "/") opcode = imm ? 0x0C : 0x04;
        else { std::cerr << "Line " << lineno << ": unknown op " << op << "\n"; return false; }

        code.push_back({opcode, dest, s1, s2});
    }

    // Resolve branches
    for (size_t i = 0; i < code.size(); ++i) {
        if (!code[i].isBranch) continue;
        auto it = labels.find(code[i].labelTarget);
        if (it == labels.end()) {
            std::cerr << "Undefined label " << code[i].labelTarget << "\n";
            return false;
        }
        int targetIdx = it->second;
        int offset = targetIdx - static_cast<int>(i) - 1; // relative to next instruction
        if (offset < -128 || offset > 127) {
            std::cerr << "Branch offset out of range for " << code[i].labelTarget << "\n";
            return false;
        }
        code[i].s2 = static_cast<uint8_t>(static_cast<int8_t>(offset));
    }

    // Emit
    std::ofstream out("program.byte");
    if (!out) { std::cerr << "Cannot write program.byte\n"; return false; }
    for (auto& i : code) {
        out << std::hex << std::uppercase
            << i.op << " " << i.d << " " << i.s1 << " " << static_cast<int>(i.s2 & 0xFF) << "\n";
    }
    out << "0 0 0 0\n";
    out.close();
    std::cout << "Compiled " << code.size() << " instructions → program.byte\n";
    return true;
}
