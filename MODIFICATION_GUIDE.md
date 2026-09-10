# Modification Guide

How to change this project for a new task: where each part lives, how to rebuild and test, and ready-to-copy patterns. Every code block in this guide has been compiled and run.

## 1. The edit → compile → run loop

```bash
cd ~/CS509_CSM10144                               # repository root (the folder with the Makefile)
# 1. edit the file in the IDE and save it
make                                              # 2. rebuild; only changed files are recompiled
                                                  # 3. on errors: fix the FIRST one shown (file:line), save, make again
bin/cs509 <algorithm> <small-test-file>           # 4. check a small input
bin/cs509 <algorithm> <large-test-file> --quiet   # 5. check a large input
```

New `.cpp` files placed in `assignment3/src/` or `assignment4/src/` are compiled automatically; the Makefile does not need to change.

Save checkpoints with git (works without network):

```bash
git config user.name "Your Name"; git config user.email "you@example.com"   # once, if git asks who you are
git add -A && git commit -m "Task: add BFS"        # checkpoint
git diff                                           # changes since the last checkpoint
git checkout -- assignment4/src/pagerank.cpp       # discard the changes to one file
```

## 2. Where things are

Every algorithm runs the same way:

```
bin/cs509 <name> <file>
  └─ driver/main.cpp          looks up <name> in the kAlgorithms table and calls its runner
      └─ run_<name>()         assignmentN/src/runners.cpp
          1. read + validate  reader in assignmentN/src/inputs.cpp   (not timed)
          2. build CSR        cs509::adjacency_list_to_csr()         (not timed)
          3. run algorithm    inside cs509::run_timed(...)           (TIMED)
          4. print result     printf in the runner                   (not timed)
```

| Algorithm | Algorithm function | Reader (`inputs.cpp`) | Runner (`runners.cpp`) |
|---|---|---|---|
| Kruskal / Prim | `assignment3/src/mst.cpp`: `kruskal_mst`, `prim_mst` | `read_mst_graph` | `run_kruskal`, `run_prim`, `run_mst_compare` |
| Gradient Descent | `assignment3/src/gradient_descent.cpp`: `gradient_descent` | `read_gradient_descent_problem` | `run_gradient_descent` |
| Maxflow-Mincut | `assignment3/src/maxflow.cpp`: `dinic_max_flow` | `read_flow_network` | `run_maxflow` |
| Vertex Coloring | `assignment4/src/coloring.cpp`: `welsh_powell_coloring` | `read_coloring_input` | `run_coloring` |
| PageRank | `assignment4/src/pagerank.cpp`: `pagerank` | `read_pagerank_input` | `run_pagerank` |
| K-Means | `assignment4/src/kmeans.cpp`: `kmeans` | `read_kmeans_input` | `run_kmeans` |
| FastMap | `assignment4/src/fastmap.cpp`: `fastmap` | `read_fastmap_input` | `run_fastmap` |

Result and input structs (`MSTResult`, `MaxFlowResult`, `PageRankParams`, `KMeansResult`, …) are declared in the headers under `assignment3/include/a3/` and `assignment4/include/a4/`.

Shared building blocks in `common/include/cs509/`:

| Name | Use |
|---|---|
| `cs509::Scanner in(path)` | read a file line by line: `in.next_line()`, `in.read_int("name")`, `in.read_double("name")`, `in.read_word("name")`, `in.has_more_on_line()`, `in.expect_line_end("context")` |
| `in.fail("reason")` / `in.fail_file("reason")` | reject the input: prints `Error: file:line: reason` / `Error: file: reason`, exit status 1 |
| `cs509::read_adjacency_list(in, format)` | reads the `V E` header and adjacency lines and validates them |
| `cs509::expect_end_of_file(in)` | rejects extra lines after the adjacency list |
| `cs509::adjacency_list_to_csr(list)` | Assignment 2 CSR conversion; returns a `cs509::CSRGraph` |
| `cs509::run_timed(options.runs, ms, [&] { return algorithm(...); })` | times only the algorithm call and returns its result |
| `cs509::print_execution_time(ms, options.runs)` | prints `Execution time: ... ms` |

