# CS509 (PGSL) — Assignments 3 and 4

C++17 implementations of the CS509 Assignment 3 and 4 algorithms, all run through one common wrapper:

| Assignment | Task | Algorithms | Input structure |
|---|---|---|---|
| 3 | Individual | MST — Kruskal and Prim | weighted undirected graph → CSR |
| 3 | Buddy | Gradient Descent (polynomial), Maxflow-Mincut (Dinic) | coefficients file / directed capacity graph → CSR |
| 4 | Individual | Greedy Vertex Coloring (Welsh-Powell order), PageRank | undirected / directed graph → CSR |
| 4 | Buddy | K-Means Clustering, FastMap | points file / distance matrix |

## Repository layout

```
common/                 Shared code from Assignments 1-2, built once as build/libcs509_common.a
  include/cs509/
    scanner.hpp         line-aware streaming tokenizer + InputError (all file readers use it)
    csr.hpp             AdjacencyList, CSRGraph and adjacency_list_to_csr()  (Assignment 2 helper)
    graph_reader.hpp    "V E / u degree neighbour [weight] ..." reader + validation
    timer.hpp           run_timed(): times only the algorithm call, averages over --runs
    options.hpp         RunOptions passed from the wrapper to every runner
driver/main.cpp         Common wrapper (Assignment 1): menu or command-line selection
assignment3/            mst, gradient_descent, maxflow (+ inputs = readers, runners = read/convert/time/print)
assignment4/            coloring, pagerank, kmeans, fastmap (+ inputs, runners)
tests/assignment3/      mst_*, gd_*, maxflow_* inputs; examples/ (spec examples); invalid/ (rejection tests)
tests/assignment4/      color_*, pagerank_*, km_*, fm_* inputs; examples/; invalid/
tools/                  gen_graph / gen_points / gen_distance_matrix (C++), verify_*.py (independent checks)
scripts/                generate_tests.sh (all inputs, fixed seeds), make_report.py (result tables)
reports/                Assignment3_Report.md, Assignment4_Report.md
```

Assignments 3 and 4 **call** `cs509::adjacency_list_to_csr()` from `common/`; the conversion code is not copied into either assignment.

## Build

Requires `g++` (7+) or `clang++` with C++17 and GNU make. See [OPERATING_INSTRUCTIONS.md](OPERATING_INSTRUCTIONS.md) for full step-by-step instructions.

```bash
make            # bin/cs509 (wrapper) and bin/gen_* (generators)
make clean
```

## Running

```bash
bin/cs509                                  # interactive menu
bin/cs509 --list                           # algorithm names
bin/cs509 <algorithm> <input-file> [--runs N] [--quiet] [--seed S]
```

| Algorithm name | What runs |
|---|---|
| `kruskal`, `prim` | one MST algorithm |
| `mst` | Kruskal and Prim on the same CSR graph, then compares the total weights |
| `gd` | Gradient Descent |
| `maxflow` | Dinic max flow + minimum cut |
| `coloring` | Greedy (Welsh-Powell) vertex coloring |
| `pagerank` | PageRank |
| `kmeans` | K-Means (Lloyd) |
| `fastmap` | FastMap |

Options: `--runs N` repeats the timed call N times and prints the average; `--quiet` skips per-vertex/per-edge listings for large inputs; `--seed S` seeds FastMap's random pivot start (default 1).

Examples:

```bash
bin/cs509 mst tests/assignment3/examples/mst_example.txt
bin/cs509 gd tests/assignment3/gd_05.txt --runs 1000
bin/cs509 maxflow tests/assignment3/maxflow_50000.txt --quiet
bin/cs509 coloring tests/assignment4/color_100000.txt --quiet
bin/cs509 pagerank tests/assignment4/examples/pagerank_example.txt
bin/cs509 kmeans tests/assignment4/km_04.txt --quiet
bin/cs509 fastmap tests/assignment4/fm_03.txt --quiet
```

