#include "a4/pagerank.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace a4 {

PageRankResult pagerank(const cs509::CSRGraph& graph, const PageRankParams& params) {
  const int n = graph.num_vertices;
  const double d = params.damping;
  PageRankResult result;
  result.ranks.assign(static_cast<std::size_t>(n), 1.0 / n);
  std::vector<double>& rank = result.ranks;
  std::vector<double> next(static_cast<std::size_t>(n));

  for (;;) {
    // Push each vertex's rank along its out-arcs, so the CSR can be used as is (no transpose),
    // and collect the rank held by dangling vertices in the same pass.
    std::fill(next.begin(), next.end(), 0.0);
    double dangling = 0.0;
    for (int u = 0; u < n; ++u) {
      const long long begin = graph.row_ptr[u];
      const long long end = graph.row_ptr[u + 1];
      if (begin == end) {
        dangling += rank[u];
        continue;
      }
      const double share = d * rank[u] / static_cast<double>(end - begin);
      for (long long k = begin; k < end; ++k) next[graph.col_idx[k]] += share;
    }

    // Teleport term plus the dangling mass, which is the same for every vertex.
    const double base = (1.0 - d) / n + d * dangling / n;
    double change = 0.0;
    for (int v = 0; v < n; ++v) {
      next[v] += base;
      change += std::fabs(next[v] - rank[v]);
    }
    rank.swap(next);
    ++result.iterations;
    result.final_change = change;
    if (change <= params.tolerance) {
      result.converged = true;
      break;
    }
    if (result.iterations >= params.max_iterations) break;
  }
  return result;
}

}  // namespace a4