`cs509::AdjacencyFormat` fields:

| Field | Default | Meaning |
|---|---|---|
| `directed` | `false` | `true`: only outgoing edges are listed (self-loops allowed) |
| `weighted` | `false` | `true`: each neighbour id is followed by an integer weight |
| `allow_negative_weights` | `true` | `false` rejects negative weights (e.g. capacities) |
| `allow_parallel_edges` | `true` | undirected graphs: `false` rejects an edge listed twice |
| `weight_name` | `"weight"` | name used in error messages |

`cs509::CSRGraph` has `num_vertices`, `num_edges`, `row_ptr` (size V+1), `col_idx`, `values` (weights; empty when unweighted) and `degree(u)`. The neighbours of `u`:

```cpp
for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
  int v = graph.col_idx[k];
  long long w = graph.values[k];  // weighted graphs only
}
```

## 3. Which file to change

| The task asks you to… | Change |
|---|---|
| change how an algorithm works (stopping rule, ordering, tie-breaking, distance, initialization) | the algorithm function (table in §2) |
| print more information or change the output format | the `run_*` function in `runners.cpp` |
| return an extra value (e.g. a counter) | add a field to the `…Result` struct in the header, set it in the algorithm, print it in the runner |
| read a new parameter line from the input file (e.g. `START 3`) | the reader in `inputs.cpp`; copy the `SOURCE`/`SINK` loop in `read_flow_network` or the BFS runner in §4.3 |
| reject another kind of invalid input | the reader: `if (condition) in.fail("reason");` |
| use a different graph type (directed, weighted, ...) | the `AdjacencyFormat` fields in the reader |
| add a command-line option | `RunOptions` in `common/include/cs509/options.hpp` and the option loop in `driver/main.cpp` (§5) |
| add a new algorithm | §4 |
| test other input sizes | `bin/gen_graph`, `bin/gen_points`, `bin/gen_distance_matrix` (OPERATING_INSTRUCTIONS.md §5) |

Keep the timing rule: only the algorithm call goes inside `run_timed`. Reading, CSR conversion, checking and printing stay outside.

## 4. Worked example: add a new algorithm (BFS)

Goal: `bin/cs509 bfs <file>` prints the BFS hop distance of every vertex from a source. The input is a directed adjacency list followed by a `SOURCE s` line.

### 4.1 New file `assignment4/include/a4/bfs.hpp`

```cpp
// Breadth-first search hop distances from a source vertex on a CSR graph.
#pragma once

#include <vector>

#include "cs509/csr.hpp"

namespace a4 {

struct BfsResult {
  std::vector<int> distance;  // -1 for vertices that are not reachable
  int reached = 0;
};

BfsResult bfs(const cs509::CSRGraph& graph, int source);

}  // namespace a4
```

### 4.2 New file `assignment4/src/bfs.cpp`

```cpp
#include "a4/bfs.hpp"

#include <cstddef>

namespace a4 {

BfsResult bfs(const cs509::CSRGraph& graph, int source) {
  const int n = graph.num_vertices;
  BfsResult result;
  result.distance.assign(static_cast<std::size_t>(n), -1);
  std::vector<int> queue(static_cast<std::size_t>(n));
  int head = 0;
  int tail = 0;
  result.distance[source] = 0;
  queue[tail++] = source;
  while (head < tail) {
    const int u = queue[head++];
    for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
      const int v = graph.col_idx[k];
      if (result.distance[v] < 0) {
        result.distance[v] = result.distance[u] + 1;
        queue[tail++] = v;
      }
    }
  }
  result.reached = tail;
  return result;
}

}  // namespace a4
```

### 4.3 Runner in `assignment4/src/runners.cpp`

At the top of the file, with the other includes:

```cpp
#include <string>

#include "a4/bfs.hpp"
#include "cs509/graph_reader.hpp"
#include "cs509/scanner.hpp"
```

At the end of the file, just before the last line `}  // namespace a4`:

