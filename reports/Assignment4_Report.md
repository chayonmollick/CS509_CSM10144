# CS509 Assignment 4 - Results

- Date: 2026-09-10 13:29
- Machine: macOS-15.7.9-arm64-arm-64bit-Mach-O, Apple M4
- Compiler: Apple clang version 17.0.0 (clang-1700.6.3.2) (flags: -std=c++17 -O2)

Timing covers only the algorithm call (clock started immediately before and stopped immediately after it); file reading, validation, adjacency-list-to-CSR conversion, result verification and printing are excluded. Each time is the average of repeated runs of the same call: 1000 runs for gradient descent, 100 runs for inputs with at most 1,000 vertices/points/objects, and 5 runs for larger inputs (1 run for fm_04).

## 1. Greedy Vertex Coloring (Welsh-Powell order)

Unweighted undirected graphs (CSR). Validity is checked by the program and independently by tools/verify_coloring.py.

| File | V | E | Colors Used | Max Degree | Valid? | Time | Status |
|---|---|---|---|---|---|---|---|
| color_10.txt | 10 | 30 | 5 | 8 | Yes (independent check: yes) | 0.0002 ms | Pass |
| color_100.txt | 100 | 300 | 5 | 12 | Yes (independent check: yes) | 0.0008 ms | Pass |
| color_10000.txt | 10000 | 30000 | 6 | 16 | Yes (independent check: yes) | 0.1737 ms | Pass |
| color_50000.txt | 50000 | 150000 | 6 | 19 | Yes (independent check: yes) | 1.0642 ms | Pass |
| color_100000.txt | 100000 | 300000 | 6 | 19 | Yes (independent check: yes) | 2.3969 ms | Pass |

## 2. PageRank

Unweighted directed graphs (CSR). Exp. Top = independent Python power iteration (tools/verify_pagerank.py).

| File | V | E | Damping | Top Vertex | Exp. Top | Dangling | Sum of Ranks | Iter. / Time | Status |
|---|---|---|---|---|---|---|---|---|---|
| pagerank_10.txt | 10 | 40 | 0.85 | 3 (rank 0.168279) | 3 | 1 | 1.000000 | 14 / 0.0005 ms | Pass |
| pagerank_100.txt | 100 | 400 | 0.85 | 69 (rank 0.034533) | 69 | 1 | 1.000000 | 20 / 0.0050 ms | Pass |
| pagerank_1000.txt | 1000 | 4000 | 0.85 | 655 (rank 0.008324) | 655 | 10 | 1.000000 | 19 / 0.0500 ms | Pass |
| pagerank_10000.txt | 10000 | 40000 | 0.85 | 8719 (rank 0.002350) | 8719 | 100 | 1.000000 | 19 / 0.5476 ms | Pass |
| pagerank_50000.txt | 50000 | 200000 | 0.85 | 21369 (rank 0.001321) | 21369 | 500 | 1.000000 | 19 / 5.5118 ms | Pass |
| pagerank_100000.txt | 100000 | 400000 | 0.85 | 21206 (rank 0.000899) | 21206 | 1000 | 1.000000 | 19 / 12.1248 ms | Pass |

## 3. K-Means Clustering

Exp. WCSS = independent Python Lloyd's algorithm (tools/verify_kmeans.py). Centroids are initialized with the first K points, as the assignment recommends; on km_03 and km_04 this converges to a local optimum (some blobs are split, others merged), so WCSS is higher than the best possible clustering.

| File | N | D | K | Max Iter. | Actual Iter. | WCSS | Exp. WCSS | Time | Status |
|---|---|---|---|---|---|---|---|---|---|
| km_01.txt | 100 | 2 | 3 | 300 | 3 | 2606.999460 | 2606.999460 | 0.0017 ms | Pass |
| km_02.txt | 1000 | 2 | 5 | 300 | 7 | 69182.218089 | 69182.218089 | 0.0396 ms | Pass |
| km_03.txt | 10000 | 5 | 8 | 300 | 36 | 11192836.662324 | 11192836.662324 | 5.5678 ms | Pass |
| km_04.txt | 100000 | 5 | 10 | 300 | 68 | 310038228.806629 | 310038228.806615 | 121.0754 ms | Pass |

## 4. FastMap

Pivot selection: random start object (seed 1), then 2 rounds of farthest-object search per dimension. Avg. distance error = mean |d(i,j) - ||x_i - x_j|| | over all pairs.

| File | N | Target k | Pivots (per dim) | Avg. Distance Error | Time | Status |
|---|---|---|---|---|---|---|
| fm_01.txt | 10 | 2 | 8 5; 4 9 | 3.921767 | 0.0002 ms | Pass |
| fm_02.txt | 100 | 2 | 54 21; 11 13 | 3.194229 | 0.0012 ms | Pass |
| fm_03.txt | 1000 | 3 | 630 643; 213 464; 470 583 | 0.356924 | 0.0171 ms | Pass |
| fm_04.txt | 10000 | 3 | 1123 626; 3930 5028; 197 2529 | 0.315080 | 0.3940 ms | Pass |
