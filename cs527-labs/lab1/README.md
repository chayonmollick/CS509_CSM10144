# Lab 1 – Basic Mini-Computer Simulator

## Files (as required by the assignment)

- `main.c` → `main.cpp`
- `compiler.c` / `compiler.h`
- `processor.c` / `processor.h`
- `memory.c` / `memory.h`
- `Makefile`

## Build & Run

```bash
make
./simulator examples/sum4.txt
```

The program adds four numbers stored at addresses 0,4,8,12 and writes the result to address 16.

Expected result (with the provided data.byte containing 10,20,30,40):

```
result = 100 (0x64) written at address 16
```

## Bytecode format

Each line of `program.byte`:

```
<opcode> <dest> <src1> <src2>
```

Opcodes: 1=ADD, 2=SUB, 3=MUL, 4=DIV, 5=READ, 6=WRITE, 7=MOV