```cpp
int run_bfs(const cs509::RunOptions& options) {
  // 1. Read and validate the input (not timed).
  cs509::Scanner in(options.input_path);
  cs509::AdjacencyFormat format;
  format.directed = true;
  const cs509::AdjacencyList list = cs509::read_adjacency_list(in, format);
  long long source = -1;
  while (in.next_line()) {
    const std::string keyword = in.read_word("keyword");
    if (keyword == "SOURCE") {
      source = in.read_int("SOURCE");
    } else {
      in.fail("unknown keyword '" + keyword + "'; expected SOURCE");
    }
    in.expect_line_end(keyword.c_str());
  }
  if (source < 0 || source >= list.num_vertices) in.fail_file("missing or invalid SOURCE vertex");

  // 2. Convert to CSR with the Assignment 2 helper (not timed).
  const cs509::CSRGraph graph = cs509::adjacency_list_to_csr(list);

  // 3. Time only the algorithm call.
  double ms = 0.0;
  const BfsResult result = cs509::run_timed(options.runs, ms, [&] { return bfs(graph, static_cast<int>(source)); });

  // 4. Print the result (not timed).
  std::printf("Input: %s (V = %d, E = %lld)\n", options.input_path.c_str(), graph.num_vertices, graph.num_edges);
  std::printf("Algorithm: BFS\n");
  std::printf("Source: %lld\n", source);
  if (!options.quiet) {
    std::printf("Distances:\n");
    for (int v = 0; v < graph.num_vertices; ++v) std::printf("%d %d\n", v, result.distance[v]);
  }
  std::printf("Reachable vertices: %d\n", result.reached);
  cs509::print_execution_time(ms, options.runs);
  return 0;
}
```

### 4.4 Declare it in `assignment4/include/a4/runners.hpp`

Next to the other declarations, inside `namespace a4`:

```cpp
int run_bfs(const cs509::RunOptions& options);
```

### 4.5 Register it in `driver/main.cpp`

Add a line to the `kAlgorithms` table, after the `fastmap` line:

```cpp
    {"bfs", "A4", "Breadth-first search hop distances", a4::run_bfs},
```

### 4.6 Build, create a test file, run

```bash
make
cat > tests/assignment4/bfs_example.txt <<'EOF'
6 7
0 2 1 2
1 1 3
2 2 3 4
3 1 5
4 1 5
5 0
SOURCE 0
EOF
bin/cs509 bfs tests/assignment4/bfs_example.txt
```

Expected output:

```
Input: tests/assignment4/bfs_example.txt (V = 6, E = 7)
Algorithm: BFS
Source: 0
Distances:
0 0
1 1
2 1
3 2
4 2
5 3
Reachable vertices: 6
Execution time: ... ms
```

Variations:
- Undirected graph: `format.directed = false` (every edge must then be listed at both endpoints).
- Weighted graph: `format.weighted = true`, and read `graph.values[k]` in the algorithm.
- Input that is not a graph (like Gradient Descent): skip `read_adjacency_list` and read the file with the `Scanner`, as `read_gradient_descent_problem` in `assignment3/src/inputs.cpp` does.
- To try it on a large graph, copy an undirected coloring graph and append a source line: `cp tests/assignment4/color_50000.txt big.txt && echo "SOURCE 0" >> big.txt`. Also set `format.directed = false`, because coloring graphs list every edge at both endpoints; otherwise the reader warns that E does not match. Other generated files already end with their own keyword lines (`SOURCE`/`SINK`, `DAMPING`, ...), which this reader would reject.

## 5. Worked example: add a command-line option (`--start` for Prim)

1. `common/include/cs509/options.hpp`, inside `struct RunOptions`:

   ```cpp
   int start = 0;  // start vertex for Prim
   ```

2. `driver/main.cpp`, function `run_command_line`: add this branch just before the final `} else {` that prints "unknown option":

   ```cpp
       } else if (arg == "--start" && i + 1 < argc) {
         options.start = std::atoi(argv[++i]);
   ```

3. `assignment3/src/runners.cpp`, function `run_prim`: change `prim_mst(graph, 0)` to `prim_mst(graph, options.start)`.