Invalid or missing input files print `Error: <file>:<line>: <reason>` on stderr and exit with status 1; usage errors exit with 2.

## Timing rule

`cs509::run_timed()` starts a `steady_clock` immediately before the algorithm call and stops it immediately after. File reading, validation, adjacency-list → CSR conversion, result verification (e.g. coloring validity, FastMap distance error) and printing are outside the timed region. As the spec requires, these belong **inside** the timed call:
- Kruskal: extracting the edge list from CSR and sorting it.
- Maxflow: building the residual network from CSR and extracting the minimum cut.
- PageRank: every iteration. K-Means: every assignment and update step. FastMap: pivot selection, projection and deflation for all k dimensions.

## Algorithms and design notes

### Assignment 3
- **Kruskal** (`assignment3/src/mst.cpp`): keeps the `u < v` copy of each CSR edge, sorts by (weight, u, v), then uses a disjoint-set union (union by size + path halving). Stops after V−1 edges. O(E log E).
- **Prim**: starts at vertex 0 and uses a binary-heap priority queue with lazy deletion. It pushes a vertex only when its best known edge improves. O(E log V). Edges print as `parent child weight` in the order they are added.
- Both detect disconnected graphs and report an error (a spanning tree does not exist). Weights may be negative or zero.
- **Gradient Descent** (`gradient_descent.cpp`): one generic Horner evaluator for f(x) and f′(x), built from the coefficient list for any degree. The loop stops when |f′(x)| ≤ tolerance or after MAX_ITERATIONS updates. Non-finite iterates stop the loop and report `Converged: false`.
- **Maxflow-Mincut** (`maxflow.cpp`): Dinic's algorithm on a CSR residual network (forward arc + paired reverse arc). BFS builds the level graph, and an iterative DFS finds the blocking flow using current-arc pointers and dead-end pruning, so deep paths cannot overflow the stack. The min cut is the set of vertices still reachable from s in the final residual graph. Cut edges are the original edges from that set to the rest. O(V²E) worst case; fast on sparse graphs.

### Assignment 4
- **Vertex Coloring**: vertices sorted by non-increasing degree (counting sort, ties by id). Each vertex gets the smallest color not used by an already-colored neighbour, tracked with a stamped mark array. O(V+E). Validity is checked after timing.
- **PageRank**: push-style iteration over outgoing CSR edges. The rank of dangling vertices (outdegree 0) is spread evenly over all vertices. Stops when Σ|PRₜ − PRₜ₋₁| ≤ tolerance or MAX_ITERATIONS is reached.
- **K-Means**: the first K points are the initial centroids. Distance ties go to the lower cluster index, and an empty cluster keeps its previous centroid. Stops when the largest centroid shift ≤ tolerance or MAX_ITERATIONS is reached. WCSS is reported against the final centroids.
- **FastMap**: the distance matrix is stored as its strict upper triangle (N(N−1)/2 doubles), and the file is streamed, so no full N×N copy is kept. Deflation uses the paper's recursive distance, D′(i,j)² = D(i,j)² − Σ(xᵢ−xⱼ)² over the dimensions already produced. It is computed on demand, which gives the same result as rewriting the matrix. Pivots: a random start object (seeded), then two rounds of farthest-object search per dimension.

## Input validation (beyond format errors)

| Input | Rejected |
|---|---|
| All graphs | out-of-range vertex id, degree ≠ number of listed neighbours, duplicate/missing vertex lines |
| Undirected graphs (MST, Coloring) | self-loops, an edge not listed at both endpoints with the same weight; Coloring also rejects parallel edges |
| Maxflow | negative capacity, source/sink out of range, source = sink, missing SOURCE/SINK |
| Gradient Descent | degree < 1, coefficient count ≠ d+1, learning rate ≤ 0, tolerance ≤ 0, MAX_ITERATIONS ≤ 0 |
| PageRank | damping ≤ 0 or ≥ 1, tolerance ≤ 0, MAX_ITERATIONS ≤ 0 |
| K-Means | N, D or K ≤ 0, K > N, tolerance ≤ 0, MAX_ITERATIONS ≤ 0, wrong coordinate count |
| FastMap | non-square or non-symmetric matrix, non-zero diagonal, negative distance, k ≤ 0 or k ≥ N |

