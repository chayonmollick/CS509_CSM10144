#!/usr/bin/env python3
"""Independent PageRank reference (power iteration) for CS509 Assignment 4 inputs.

Usage: verify_pagerank.py <graph-file>

Input: header "V E", V lines "u outdegree n1 n2 ...", then DAMPING / TOLERANCE / MAX_ITERATIONS
lines in any order. The listed arcs are trusted over the header's E.
Iteration (init 1/N):
  PR'(v) = (1-d)/N + d*dangling_sum/N + d*sum_{u->v} PR(u)/outdeg(u)
  stop when sum_v |PR'(v)-PR(v)| <= TOLERANCE, or after MAX_ITERATIONS iterations.
Prints KEY value lines: VERTICES, ARCS, ITERATIONS, CONVERGED, FINAL_DIFF, TOP_VERTEX, SUM,
DANGLING, TOP5. Exit status: 0 ok, 2 parse error.
"""
import sys
from operator import sub


def fail(msg):
    print(f"PARSE_ERROR {msg}")
    sys.exit(2)


def main():
    if len(sys.argv) != 2:
        print("usage: verify_pagerank.py <graph-file>", file=sys.stderr)
        sys.exit(2)
    try:
        with open(sys.argv[1], "rb") as f:
            tok = f.read().split()
    except OSError as e:
        fail(f"cannot read {sys.argv[1]}: {e}")
    if len(tok) < 2:
        fail("missing header 'V E'")
    try:
        N = int(tok[0])
        E_header = int(tok[1])
    except ValueError:
        fail("header must be two integers 'V E'")
    if N < 1:
        fail("V must be >= 1")
    inl = [[] for _ in range(N)]
    outdeg = [0] * N
    seen = bytearray(N)
    pos = 2
    n_tok = len(tok)
    arcs = 0
    try:
        for _ in range(N):
            if pos + 2 > n_tok:
                fail("file ends before all vertex lines were read")
            u = int(tok[pos])
            deg = int(tok[pos + 1])
            pos += 2
            if not 0 <= u < N:
                fail(f"vertex id {u} out of range")
            if seen[u]:
                fail(f"vertex {u} listed twice")
            seen[u] = 1
            if deg < 0 or pos + deg > n_tok:
                fail(f"vertex {u}: bad degree {deg}")
            for v in map(int, tok[pos:pos + deg]):
                if not 0 <= v < N:
                    fail(f"vertex {u}: neighbour {v} out of range")
                inl[v].append(u)
            outdeg[u] = deg
            arcs += deg
            pos += deg
    except ValueError as e:
        fail(f"non-numeric token in adjacency lists: {e}")
    params = {}
    while pos < n_tok:
        key = tok[pos].decode(errors="replace").upper()
        if pos + 1 >= n_tok:
            fail(f"trailer key {key} without value")
        try:
            params[key] = float(tok[pos + 1])
        except ValueError:
            fail(f"trailer {key}: bad value")
        pos += 2
    for k in ("DAMPING", "TOLERANCE", "MAX_ITERATIONS"):
        if k not in params:
            fail(f"missing {k} line")
    d = params["DAMPING"]
    tol = params["TOLERANCE"]
    max_it = int(params["MAX_ITERATIONS"])

    dangling = [u for u in range(N) if outdeg[u] == 0]
    pr = [1.0 / N] * N
    it = 0
    converged = False
    diff = float("inf")
    while it < max_it:
        ds = 0.0
        for u in dangling:
            ds += pr[u]
        base = (1.0 - d) / N + d * ds / N
        contrib = [p / k if k else 0.0 for p, k in zip(pr, outdeg)]
        get = contrib.__getitem__
        new = [base + d * sum(map(get, lst)) for lst in inl]
        diff = sum(map(abs, map(sub, new, pr)))
        pr = new
        it += 1
        if diff <= tol:
            converged = True
            break

    order = sorted(range(N), key=lambda v: (-pr[v], v))
    top = order[0]
    print(f"VERTICES {N}")
    print(f"HEADER_ARCS {E_header}")
    print(f"ARCS {arcs}")
    print(f"ITERATIONS {it}")
    print(f"CONVERGED {'yes' if converged else 'no'}")
    print(f"FINAL_DIFF {diff:.3e}")
    print(f"TOP_VERTEX {top} {pr[top]:.6f}")
    print(f"SUM {sum(pr):.6f}")
    print(f"DANGLING {len(dangling)}")
    print("TOP5 " + " ".join(f"{v}:{pr[v]:.8f}" for v in order[:5]))


if __name__ == "__main__":
    main()
