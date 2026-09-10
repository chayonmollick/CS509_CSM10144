// Assignment 2: adjacency-list and Compressed Sparse Row (CSR) graph representations.
#pragma once

#include <vector>

namespace cs509 {

// One entry of an adjacency list. Unweighted graphs store weight = 1.
struct AdjEdge {
  int to;
  long long weight;
};

// Graph exactly as listed in an input file: adj[u] holds u's neighbours in file order.
// Undirected graphs list every edge at both endpoints.
struct AdjacencyList {
  int num_vertices = 0;
  long long num_edges = 0;  // directed: number of arcs; undirected: each edge counted once
  bool directed = false;
  bool weighted = false;
  std::vector<std::vector<AdjEdge>> adj;
};

// Compressed Sparse Row graph. The neighbours of u are col_idx[row_ptr[u]] .. col_idx[row_ptr[u+1]-1],
// with matching weights (MST) or capacities (max-flow) in values. Unweighted graphs leave values empty.
struct CSRGraph {
  int num_vertices = 0;
  long long num_edges = 0;  // same meaning as AdjacencyList::num_edges
  bool directed = false;
  bool weighted = false;
  std::vector<long long> row_ptr;  // size V + 1
  std::vector<int> col_idx;        // size row_ptr[V] (2E entries for undirected graphs)
  std::vector<long long> values;   // size row_ptr[V], or empty when unweighted

  long long degree(int u) const { return row_ptr[u + 1] - row_ptr[u]; }
};

// Converts an adjacency list to CSR, keeping each vertex's neighbour order.
// This is preprocessing: callers must not include it in algorithm timings.
CSRGraph adjacency_list_to_csr(const AdjacencyList& list);

}  // namespace cs509
