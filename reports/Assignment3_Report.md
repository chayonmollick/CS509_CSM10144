# CS509 Assignment 3 - Results

- Date: 2026-09-10 13:29
- Machine: macOS-15.7.9-arm64-arm-64bit-Mach-O, Apple M4
- Compiler: Apple clang version 17.0.0 (clang-1700.6.3.2) (flags: -std=c++17 -O2)

Timing covers only the algorithm call (clock started immediately before and stopped immediately after it); file reading, validation, adjacency-list-to-CSR conversion, result verification and printing are excluded. Each time is the average of repeated runs of the same call: 1000 runs for gradient descent, 100 runs for inputs with at most 1,000 vertices/points/objects, and 5 runs for larger inputs (1 run for fm_04).

## 1. MST - Kruskal and Prim

Weighted undirected connected graphs (CSR). Exp. Wt. = independent Python Kruskal (tools/verify_mst.py).

| File | V | E | Exp. Wt. | Kruskal Wt. | Prim Wt. | Kruskal Time | Prim Time | Equal? | Status |
|---|---|---|---|---|---|---|---|---|---|
| mst_10.txt | 10 | 30 | 712 | 712 | 712 | 0.0003 ms | 0.0004 ms | Yes | Pass |
| mst_100.txt | 100 | 300 | 11290 | 11290 | 11290 | 0.0027 ms | 0.0044 ms | Yes | Pass |
| mst_10000.txt | 10000 | 30000 | 1167003 | 1167003 | 1167003 | 1.4996 ms | 1.2911 ms | Yes | Pass |
| mst_50000.txt | 50000 | 150000 | 5818212 | 5818212 | 5818212 | 8.8505 ms | 8.2338 ms | Yes | Pass |
| mst_100000.txt | 100000 | 300000 | 11636203 | 11636203 | 11636203 | 18.3654 ms | 19.1040 ms | Yes | Pass |

## 2. Gradient Descent

| File | Degree | x0 | Rate | Tol. | Max Iter. | Exp. x* | Exp. f(x*) | Actual x | Actual f(x) | Iter. / Time | Status |
|---|---|---|---|---|---|---|---|---|---|---|---|
| gd_01.txt | 2 | 0 | 0.10 | 0.000001 | 5000 | 3 | 0 | 2.999999506349 | 0.000000000000 | 70 / 0.0002 ms | Pass |
| gd_02.txt | 4 | 2 | 0.02 | 0.000001 | 10000 | 0 | 0 | 0.000000237910 | 0.000000000000 | 180 / 0.0008 ms | Pass |
| gd_03.txt | 6 | 2 | 0.02 | 0.000001 | 20000 | 0 | 0 | 0.000000483545 | 0.000000000000 | 349 / 0.0021 ms | Pass |
| gd_04.txt | 8 | 2 | 0.01 | 0.00000001 | 50000 | 0 | 0 | 0.000000004975 | 0.000000000000 | 948 / 0.0072 ms | Pass |
| gd_05.txt | 10 | 2 | 0.005 | 0.0000000001 | 100000 | 0 | 0 | 0.000000000050 | 0.000000000000 | 2364 / 0.0218 ms | Pass |

## 3. Maxflow-Mincut (Dinic)

Directed capacity networks (CSR). Exp. Flow = independent Python max-flow (tools/verify_maxflow.py).

| File | V | E | Source | Sink | Exp. Flow | Actual Flow | Cut Capacity | Time | Status |
|---|---|---|---|---|---|---|---|---|---|
| maxflow_10.txt | 10 | 40 | 0 | 9 | 151 | 151 | 151 | 0.0008 ms | Pass |
| maxflow_100.txt | 100 | 400 | 0 | 99 | 728 | 728 | 728 | 0.0135 ms | Pass |
| maxflow_1000.txt | 1000 | 4000 | 0 | 999 | 1590 | 1590 | 1590 | 0.2336 ms | Pass |
| maxflow_10000.txt | 10000 | 40000 | 0 | 9999 | 4928 | 4928 | 4928 | 5.4904 ms | Pass |
| maxflow_50000.txt | 50000 | 200000 | 0 | 49999 | 11088 | 11088 | 11088 | 40.5005 ms | Pass |
| maxflow_100000.txt | 100000 | 400000 | 0 | 99999 | 15620 | 15620 | 15620 | 110.0587 ms | Pass |