If the header's E does not match the listed edges, the program prints a warning and uses the listed edges. The spec's own PageRank example says `4 4` but lists 5 arcs.

## Test inputs

All generated inputs are reproducible: `scripts/generate_tests.sh` rebuilds them with fixed seeds, using a self-implemented splitmix64 generator, so files are identical on macOS and Linux.

```bash
scripts/generate_tests.sh              # every required input except fm_04
scripts/generate_tests.sh --large      # + fm_04.txt (N = 10,000, ~575 MB, not committed)
scripts/generate_tests.sh --optional   # + maxflow_100000.txt, pagerank_100000.txt
```

| File(s) | V / N | E | Graph type and properties |
|---|---|---|---|
| mst_{10,100,10000,50000,100000} | 10 … 100,000 | 3V (30 … 300,000) | weighted undirected, connected (random spanning tree + random extra edges), no self-loops/parallel edges, integer weights in [−100, 1000] |
| maxflow_{10,100,1000,10000,50000} | 10 … 50,000 | 4V (40 … 200,000) | directed, capacities in [1, 100], source 0, sink V−1, guaranteed s→t path (random Hamiltonian path), ≈√V extra arcs out of s and into t |
| gd_01 … gd_05 | – | – | polynomials of degree 2, 4, 6, 8, 10 exactly as in spec §4.3 |
| color_{10,100,10000,50000,100000} | 10 … 100,000 | 3V | unweighted undirected, random, may be disconnected, no self-loops/parallel edges |
| pagerank_{10,100,1000,10000,50000} | 10 … 50,000 | 4V | unweighted directed, ~1% dangling vertices (outdegree 0), every vertex has an incident edge, in-degree skewed toward popular vertices; d = 0.85, tol = 1e-6, max 1000 iterations |
| km_01 … km_04 | 100 / 1,000 / 10,000 / 100,000 | – | D = 2/2/5/5, K = 3/5/8/10, Gaussian-like blobs, MAX_ITERATIONS 300, TOLERANCE 1e-4 |
| fm_01 … fm_04 | 10 / 100 / 1,000 / 10,000 | – | symmetric Euclidean distances between random points in a 4-D space (axis ranges 100/60/30/10), k = 2/2/3/3 |

`tests/*/examples/` holds the spec's example inputs, and `tests/*/invalid/` holds one file per required rejection.

## Reports

```bash
make && python3 scripts/make_report.py      # writes reports/Assignment3_Report.md and reports/Assignment4_Report.md
```

The script runs every input through `bin/cs509`. It fills the "expected" columns from independent pure-Python reference implementations (`tools/verify_*.py`) and marks each row Pass/Fail. Raw program output is saved in `reports/raw/`.

## Notes on the specification examples

- **PageRank example** (A4 §6.2): the header says E = 4 but 5 arcs are listed. The printed sample ranks cannot come from the stated formula, because vertex 3 has no in-links, so its rank must be (1−d)/N = 0.0375. This implementation converges to ≈ 0.2019, 0.3736, 0.3869, 0.0375 (sum 1).
- **FastMap example** (A4 §8.3): the sample coordinates are illustrative and do not follow from the law-of-cosines projection. For example, with pivots 1 and 3, object 0 projects to (3² + 6² − 5²)/(2·6) = 1.6667. The spec itself notes that results differ with pivot choice.

## Operating instructions

Step-by-step instructions for a Linux (Ubuntu) machine — cloning, building with or without `make`, the menu and command-line modes, expected results for the examples, regenerating inputs, adding an algorithm, packaging and troubleshooting — are in [OPERATING_INSTRUCTIONS.md](OPERATING_INSTRUCTIONS.md).
