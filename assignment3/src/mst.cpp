#include "a3/mst.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <utility>

namespace a3 {
namespace {

// Disjoint-set union with union by size and path halving (near-constant amortized operations).
class DisjointSets {
 public:
  explicit DisjointSets(int n) : parent_(static_cast<std::size_t>(n)), size_(static_cast<std::size_t>(n), 1) {
    std::iota(parent_.begin(), parent_.end(), 0);
  }

  int find(int x) {
    while (parent_[x] != x) {
      parent_[x] = parent_[parent_[x]];
      x = parent_[x];
    }
    return x;
  }

  // Merges the sets containing a and b; returns false if they were already the same set.
  bool unite(int a, int b) {
    a = find(a);
    b = find(b);
    if (a == b) return false;
    if (size_[a] < size_[b]) std::swap(a, b);
    parent_[b] = a;
    size_[a] += size_[b];
    return true;
  }

 private:
  std::vector<int> parent_;
  std::vector<int> size_;
};

}  // namespace

MSTResult kruskal_mst(const cs509::CSRGraph& graph) {
  const int n = graph.num_vertices;

  // Each undirected edge is stored twice in CSR; keep the copy with u < v.
  std::vector<MSTEdge> edges;
  edges.reserve(graph.col_idx.size() / 2);
  for (int u = 0; u < n; ++u) {
    for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
      if (u < graph.col_idx[k]) edges.push_back({u, graph.col_idx[k], graph.values[k]});
    }
  }
  // Equal weights are ordered by endpoints so the selected edge list is identical on every platform.
  std::sort(edges.begin(), edges.end(), [](const MSTEdge& a, const MSTEdge& b) {
    if (a.weight != b.weight) return a.weight < b.weight;
    if (a.u != b.u) return a.u < b.u;
    return a.v < b.v;
  });

  MSTResult result;
  result.edges.reserve(static_cast<std::size_t>(n - 1));
  DisjointSets components(n);
  for (const MSTEdge& edge : edges) {
    if (static_cast<int>(result.edges.size()) == n - 1) break;
    if (components.unite(edge.u, edge.v)) {
      result.edges.push_back(edge);
      result.total_weight += edge.weight;
    }
  }
  result.spanning = static_cast<int>(result.edges.size()) == n - 1;
  return result;
}

MSTResult prim_mst(const cs509::CSRGraph& graph, int start_vertex) {
  const int n = graph.num_vertices;
  if (start_vertex < 0 || start_vertex >= n) throw std::invalid_argument("Prim start vertex is out of range");

  struct Candidate {
    long long weight;
    int vertex;
    int parent;
  };
  // Min-heap on (weight, vertex) so ties are resolved the same way on every platform.
  auto later = [](const Candidate& a, const Candidate& b) {
    if (a.weight != b.weight) return a.weight > b.weight;
    return a.vertex > b.vertex;
  };
  std::priority_queue<Candidate, std::vector<Candidate>, decltype(later)> heap(later);

  // best[v] is the cheapest known edge from the tree to v; a vertex is only pushed when it improves,
  // so older, heavier heap entries for v are skipped when popped (lazy deletion).
  std::vector<long long> best(static_cast<std::size_t>(n), std::numeric_limits<long long>::max());
  std::vector<char> in_tree(static_cast<std::size_t>(n), 0);
  MSTResult result;
  result.edges.reserve(static_cast<std::size_t>(n - 1));

  auto add_to_tree = [&](int u) {
    in_tree[u] = 1;
    for (long long k = graph.row_ptr[u]; k < graph.row_ptr[u + 1]; ++k) {
      const int v = graph.col_idx[k];
      const long long w = graph.values[k];
      if (!in_tree[v] && w < best[v]) {
        best[v] = w;
        heap.push({w, v, u});
      }
    }
  };

  add_to_tree(start_vertex);
  int tree_size = 1;
  while (!heap.empty() && tree_size < n) {
    const Candidate next = heap.top();
    heap.pop();
    if (in_tree[next.vertex]) continue;  // stale entry
    result.edges.push_back({next.parent, next.vertex, next.weight});
    result.total_weight += next.weight;
    ++tree_size;
    add_to_tree(next.vertex);
  }
  result.spanning = tree_size == n;
  return result;
}

}  // namespace a3
