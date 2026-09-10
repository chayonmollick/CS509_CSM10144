#!/usr/bin/env python3
"""Independent max-flow reference (iterative Dinic) for CS509 Assignment 3 inputs.

Usage: verify_maxflow.py <graph-file>

Input: header "V E", V lines "u degree n1 c1 n2 c2 ..." (outgoing arcs), then "SOURCE s" and
"SINK t" lines (any order). Parallel arcs are allowed and simply add capacity.
Prints KEY value lines: VERTICES, ARCS, SOURCE, SINK, EXPECTED_FLOW.
Exit status: 0 ok, 2 parse error.
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


def dinic(n, s, t, g, to, cap):
    flow = 0
    while True:
        level = [-1] * n
        level[s] = 0
        q = [s]
        for u in q:  # q grows while being iterated: plain BFS
            lu = level[u] + 1
            for e in g[u]:
                if cap[e] > 0:
                    v = to[e]
                    if level[v] < 0:
                        level[v] = lu
                        q.append(v)
            if level[t] >= 0 and lu > level[t]:
                break
        if level[t] < 0:
            return flow
        ptr = [0] * n
        path = []
        u = s
        while True:
            if u == t:
                f = min(cap[e] for e in path)
                cut = -1
                for i, e in enumerate(path):
                    cap[e] -= f
                    cap[e ^ 1] += f
                    if cut < 0 and cap[e] == 0:
                        cut = i
                flow += f
                del path[cut:]
                u = to[path[-1]] if path else s
                continue
            gu = g[u]
            i = ptr[u]
            m = len(gu)
            lu = level[u] + 1
            while i < m:
                e = gu[i]
                if cap[e] > 0 and level[to[e]] == lu:
                    break
                i += 1
            ptr[u] = i
            if i < m:
                e = gu[i]
                path.append(e)
                u = to[e]
            else:
                if u == s:
                    break
                level[u] = -1  # dead end for the rest of this phase
                e = path.pop()
                u = to[e ^ 1]
                ptr[u] += 1


def main():
    if len(sys.argv) != 2:
        print("usage: verify_maxflow.py <graph-file>", file=sys.stderr)
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
    g = [[] for _ in range(V)]
    to = []
    cap = []
    seen = bytearray(V)
    pos = 2
    n_tok = len(tok)
    arcs = 0
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
            cs = tok[pos + 1:pos + 2 * deg:2]
            pos += 2 * deg
            gu = g[u]
            for v, c in zip(map(int, nbrs), map(num, cs)):
                if not 0 <= v < V:
                    fail(f"vertex {u}: neighbour {v} out of range")
                if c < 0:
                    fail(f"arc {u}->{v}: negative capacity")
                if u == v:
                    continue
                e = len(to)
                gu.append(e)
                to.append(v)
                cap.append(c)
                g[v].append(e + 1)
                to.append(u)
                cap.append(0)
                arcs += 1
    except ValueError as e:
        fail(f"non-numeric token: {e}")
    src = sink = None
    while pos < n_tok:
        key = tok[pos].decode(errors="replace").upper()
        if pos + 1 >= n_tok:
            fail(f"trailer key {key} without value")
        try:
            val = int(tok[pos + 1])
        except ValueError:
            fail(f"trailer {key}: value must be an integer")
        if key == "SOURCE":
            src = val
        elif key == "SINK":
            sink = val
        else:
            fail(f"unexpected trailer token {key}")
        pos += 2
    if src is None or sink is None:
        fail("missing SOURCE or SINK line")
    if not (0 <= src < V and 0 <= sink < V):
        fail("SOURCE/SINK out of range")
    if src == sink:
        fail("SOURCE equals SINK")
    flow = dinic(V, src, sink, g, to, cap)
    print(f"VERTICES {V}")
    print(f"HEADER_ARCS {E_header}")
    print(f"ARCS {arcs}")
    print(f"SOURCE {src}")
    print(f"SINK {sink}")
    print(f"EXPECTED_FLOW {flow if isinstance(flow, int) else f'{flow:.6f}'}")


if __name__ == "__main__":
    main()
