#include "a3/maxflow.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace a3 {

MaxFlowResult dinic_max_flow(const cs509::CSRGraph& network, int source, int sink) {
  const int n = network.num_vertices;
  if (source < 0 || source >= n || sink < 0 || sink >= n || source == sink) {
    throw std::invalid_argument("source and sink must be different vertices of the network");
  }
  const auto vertex_count = static_cast<std::size_t>(n);

  // Residual network in CSR layout: each original edge u->v with capacity c becomes a forward arc
  // u->v (capacity c) and a reverse arc v->u (capacity 0); rev[a] is the arc paired with a.
  std::vector<long long> head(vertex_count + 1, 0);
  for (int u = 0; u < n; ++u) {
    head[u + 1] += network.degree(u);
    for (long long k = network.row_ptr[u]; k < network.row_ptr[u + 1]; ++k) ++head[network.col_idx[k] + 1];
  }
  for (std::size_t u = 0; u < vertex_count; ++u) head[u + 1] += head[u];

  const auto arc_count = static_cast<std::size_t>(head[vertex_count]);
  std::vector<int> to(arc_count);
  std::vector<long long> capacity(arc_count);
  std::vector<long long> rev(arc_count);
  std::vector<long long> next_slot(head.begin(), head.end() - 1);
  for (int u = 0; u < n; ++u) {
    for (long long k = network.row_ptr[u]; k < network.row_ptr[u + 1]; ++k) {
      const int v = network.col_idx[k];
      const long long forward = next_slot[u]++;
      const long long backward = next_slot[v]++;
      to[forward] = v;
      capacity[forward] = network.values[k];
      rev[forward] = backward;
      to[backward] = u;
      capacity[backward] = 0;
      rev[backward] = forward;
    }
  }

  std::vector<int> level(vertex_count);
  std::vector<int> queue(vertex_count);
  // BFS from the source over arcs with remaining capacity; true if the sink is still reachable.
  auto build_level_graph = [&]() {
    std::fill(level.begin(), level.end(), -1);
    std::size_t front = 0;
    std::size_t back = 0;
    level[source] = 0;
    queue[back++] = source;
    while (front < back) {
      const int u = queue[front++];
      for (long long a = head[u]; a < head[u + 1]; ++a) {
        if (capacity[a] > 0 && level[to[a]] < 0) {
          level[to[a]] = level[u] + 1;
          queue[back++] = to[a];
        }
      }
    }
    return level[sink] >= 0;
  };

  long long flow = 0;
  std::vector<long long> current(vertex_count);  // next arc to try at each vertex (current-arc pointer)
  std::vector<long long> path;                   // arcs of the partial source -> u path; iterative, no recursion
  path.reserve(vertex_count);
  while (build_level_graph()) {
    std::copy(head.begin(), head.end() - 1, current.begin());
    path.clear();
    int u = source;
    for (;;) {
      if (u == sink) {
        // Augment by the bottleneck, then resume from the tail of the first saturated arc.
        long long bottleneck = std::numeric_limits<long long>::max();
        std::size_t first_saturated = 0;
        for (std::size_t i = 0; i < path.size(); ++i) {
          if (capacity[path[i]] < bottleneck) {
            bottleneck = capacity[path[i]];
            first_saturated = i;
          }
        }
        for (long long a : path) {
          capacity[a] -= bottleneck;
          capacity[rev[a]] += bottleneck;
        }
        flow += bottleneck;
        path.resize(first_saturated);
        u = path.empty() ? source : to[path.back()];
        continue;
      }

      long long& arc = current[u];
      const long long end = head[u + 1];
      while (arc < end && (capacity[arc] <= 0 || level[to[arc]] != level[u] + 1)) ++arc;
      if (arc < end) {
        path.push_back(arc);
        u = to[arc];
      } else if (u == source) {
        break;  // blocking flow reached for this level graph
      } else {
        // Dead end: drop u from the level graph and retreat along the last arc.
        level[u] = -1;
        path.pop_back();
        u = path.empty() ? source : to[path.back()];
        ++current[u];
      }
    }
  }

  MaxFlowResult result;
  result.max_flow = flow;

  // Minimum cut: the source side is every vertex still reachable from the source in the residual graph.
  result.source_side.assign(vertex_count, 0);
  std::size_t front = 0;
  std::size_t back = 0;
  result.source_side[source] = 1;
  queue[back++] = source;
  while (front < back) {
    const int u = queue[front++];
    for (long long a = head[u]; a < head[u + 1]; ++a) {
      if (capacity[a] > 0 && !result.source_side[to[a]]) {
        result.source_side[to[a]] = 1;
        queue[back++] = to[a];
      }
    }
  }
  for (int u = 0; u < n; ++u) {
    if (!result.source_side[u]) continue;
    for (long long k = network.row_ptr[u]; k < network.row_ptr[u + 1]; ++k) {
      const int v = network.col_idx[k];
      if (!result.source_side[v]) {
        result.cut_edges.push_back({u, v, network.values[k]});
        result.cut_capacity += network.values[k];
      }
    }
  }
  return result;
}

}  // namespace a3