4. Build and run:

   ```bash
   make
   bin/cs509 prim tests/assignment3/examples/mst_example.txt --start 3
   ```

   The total weight is still `16`; the edges are now listed starting from vertex 3. An out-of-range start prints `Error: Prim start vertex is out of range`.

## 6. Small changes to existing algorithms

| Task | File | Change |
|---|---|---|
| Maximum spanning tree with Kruskal | `assignment3/src/mst.cpp`, sort in `kruskal_mst` | `return a.weight < b.weight;` → `return a.weight > b.weight;` |
| PageRank stops on the largest single change instead of the sum | `assignment4/src/pagerank.cpp` | `change += std::fabs(next[v] - rank[v]);` → `change = std::max(change, std::fabs(next[v] - rank[v]));` |
| K-Means assigns points by Manhattan distance | `assignment4/src/kmeans.cpp`, assignment step | `distance += diff * diff;` → `distance += std::fabs(diff);` (WCSS stays squared) |
| Coloring visits the smallest degree first | `assignment4/src/coloring.cpp` | replace `max_degree - degree` with `degree` in both counting-sort lines |
| Report an extra number (e.g. iterations, augmenting paths) | algorithm header + `.cpp` + runner | add a field to the result struct, update it in the algorithm, print it in the runner |

## 7. Three more worked examples

These cover the three common kinds of task: changing only the output, adding an input parameter, and adding a new algorithm. Each was built and run; the outputs shown are real.

### 7.1 Change only the output: top 5 PageRank vertices

Task: "PageRank: also print the top 5 vertices."

In `assignment4/src/runners.cpp`, function `run_pagerank`, add this right after the line `std::printf("Top vertex: %d (rank %.6f)\n", top, ranks[top]);`:

```cpp
  // Top 5 vertices by rank (printing only, so it stays outside the timed call).
  std::vector<int> order(graph.num_vertices);
  for (int v = 0; v < graph.num_vertices; ++v) order[v] = v;
  const int shown = std::min(5, graph.num_vertices);
  std::partial_sort(order.begin(), order.begin() + shown, order.end(),
                    [&](int a, int b) { return ranks[a] > ranks[b]; });
  std::printf("Top %d vertices:\n", shown);
  for (int i = 0; i < shown; ++i) std::printf("  %d. vertex %d (rank %.6f)\n", i + 1, order[i], ranks[order[i]]);
```

```bash
make
bin/cs509 pagerank tests/assignment4/pagerank_50000.txt --quiet
```

```
Top vertex: 21369 (rank 0.001321)
Top 5 vertices:
  1. vertex 21369 (rank 0.001321)
  2. vertex 46333 (rank 0.000525)
  3. vertex 38949 (rank 0.000416)
  4. vertex 4846 (rank 0.000416)
  5. vertex 2781 (rank 0.000366)
```

### 7.2 New input parameter: Gradient Descent with momentum

Task: "Read `MOMENTUM β` (0 ≤ β < 1) from the input file and update v = β·v + f′(x), x = x − α·v."

1. `assignment3/include/a3/gradient_descent.hpp`, in `struct GradientDescentProblem`, after `long long max_iterations = 0;`:

   ```cpp
   double momentum = 0.0;  // beta in [0, 1); 0 gives plain gradient descent
   ```

2. `assignment3/src/inputs.cpp`, function `read_gradient_descent_problem`. Add a keyword branch after the `MAX_ITERATIONS` branch, before `} else {`:

   ```cpp
       } else if (keyword == "MOMENTUM") {
         problem.momentum = in.read_double("MOMENTUM");
   ```

   and the validation just before `return problem;`:

   ```cpp
     if (problem.momentum < 0.0 || problem.momentum >= 1.0) {
       in.fail_file("MOMENTUM must be in [0, 1), found " + format_number(problem.momentum));
     }
   ```

3. `assignment3/src/gradient_descent.cpp`, the update loop:

   ```diff
   +  double velocity = 0.0;
      while (std::fabs(gradient) > problem.tolerance && iterations < problem.max_iterations) {
   -    x -= problem.learning_rate * gradient;
   +    velocity = problem.momentum * velocity + gradient;
   +    x -= problem.learning_rate * velocity;
   ```

