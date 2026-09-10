#include "cs509/csr.hpp"

#include <cstddef>

namespace cs509 {

CSRGraph adjacency_list_to_csr(const AdjacencyList& list) {
  CSRGraph graph;
  graph.num_vertices = list.num_vertices;
  graph.num_edges = list.num_edges;
  graph.directed = list.directed;
  graph.weighted = list.weighted;

  const auto n = static_cast<std::size_t>(list.num_vertices);
  graph.row_ptr.assign(n + 1, 0);
  for (std::size_t u = 0; u < n; ++u) {
    graph.row_ptr[u + 1] = graph.row_ptr[u] + static_cast<long long>(list.adj[u].size());
  }

  const auto entries = static_cast<std::size_t>(graph.row_ptr[n]);
  graph.col_idx.resize(entries);
  if (list.weighted) graph.values.resize(entries);

  for (std::size_t u = 0; u < n; ++u) {
    auto k = static_cast<std::size_t>(graph.row_ptr[u]);
    for (const AdjEdge& edge : list.adj[u]) {
      graph.col_idx[k] = edge.to;
      if (list.weighted) graph.values[k] = edge.weight;
      ++k;
    }
  }
  return graph;
}

}  // namespace cs509
