# Modification Guide

## Adding a New Instruction

1. **Compiler (`compiler.cpp`)**
   - Extend the parser to recognise the new syntax.
   - Assign a new opcode (see bytecode tables in the lab statements).
   - Emit the 4-byte instruction.

2. **Processor (`processor.cpp`)**
   - In `execute()` add a new `case` for the opcode.
   - Update flags if the instruction affects Z/N/C/V.

3. **Headers**
   - Update any shared constants / enums in `*.h`.

## Changing Memory Size / Page Size (Lab 5)

Edit the `#define`s at the top of `memory.h` / `os.h`:

```cpp
#define MEMSIZE   8192
#define PAGESIZE  512
#define NP        4          // number of processors
```

Recompile.

## Adding a New Vector Operation

- Follow the pattern already used for ADD/SUB/MUL in both compiler and processor.
- Vector registers are stored as `uint32_t VReg[NP][32][8]`.

## Debugging Tips

- Print the generated `program.byte` (it is human-readable hex).
- Use the `Print` instruction (Lab 4+) liberally.
- Single-step by reducing the time-slice to 1 in the scheduler.

## Common Pitfalls

- Off-by-one in relative branch offsets.
- Forgetting that memory is **byte-addressable** but data is 32-bit.
- Not zeroing registers / flags on `reset()`.
- Page-table index calculation in Lab 5 (instruction pages vs data pages).

