# CS527 Mini-Computer System Simulator (Labs 1–5)

Complete C++ implementations of the progressive CS527 lab assignments.

## Repository Structure

```
cs527-labs/
├── README.md                 # This file
├── docs/
│   ├── operating-guide.md
│   ├── modification-guide.md
│   └── language-spec.md
├── examples/                 # Sample .txt programs + data
├── lab1/                     # Basic arithmetic + memory
├── lab2/                     # + flags, branches, labels, comments, constants
├── lab3/                     # + vector registers & vector ops
├── lab4/                     # + multi-processing + thin OS + Print
└── lab5/                     # + virtual memory (pages/frames)
```

Each `labN/` directory is **self-contained** and builds with a single `make`.

## Quick Start (any lab)

```bash
cd lab1          # or lab2 … lab5
make
./simulator examples/sum4.txt          # Lab 1 style
# or
./simulator prog.txt data.byte         # later labs
```

See `docs/operating-guide.md` for full details.

## Lab Progression Summary

| Lab | Key Features Added |
|-----|--------------------|
| 1   | Arithmetic, Read/Write, Data movement, simple compiler → bytecode, processor (reset/fetch/decode/execute), memory |
| 2   | Flags (Z/N/C/V), branches (BEQ/BNE/…), labels, comments (`%`), constant operands, larger data memory |
| 3   | 32 vector registers (v0–v31, 256-bit = 8×32-bit), vector ADD/SUB/MUL/LOAD/STORE |
| 4   | Multi-processing (NP processors), thin OS (scheduler, shell, loader), Print instruction, time-slicing |
| 5   | Virtual memory (logical ↔ physical), page tables, frames, `getPhysicalAddress` |

## Building & Testing

Every lab has its own `Makefile`.  
`make clean && make` produces the executable `simulator`.

Example programs are provided under `examples/` and also copied into each lab folder.

## Author Notes / Academic Use

- Code is written in modern C++17.
- Clear separation of compiler / processor / memory / OS.
- Extensive comments so you can understand and extend it.
- Designed so you can submit the corresponding `labN` folder (or the whole repo) for the lab.

See individual lab READMEs for the exact files expected by the assignment.

