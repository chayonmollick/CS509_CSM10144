# Lab 2 – Flags, Branches, Labels, Comments, Constants

Extends Lab 1 with:

- Condition flags Z N C V on ADD/SUB
- Full set of conditional branches (BEQ, BNE, BGE, … BAL)
- Labels (`.label`)
- Comments (`% ...`)
- Constant second operands (`x1 = x2 + 10`)
- Square-bracket memory syntax (`x1 = [x2]`, `[x2] = x1`)
- Larger data memory (4096 bytes)

## Build & Run

```bash
make
./simulator examples/array_sum.txt [data.byte]
```

## Key opcode changes (see assignment table)

- 0x01/0x09 ADD (reg / imm)
- 0x02/0x0A SUB
- 0x05 / 0x0D load
- 0x06 / 0x0E store
- 0x0F MOV imm
- 0x10+cc branch