4. `assignment3/src/runners.cpp`, function `run_gradient_descent`, after the `Degree` line:

   ```cpp
   std::printf("Momentum: %g\n", problem.momentum);
   ```

Build and test:

```bash
make
cp tests/assignment3/gd_05.txt tests/assignment3/gd_05_momentum.txt
echo "MOMENTUM 0.9" >> tests/assignment3/gd_05_momentum.txt
bin/cs509 gd tests/assignment3/gd_05.txt            # Momentum: 0    Iterations: 2364  Converged: true
bin/cs509 gd tests/assignment3/gd_05_momentum.txt   # Momentum: 0.9  Iterations: 427   Converged: true
```

A file with `MOMENTUM 1.5` prints `Error: ...: MOMENTUM must be in [0, 1), found 1.5` (exit status 1). Files without a `MOMENTUM` line behave as before (momentum 0).

### 7.3 New algorithm: connected components

Task: "Count the connected components of an undirected graph and print the size of the largest one."

1. New file `assignment4/include/a4/components.hpp`:

   ```cpp
   // Connected components of an undirected CSR graph.
   #pragma once

   #include <vector>

   #include "cs509/csr.hpp"

   namespace a4 {

   struct ComponentsResult {
     std::vector<int> component;  // component id of every vertex
     int count = 0;
     int largest = 0;  // number of vertices in the biggest component
   };

   ComponentsResult connected_components(const cs509::CSRGraph& graph);

   }  // namespace a4
   ```

2. New file `assignment4/src/components.cpp`:

   ```cpp
   #include "a4/components.hpp"

   #include <algorithm>
   #include <cstddef>

   namespace a4 {

   // A BFS from every vertex that has no component yet; each BFS labels one whole component.
   ComponentsResult connected_components(const cs509::CSRGraph& graph) {
     const int n = graph.num_vertices;
     ComponentsResult result;
     result.component.assign(static_cast<std::size_t>(n), -1);
     std::vector<int> queue(static_cast<std::size_t>(n));
     for (int start = 0; start < n; ++start) {
       if (result.component[start] >= 0) continue;
       int head = 0;
       int tail = 0;
       queue[tail++] = start;
       result.component[start] = result.count;
       while (head < tail) {
         const int u = queue[head++];
         for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
           const int v = graph.col_idx[k];
           if (result.component[v] < 0) {
             result.component[v] = result.count;
             queue[tail++] = v;
           }
         }
       }
       result.largest = std::max(result.largest, tail);
       ++result.count;
     }
     return result;
   }

   }  // namespace a4
   ```

3. `assignment4/src/runners.cpp`: add `#include "a4/components.hpp"` with the other includes, and just before the last line `}  // namespace a4`:

   ```cpp
   int run_components(const cs509::RunOptions& options) {
     // Same input format as vertex coloring, so reuse its reader (validation included).
     const cs509::CSRGraph graph = cs509::adjacency_list_to_csr(read_coloring_input(options.input_path));

     double average_ms = 0.0;
     const ComponentsResult result =
         cs509::run_timed(options.runs, average_ms, [&] { return connected_components(graph); });

     std::printf("Input: %s (V = %d, E = %lld)\n", options.input_path.c_str(), graph.num_vertices, graph.num_edges);
     std::printf("Algorithm: Connected Components\n");
     print_listing_title("Vertex components", options.quiet, graph.num_vertices, "vertices");
     if (!options.quiet) {
       for (int v = 0; v < graph.num_vertices; ++v) std::printf("%d %d\n", v, result.component[v]);
     }
     std::printf("Components: %d\n", result.count);
     std::printf("Largest component: %d vertices\n", result.largest);
     cs509::print_execution_time(average_ms, options.runs);
     return 0;
   }
   ```

4. `assignment4/include/a4/runners.hpp`:

   ```cpp
   int run_components(const cs509::RunOptions& options);
   ```

5. `driver/main.cpp`, table `kAlgorithms`, after the `fastmap` line:

   ```cpp
       {"components", "A4", "Connected components", a4::run_components},
   ```

