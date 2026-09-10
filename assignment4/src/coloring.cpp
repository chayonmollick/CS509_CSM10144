#include "a4/coloring.hpp"

#include <algorithm>
#include <cstddef>

namespace a4 {

ColoringResult welsh_powell_coloring(const cs509::CSRGraph& graph) {
  const auto n = static_cast<std::size_t>(graph.num_vertices);
  ColoringResult result;
  result.colors.assign(n, -1);
  if (n == 0) return result;

  std::size_t max_degree = 0;
  for (std::size_t u = 0; u < n; ++u) {
    max_degree = std::max(max_degree, static_cast<std::size_t>(graph.row_ptr[u + 1] - graph.row_ptr[u]));
  }

  // Welsh-Powell order by counting sort on the key (max_degree - degree): O(V + max_degree), and
  // stable, so vertices of equal degree stay in increasing id order without a comparison sort.
  std::vector<std::size_t> bucket_start(max_degree + 2, 0);
  for (std::size_t u = 0; u < n; ++u) {
    const auto degree = static_cast<std::size_t>(graph.row_ptr[u + 1] - graph.row_ptr[u]);
    ++bucket_start[max_degree - degree + 1];
  }
  for (std::size_t b = 1; b < bucket_start.size(); ++b) bucket_start[b] += bucket_start[b - 1];
  std::vector<int> order(n);
  for (std::size_t u = 0; u < n; ++u) {
    const auto degree = static_cast<std::size_t>(graph.row_ptr[u + 1] - graph.row_ptr[u]);
    order[bucket_start[max_degree - degree]++] = static_cast<int>(u);
  }

  // used_by[c] == v means some neighbour of v already has color c. Stamping with the current vertex
  // makes the array valid for exactly one vertex, so it never needs clearing (O(deg v) per vertex).
  // A vertex has at most max_degree neighbours, so a free color always exists in [0, max_degree].
  std::vector<int> used_by(max_degree + 1, -1);
  int colors_used = 0;
  for (const int v : order) {
    for (long long k = graph.row_ptr[v]; k < graph.row_ptr[v + 1]; ++k) {
      const int c = result.colors[graph.col_idx[k]];
      if (c >= 0) used_by[c] = v;
    }
    int color = 0;
    while (used_by[color] == v) ++color;
    result.colors[v] = color;
    colors_used = std::max(colors_used, color + 1);
  }
  result.colors_used = colors_used;
  return result;
}

bool is_valid_coloring(const cs509::CSRGraph& graph, const std::vector<int>& colors) {
  if (colors.size() != static_cast<std::size_t>(graph.num_vertices)) return false;
  for (int u = 0; u < graph.num_vertices; ++u) {
    if (colors[u] < 0) return false;
    for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
      if (colors[graph.col_idx[k]] == colors[u]) return false;
    }
  }
  return true;
}

}  // namespace a4
