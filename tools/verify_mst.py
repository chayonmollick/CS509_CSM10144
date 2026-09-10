#!/usr/bin/env python3
"""Independent MST reference (Kruskal + union-find) for CS509 Assignment 3 inputs.

Usage: verify_mst.py <graph-file>

Input: header "V E", then V lines "u degree n1 w1 n2 w2 ...". Every undirected edge is expected
in both endpoint lists with the same weight. Prints KEY value lines:
  VERTICES, LISTED_EDGES, SYMMETRIC, CONNECTED, COMPONENTS, EDGES, EXPECTED_WEIGHT
Exit status: 0 ok, 1 file parsed but inconsistent (asymmetric lists / header mismatch), 2 parse error.
"""
import sys


def fail(msg):
    print(f"PARSE_ERROR {msg}")
    sys.exit(2)


def num(tok):
    try:
        return int(tok)
    except ValueError:
        return float(tok)


def main():
    if len(sys.argv) != 2:
        print("usage: verify_mst.py <graph-file>", file=sys.stderr)
        sys.exit(2)
    try:
        with open(sys.argv[1], "rb") as f:
            tok = f.read().split()
    except OSError as e:
        fail(f"cannot read {sys.argv[1]}: {e}")
    if len(tok) < 2:
        fail("missing header 'V E'")
    try:
        V = int(tok[0])
        E_header = int(tok[1])
    except ValueError:
        fail("header must be two integers 'V E'")
    if V < 0:
        fail("negative V")
    fwd = []  # (w, u, v) listed at u with u < v
    back = []  # (w, v, u) listed at u with u > v
    self_loops = 0
    seen = bytearray(V)
    pos = 2
    n_tok = len(tok)
    try:
        for _ in range(V):
            if pos + 2 > n_tok:
                fail("file ends before all vertex lines were read")
            u = int(tok[pos])
            deg = int(tok[pos + 1])
            pos += 2
            if not 0 <= u < V:
                fail(f"vertex id {u} out of range")
            if seen[u]:
                fail(f"vertex {u} listed twice")
            seen[u] = 1
            if deg < 0 or pos + 2 * deg > n_tok:
                fail(f"vertex {u}: bad degree {deg}")
            nbrs = tok[pos:pos + 2 * deg:2]
            ws = tok[pos + 1:pos + 2 * deg:2]
            pos += 2 * deg
            for v, w in zip(map(int, nbrs), map(num, ws)):
                if not 0 <= v < V:
                    fail(f"vertex {u}: neighbour {v} out of range")
                if u < v:
                    fwd.append((w, u, v))
                elif u > v:
                    back.append((w, v, u))
                else:
                    self_loops += 1
    except ValueError as e:
        fail(f"non-numeric token: {e}")
    if pos != n_tok:
        fail(f"{n_tok - pos} unexpected trailing tokens")

    fwd.sort()
    back.sort()
    symmetric = fwd == back
    if symmetric:
        edges = fwd
    else:
        edges = sorted(set(fwd) | set(back))

    parent = list(range(V))
    total = 0
    used = 0
    for w, u, v in edges:
        while parent[u] != u:
            parent[u] = parent[parent[u]]
            u = parent[u]
        while parent[v] != v:
            parent[v] = parent[parent[v]]
            v = parent[v]
        if u != v:
            parent[u] = v
            total += w
            used += 1
            if used == V - 1:
                break
    components = V - used
    print(f"VERTICES {V}")
    print(f"HEADER_EDGES {E_header}")
    print(f"LISTED_EDGES {len(edges)}")
    print(f"SELF_LOOPS {self_loops}")
    print(f"SYMMETRIC {'yes' if symmetric else 'no'}")
    print(f"CONNECTED {'yes' if components <= 1 else 'no'}")
    print(f"COMPONENTS {components}")
    print(f"EDGES {used}")
    print(f"EXPECTED_WEIGHT {total if isinstance(total, int) else f'{total:.6f}'}")
    if not symmetric or E_header != len(edges):
        sys.exit(1)


if __name__ == "__main__":
    main()
