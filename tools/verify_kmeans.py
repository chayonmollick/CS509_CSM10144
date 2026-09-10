#!/usr/bin/env python3
"""Independent K-Means reference (Lloyd's algorithm) for CS509 Assignment 4 inputs.

Usage: verify_kmeans.py <points-file>

Input: header "N D K", N lines of D numbers, then MAX_ITERATIONS / TOLERANCE lines (any order).
Algorithm: centroids start as the first K points; each iteration assigns every point to the
nearest centroid (ties -> lower index) and recomputes centroids as cluster means (an empty cluster
keeps its previous centroid); shift = max Euclidean centroid movement; stop when
shift <= TOLERANCE or MAX_ITERATIONS iterations were done. WCSS = sum of squared distances of each
point to the final centroid of the cluster it was assigned to in the last iteration.
Prints KEY value lines: POINTS, DIMENSIONS, CLUSTERS, ITERATIONS, CONVERGED, FINAL_SHIFT, WCSS,
WCSS_REASSIGNED (points re-assigned to the final centroids), CLUSTER_SIZES.
Exit status: 0 ok, 2 parse error.
"""
import math
import sys

dist = math.dist


def fail(msg):
    print(f"PARSE_ERROR {msg}")
    sys.exit(2)


def assign(pts, cents):
    # Euclidean distance is monotone in the squared distance; list.index(min) picks the lowest index on ties.
    return [ds.index(min(ds)) for p in pts for ds in [[dist(p, c) for c in cents]]]


def main():
    if len(sys.argv) != 2:
        print("usage: verify_kmeans.py <points-file>", file=sys.stderr)
        sys.exit(2)
    try:
        with open(sys.argv[1], "rb") as f:
            tok = f.read().split()
    except OSError as e:
        fail(f"cannot read {sys.argv[1]}: {e}")
    if len(tok) < 3:
        fail("missing header 'N D K'")
    try:
        N, D, K = int(tok[0]), int(tok[1]), int(tok[2])
    except ValueError:
        fail("header must be three integers 'N D K'")
    if N < 1 or D < 1 or K < 1 or K > N:
        fail("need N,D,K >= 1 and K <= N")
    if 3 + N * D > len(tok):
        fail("file ends before all points were read")
    try:
        vals = list(map(float, tok[3:3 + N * D]))
    except ValueError as e:
        fail(f"non-numeric coordinate: {e}")
    pos = 3 + N * D
    params = {}
    while pos < len(tok):
        key = tok[pos].decode(errors="replace").upper()
        if pos + 1 >= len(tok):
            fail(f"trailer key {key} without value")
        try:
            params[key] = float(tok[pos + 1])
        except ValueError:
            fail(f"trailer {key}: bad value (or too many coordinates)")
        pos += 2
    if "MAX_ITERATIONS" not in params or "TOLERANCE" not in params:
        fail("missing MAX_ITERATIONS or TOLERANCE line")
    max_it = int(params["MAX_ITERATIONS"])
    tol = params["TOLERANCE"]

    cols = [vals[j::D] for j in range(D)]
    pts = list(zip(*cols))
    del vals
    cents = [pts[c] for c in range(K)]
    labels = [0] * N
    it = 0
    converged = False
    shift = float("inf")
    while it < max_it:
        labels = assign(pts, cents) if K > 1 else [0] * N
        counts = [0] * K
        for lab in labels:
            counts[lab] += 1
        sums = []
        for col in cols:
            s = [0.0] * K
            for lab, x in zip(labels, col):
                s[lab] += x
            sums.append(s)
        new = []
        for c in range(K):
            if counts[c]:
                new.append(tuple(sums[j][c] / counts[c] for j in range(D)))
            else:
                new.append(cents[c])
        shift = max(dist(a, b) for a, b in zip(cents, new))
        cents = new
        it += 1
        if shift <= tol:
            converged = True
            break

    wcss = math.fsum(dist(p, cents[lab]) ** 2 for p, lab in zip(pts, labels))
    relabels = assign(pts, cents) if K > 1 else [0] * N
    wcss_re = math.fsum(dist(p, cents[lab]) ** 2 for p, lab in zip(pts, relabels))
    sizes = [0] * K
    for lab in labels:
        sizes[lab] += 1
    print(f"POINTS {N}")
    print(f"DIMENSIONS {D}")
    print(f"CLUSTERS {K}")
    print(f"ITERATIONS {it}")
    print(f"CONVERGED {'yes' if converged else 'no'}")
    print(f"FINAL_SHIFT {shift:.6e}")
    print(f"WCSS {wcss:.6f}")
    print(f"WCSS_REASSIGNED {wcss_re:.6f}")
    print("CLUSTER_SIZES " + " ".join(map(str, sizes)))


if __name__ == "__main__":
    main()
