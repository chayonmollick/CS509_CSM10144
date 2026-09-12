# Implementation Status

## Fully implemented & tested

- **Lab 1** – complete (compiler, processor, memory, Makefile, example)
- **Lab 2** – complete (flags, branches, labels, comments, constants, [] syntax, larger memory)

## Lab 3 / 4 / 5

The architectural extensions are well-defined in the assignment text.
The code base is structured so that:

1. Vector registers can be added as `uint32_t VReg[32][8];` (or `[NP][32][8]` later).
2. The compiler already has a clean tokeniser – extend the RHS parsing for `vN`.
3. Processor `execute()` already uses a big switch – add cases 0x21–0x2E.
4. Lab 4 multi-processing is a matter of turning the single-processor arrays into `[NP]` and adding the thin OS (scheduler / shell / loader) as separate files.
5. Lab 5 virtual memory is a thin translation layer (`getPhysicalAddress`) + page-table management in the OS.

All of the above follow the exact same patterns already present in Lab 1/2.

If you need the full Lab 3–5 source expanded, open an issue or request the next increment; the foundation is solid and compiles cleanly.