Build and test (the small graph has the components {0,1,2}, {3,4} and {5}):

```bash
make
printf '6 3\n0 1 1\n1 2 0 2\n2 1 1\n3 1 4\n4 1 3\n5 0\n' > tests/assignment4/components_example.txt
bin/cs509 components tests/assignment4/components_example.txt
bin/cs509 components tests/assignment4/color_100000.txt --quiet
```

```
Vertex components:
0 0
1 0
2 0
3 1
4 1
5 2
Components: 3
Largest component: 3 vertices
```

For `color_100000.txt`: `Components: 263`, `Largest component: 99734 vertices`.

## 8. Useful snippets

Reverse (transpose) a directed CSR graph, for algorithms that need incoming edges (e.g. strongly connected components). Put it in your `.cpp` inside `namespace { ... }` and add `#include <algorithm>` and `#include <vector>`:

```cpp
// Copy of g with every edge u->v reversed to v->u (weights are kept).
cs509::CSRGraph reverse_graph(const cs509::CSRGraph& g) {
  cs509::CSRGraph r = g;  // same sizes and flags; the arrays are refilled below
  std::fill(r.row_ptr.begin(), r.row_ptr.end(), 0);
  for (int v : g.col_idx) ++r.row_ptr[v + 1];
  for (int u = 0; u < g.num_vertices; ++u) r.row_ptr[u + 1] += r.row_ptr[u];
  std::vector<long long> next(r.row_ptr.begin(), r.row_ptr.end() - 1);
  for (int u = 0; u < g.num_vertices; ++u) {
    for (long long k = g.row_ptr[u]; k < g.row_ptr[u + 1]; ++k) {
      const long long slot = next[g.col_idx[k]]++;
      r.col_idx[slot] = u;
      if (g.weighted) r.values[slot] = g.values[k];
    }
  }
  return r;
}
```

Time two versions on the same input (see `run_mst_compare` in `assignment3/src/runners.cpp`):

```cpp
double kruskal_ms = 0.0;
double prim_ms = 0.0;
const MSTResult kruskal = cs509::run_timed(options.runs, kruskal_ms, [&] { return kruskal_mst(graph); });
const MSTResult prim = cs509::run_timed(options.runs, prim_ms, [&] { return prim_mst(graph, 0); });
```

Debug output that does not mix with the results (remove it afterwards):

```cpp
std::fprintf(stderr, "debug: u=%d v=%d\n", u, v);
```

## 9. When something goes wrong

| Message | Usual cause | Fix |
|---|---|---|
| `error: 'X' was not declared in this scope` | missing `#include`, missing `cs509::`/`a3::`/`a4::` prefix, or used before it is declared | add the include, prefix or declaration |
| `undefined reference to 'a4::run_bfs(...)'` | declared but never defined, defined outside `namespace a4`, or the `.cpp` is not in `src/` | define it inside `namespace a4 { ... }` in a `.cpp` under `src/` |
| `multiple definition of '...'` | the same helper function name in two `.cpp` files | put helpers inside `namespace { ... }` |
| `no matching function for call to ...` | wrong argument types or count | compare with the declaration in the header |
| `Error: unknown algorithm 'bfs'` | not added to `kAlgorithms` in `driver/main.cpp` | add the table line and run `make` |
| `Segmentation fault` | index out of range or empty vector | `make clean && make CXXFLAGS="-O1 -g -fsanitize=address,undefined"`, run again: it prints the file and line. Afterwards `make clean && make` |
| program never finishes | infinite loop | press Ctrl+C; add a `std::fprintf(stderr, ...)` inside the loop |
| a change has no effect | file not saved, or `make` not run | save, `make`, run `bin/cs509` from the repository root |
| strange errors after many edits | stale build files | `make clean && make` |

## 10. Before submitting

```bash
make clean && make                                         # clean build without errors
bin/cs509 mst tests/assignment3/examples/mst_example.txt   # existing examples still work
cd .. && tar --exclude=build --exclude=bin --exclude=fm_04.txt --exclude=raw -czvf CS509_CSM10144.tgz CS509_CSM10144
```
