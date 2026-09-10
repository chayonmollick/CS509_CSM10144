#!/usr/bin/env python3
"""Checks a graph-colouring result produced by `bin/cs509 coloring <graph>` (CS509 Assignment 4).

Usage: verify_coloring.py <graph-file> <cs509-output-file>

Graph: header "V E", then V lines "u degree n1 n2 ...".
Output: lines after "Vertex colors:" of the form "<vertex> <color>", up to a line starting with
"Colors used:".
Prints KEY value lines: VERTICES, EDGES, COLORED, UNCOLORED, DUPLICATES, CONFLICTS, COLORS,
REPORTED_COLORS, VALID. Exit status: 0 valid, 1 invalid colouring, 2 parse error.
"""
import sys


def fail(msg):
    print(f"PARSE_ERROR {msg}")
    sys.exit(2)


def main():
    if len(sys.argv) != 3:
        print("usage: verify_coloring.py <graph-file> <cs509-output-file>", file=sys.stderr)
        sys.exit(2)
    try:
        with open(sys.argv[1], "rb") as f:
            tok = f.read().split()
    except OSError as e:
        fail(f"cannot read {sys.argv[1]}: {e}")
    if len(tok) < 2:
        fail("graph: missing header 'V E'")
    try:
        V = int(tok[0])
        int(tok[1])
    except ValueError:
        fail("graph: header must be two integers 'V E'")
    adj = [None] * V
    pos = 2
    try:
        for _ in range(V):
            if pos + 2 > len(tok):
                fail("graph: file ends before all vertex lines were read")
            u = int(tok[pos])
            deg = int(tok[pos + 1])
            pos += 2
            if not 0 <= u < V or adj[u] is not None:
                fail(f"graph: bad or repeated vertex id {u}")
            if deg < 0 or pos + deg > len(tok):
                fail(f"graph: vertex {u}: bad degree {deg}")
            adj[u] = list(map(int, tok[pos:pos + deg]))
            pos += deg
    except ValueError as e:
        fail(f"graph: non-numeric token: {e}")
    if pos != len(tok):
        fail("graph: unexpected trailing tokens")

    try:
        with open(sys.argv[2], "r", errors="replace") as f:
            lines = f.read().splitlines()
    except OSError as e:
        fail(f"cannot read {sys.argv[2]}: {e}")
    start = None
    for i, line in enumerate(lines):
        if line.strip().startswith("Vertex colors:"):
            start = i + 1
            break
    if start is None:
        fail("output: no 'Vertex colors:' line")
    color = [None] * V
    duplicates = 0
    reported = None
    for line in lines[start:]:
        s = line.strip()
        if s.startswith("Colors used:"):
            rest = s[len("Colors used:"):].split()
            reported = rest[0] if rest else ""
            break
        if not s:
            continue
        parts = s.split()
        if len(parts) != 2:
            fail(f"output: malformed colour line '{s}'")
        try:
            v = int(parts[0])
        except ValueError:
            fail(f"output: bad vertex id in '{s}'")
        if not 0 <= v < V:
            fail(f"output: vertex {v} out of range")
        if color[v] is not None:
            duplicates += 1
        color[v] = parts[1]
    else:
        fail("output: no 'Colors used:' line after the colour list")

    uncolored = sum(1 for c in color if c is None)
    conflicts = 0
    edges = 0
    for u in range(V):
        cu = color[u]
        for v in adj[u]:
            if not 0 <= v < V:
                fail(f"graph: neighbour {v} out of range")
            if u < v:
                edges += 1
            if u == v or (cu is not None and cu == color[v] and u < v):
                conflicts += 1
    colors = len({c for c in color if c is not None})
    valid = uncolored == 0 and duplicates == 0 and conflicts == 0
    print(f"VERTICES {V}")
    print(f"EDGES {edges}")
    print(f"COLORED {V - uncolored}")
    print(f"UNCOLORED {uncolored}")
    print(f"DUPLICATES {duplicates}")
    print(f"CONFLICTS {conflicts}")
    print(f"COLORS {colors}")
    print(f"REPORTED_COLORS {reported}")
    print(f"COLORS_MATCH {'yes' if reported == str(colors) else 'no'}")
    print(f"VALID {'yes' if valid else 'no'}")
    sys.exit(0 if valid else 1)


if __name__ == "__main__":
    main()
