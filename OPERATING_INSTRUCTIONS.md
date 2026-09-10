# Operating Instructions

Step-by-step instructions for building and running the CS509 Assignment 3 and 4 programs on a Linux (Ubuntu) machine. macOS works the same way.

## 1. Requirements

| Tool | Needed for | Check |
|---|---|---|
| `g++` 7 or newer (C++17) | building the programs | `g++ --version` |
| `make` | building (optional, see §3) | `make --version` |
| `bash` | `scripts/generate_tests.sh` | — |
| `python3` | `scripts/make_report.py` only | `python3 --version` |

Tested with g++ 11.5 and g++ 13.4 on Linux, and with Apple clang 17 on macOS.

## 2. Get the code

```bash
cd ~
git clone https://github.com/chayonmollick/CS509_CSM10144.git
cd CS509_CSM10144
```

Without `git`, open the repository page in a browser and choose **Code → Download ZIP**, then:

```bash
cd ~/Downloads
unzip CS509_CSM10144-main.zip
cd CS509_CSM10144-main
```

## 3. Build

```bash
make -j4
```

This creates:
- `bin/cs509`: the program that runs every algorithm
- `bin/gen_graph`, `bin/gen_points`, `bin/gen_distance_matrix`: test-input generators

Run `make` again after editing any source file; only changed files are recompiled. `make clean` removes all build output.

If `make` is not installed, build the main program with a single command:

```bash
mkdir -p bin
g++ -std=c++17 -O2 -Icommon/include -Iassignment3/include -Iassignment4/include \
    common/src/*.cpp driver/main.cpp assignment3/src/*.cpp assignment4/src/*.cpp -o bin/cs509
```

## 4. Run

Run every command from the repository root, the folder that contains `Makefile`.

### 4.1 Interactive menu

```bash
bin/cs509
```

```
===== CS509 Algorithms (Assignments 3 and 4) =====
  1. kruskal   [A3] Minimum Spanning Tree - Kruskal
  2. prim      [A3] Minimum Spanning Tree - Prim
  3. mst       [A3] Kruskal and Prim on the same graph (comparison)
  4. gd        [A3] Gradient Descent on a polynomial
  5. maxflow   [A3] Maxflow-Mincut (Dinic)
  6. coloring  [A4] Greedy Vertex Coloring (Welsh-Powell order)
  7. pagerank  [A4] PageRank
  8. kmeans    [A4] K-Means Clustering
  9. fastmap   [A4] FastMap embedding
  0. Exit
Select an algorithm (number or name): 3
Input file path: tests/assignment3/mst_100.txt
Timed runs [1]:                                            <- press Enter for 1
Summary only, without per-vertex/edge listings? [y/N]: n
```

After printing the result, the menu appears again. Enter `0` to exit.

### 4.2 Command line

```bash
bin/cs509 <algorithm> <input-file> [--runs N] [--quiet] [--seed S]
```

| Algorithm | Runs |
|---|---|
| `kruskal` | Kruskal's MST |
| `prim` | Prim's MST |
| `mst` | Kruskal and Prim on the same graph, then compares the total weights |
| `gd` | Gradient Descent |
| `maxflow` | Maxflow-Mincut |
| `coloring` | Greedy Vertex Coloring |
| `pagerank` | PageRank |
| `kmeans` | K-Means Clustering |
| `fastmap` | FastMap |

| Option | Effect |
|---|---|
| `--runs N` | runs the timed algorithm call N times and prints the average time; use it for very fast inputs |
| `--quiet` | prints only the summary, without per-vertex or per-edge lines; use it for large inputs |
| `--seed S` | seed for FastMap's random pivot start (default 1) |

`bin/cs509 --help` prints the usage and `bin/cs509 --list` prints the algorithm names. To save output to a file, append `> output.txt`.

### 4.3 Quick check with the specification examples

| Command | Expected result |
|---|---|
| `bin/cs509 mst tests/assignment3/examples/mst_example.txt` | `Total MST weight: 16` for both algorithms |
| `bin/cs509 gd tests/assignment3/examples/gd_example.txt` | `Final x` ≈ 0, `Converged: true` |
| `bin/cs509 maxflow tests/assignment3/examples/maxflow_example.txt` | `Maximum flow: 23`, `Minimum cut capacity: 23` |
| `bin/cs509 coloring tests/assignment4/examples/color_example.txt` | `Colors used: 3`, `Valid coloring: Yes` |
| `bin/cs509 pagerank tests/assignment4/examples/pagerank_example.txt` | `Sum of ranks: 1.000000`, `Converged: true` |
| `bin/cs509 kmeans tests/assignment4/examples/km_example.txt` | `WCSS: 7.875000`, `Iterations: 3` |
| `bin/cs509 fastmap tests/assignment4/examples/fm_example.txt` | pivots for 2 dimensions and 5 coordinate rows |

The PageRank example file declares `E = 4` but lists 5 edges (as in the specification). The program prints a warning and uses the 5 listed edges.

### 4.4 Required test inputs

