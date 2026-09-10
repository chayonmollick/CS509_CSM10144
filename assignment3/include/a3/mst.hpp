// Minimum Spanning Tree on a weighted undirected CSR graph: Kruskal and Prim.
#pragma once

#include <vector>

#include "cs509/csr.hpp"

namespace a3 {

struct MSTEdge {
  int u;
  int v;
  long long weight;
};

struct MSTResult {
  // Kruskal: edges in selection order, printed as (smaller id, larger id).
  // Prim: edges in the order they were added, printed as (tree vertex, new vertex).
  std::vector<MSTEdge> edges;
  long long total_weight = 0;
  bool spanning = false;  // false when the graph is disconnected and no spanning tree exists
};

// Extracts the edges from CSR, sorts them by weight and joins components with a disjoint-set union.
MSTResult kruskal_mst(const cs509::CSRGraph& graph);

// Grows a single tree from start_vertex using a binary-heap priority queue with lazy deletion.
MSTResult prim_mst(const cs509::CSRGraph& graph, int start_vertex = 0);

}  // namespace a3