| Algorithm | Command | Input files |
|---|---|---|
| MST | `bin/cs509 mst tests/assignment3/mst_100000.txt --quiet` | `mst_10`, `mst_100`, `mst_10000`, `mst_50000`, `mst_100000` |
| Gradient Descent | `bin/cs509 gd tests/assignment3/gd_05.txt --runs 1000` | `gd_01` … `gd_05` |
| Maxflow-Mincut | `bin/cs509 maxflow tests/assignment3/maxflow_50000.txt --quiet` | `maxflow_10`, `_100`, `_1000`, `_10000`, `_50000` |
| Vertex Coloring | `bin/cs509 coloring tests/assignment4/color_100000.txt --quiet` | `color_10`, `_100`, `_10000`, `_50000`, `_100000` |
| PageRank | `bin/cs509 pagerank tests/assignment4/pagerank_50000.txt --quiet` | `pagerank_10`, `_100`, `_1000`, `_10000`, `_50000` |
| K-Means | `bin/cs509 kmeans tests/assignment4/km_04.txt --quiet` | `km_01` … `km_04` |
| FastMap | `bin/cs509 fastmap tests/assignment4/fm_03.txt --quiet` | `fm_01` … `fm_04` (`fm_04` must be generated, see §5) |

### 4.5 Invalid inputs

Every file in `tests/assignment3/invalid/` and `tests/assignment4/invalid/` must be rejected:

```bash
bin/cs509 gd tests/assignment3/invalid/gd_invalid_degree.txt
# Error: tests/assignment3/invalid/gd_invalid_degree.txt: invalid DEGREE 0; the degree must be a positive integer
echo $?     # 1
```

Exit status: `0` success, `1` invalid or missing input, `2` wrong command-line usage.

## 5. Generate test inputs

Every required input except `fm_04.txt` is already in the repository. The generator script recreates them with fixed seeds, so the files are identical each time.

```bash
bash scripts/generate_tests.sh              # every required input except fm_04
bash scripts/generate_tests.sh --large      # also tests/assignment4/fm_04.txt (N = 10,000, ~575 MB)
bash scripts/generate_tests.sh --optional   # also maxflow_100000.txt and pagerank_100000.txt
```

Custom inputs:

```bash
bin/gen_graph <mst|coloring|pagerank|maxflow> <V> <E> <output-file> [seed]
bin/gen_points <N> <D> <K> <output-file> [seed] [max_iterations] [tolerance]
bin/gen_distance_matrix <N> <k> <output-file> [seed]
```

For example, `bin/gen_graph mst 20000 60000 my_mst.txt 42` writes a connected weighted graph with 20,000 vertices and 60,000 edges.

## 6. Result tables

```bash
python3 scripts/make_report.py
```

This runs every test input and writes `reports/Assignment3_Report.md` and `reports/Assignment4_Report.md`, with raw program output in `reports/raw/`. The "expected" columns come from the independent Python checkers in `tools/verify_*.py`. Without `fm_04.txt` it takes about 15 seconds.

## 7. Input file formats

The input formats are exactly those in the Assignment 3 and 4 specifications. `tests/assignment3/examples/` and `tests/assignment4/examples/` contain one example file for each format.

## 8. Adding a new algorithm

[MODIFICATION_GUIDE.md](MODIFICATION_GUIDE.md) has the full details: where each part lives, a complete worked example (BFS), adding a command-line option, small changes to the existing algorithms, and fixes for common build errors. In short:

1. Put the algorithm in a new file, e.g. `assignment4/src/my_algorithm.cpp`, with a header in `assignment4/include/a4/`. The Makefile compiles every `.cpp` in `src/` automatically.
2. Add a runner in `assignment4/src/runners.cpp` and declare it in `assignment4/include/a4/runners.hpp`:

   ```cpp
   #include "cs509/graph_reader.hpp"
   #include "cs509/scanner.hpp"
   #include "cs509/timer.hpp"

   int run_my_algorithm(const cs509::RunOptions& options) {
     cs509::Scanner in(options.input_path);
     cs509::AdjacencyFormat format;  // set format.directed / format.weighted as needed
     const cs509::AdjacencyList list = cs509::read_adjacency_list(in, format);
     cs509::expect_end_of_file(in);
     const cs509::CSRGraph graph = cs509::adjacency_list_to_csr(list);  // preprocessing, not timed

     double ms = 0.0;
     const auto result = cs509::run_timed(options.runs, ms, [&] { return my_algorithm(graph); });
     // print the result here
     cs509::print_execution_time(ms, options.runs);
     return 0;
   }
   ```

3. Register it in the `kAlgorithms` table in `driver/main.cpp`:

   ```cpp
   {"myalgo", "A4", "My algorithm", a4::run_my_algorithm},
   ```

4. Run `make`, then `bin/cs509 myalgo <input-file>`.

## 9. Packaging

From the folder that contains `CS509_CSM10144`:

```bash
tar --exclude=build --exclude=bin --exclude=fm_04.txt --exclude=raw -czvf CS509_CSM10144.tgz CS509_CSM10144
tar -tzf CS509_CSM10144.tgz | head     # list the archive contents
ls -lh CS509_CSM10144.tgz
```

## 10. Troubleshooting

| Problem | Fix |
|---|---|
| `Permission denied` when running a script | run it with `bash scripts/...` or `python3 scripts/...` |
| `Error: cannot open input file` | run from the repository root, or check the path (Tab completes file names) |
| `g++: command not found` | try `make CXX=clang++`; otherwise a C++ compiler must be installed |
| `unrecognized command-line option '-std=c++17'` | the compiler is older than g++ 7; use a newer compiler |
| Output scrolls for too long | add `--quiet`, or redirect with `> output.txt` |
| FastMap on `fm_04.txt` reports out of memory | it needs about 400 MB of free RAM |
