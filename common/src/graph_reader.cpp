#include "cs509/graph_reader.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

namespace cs509 {
namespace {

using std::to_string;

// An undirected adjacency entry stored with a < b, remembering which endpoint listed it.
struct EdgeCopy {
  int a;
  int b;
  long long weight;
  bool listed_at_a;
};

// Every undirected edge must be listed once at each endpoint with the same weight.
void check_undirected_lists(const AdjacencyList& list, const AdjacencyFormat& format, const Scanner& in,
                            long long entries) {
  std::vector<EdgeCopy> copies;
  copies.reserve(static_cast<std::size_t>(entries));
  for (int u = 0; u < list.num_vertices; ++u) {
    for (const AdjEdge& e : list.adj[u]) copies.push_back({std::min(u, e.to), std::max(u, e.to), e.weight, u < e.to});
  }
  std::sort(copies.begin(), copies.end(), [](const EdgeCopy& x, const EdgeCopy& y) {
    if (x.a != y.a) return x.a < y.a;
    if (x.b != y.b) return x.b < y.b;
    if (x.weight != y.weight) return x.weight < y.weight;
    return x.listed_at_a < y.listed_at_a;
  });

  for (std::size_t i = 0; i < copies.size();) {
    std::size_t pair_end = i;
    while (pair_end < copies.size() && copies[pair_end].a == copies[i].a && copies[pair_end].b == copies[i].b) {
      ++pair_end;
    }
    const std::string pair = "vertices " + to_string(copies[i].a) + " and " + to_string(copies[i].b);
    if (!format.allow_parallel_edges && pair_end - i > 2) {
      in.fail_file("the edge between " + pair + " is listed more than once (parallel edges are not allowed)");
    }
    for (std::size_t j = i; j < pair_end;) {
      long long at_a = 0;
      long long at_b = 0;
      std::size_t k = j;
      for (; k < pair_end && copies[k].weight == copies[j].weight; ++k) {
        if (copies[k].listed_at_a) {
          ++at_a;
        } else {
          ++at_b;
        }
      }
      if (at_a != at_b) {
        std::string edge = "the edge between " + pair;
        if (format.weighted) edge += " with " + std::string(format.weight_name) + " " + to_string(copies[j].weight);
        in.fail_file(edge + " is listed " + to_string(at_a) + " time(s) at vertex " + to_string(copies[i].a) + " but " +
                     to_string(at_b) + " time(s) at vertex " + to_string(copies[i].b) +
                     "; undirected edges must appear in both adjacency lists" +
                     (format.weighted ? " with the same weight" : ""));
      }
      j = k;
    }
    i = pair_end;
  }
}

}  // namespace

AdjacencyList read_adjacency_list(Scanner& in, const AdjacencyFormat& format) {
  if (!in.next_line()) in.fail("file is empty; expected the header line 'V E'");
  const long long v = in.read_int("V (number of vertices)");
  const long long e = in.read_int("E (number of edges)");
  in.expect_line_end("the header line 'V E'");
  if (v <= 0) in.fail("V must be positive, found " + to_string(v));
  if (v >= std::numeric_limits<int>::max()) in.fail("V is too large: " + to_string(v));
  if (e < 0) in.fail("E must not be negative, found " + to_string(e));

  AdjacencyList list;
  list.num_vertices = static_cast<int>(v);
  list.directed = format.directed;
  list.weighted = format.weighted;
  list.adj.resize(static_cast<std::size_t>(v));
  std::vector<char> seen(static_cast<std::size_t>(v), 0);
  long long entries = 0;

  for (long long listed = 0; listed < v; ++listed) {
    if (!in.next_line()) {
      in.fail("expected " + to_string(v) + " vertex lines but found only " + to_string(listed));
    }
    const long long u = in.read_int("vertex id");
    if (u < 0 || u >= v) in.fail("vertex id " + to_string(u) + " is out of range [0, " + to_string(v - 1) + "]");
    if (seen[u]) in.fail("vertex " + to_string(u) + " has more than one adjacency line");
    seen[u] = 1;

    const long long degree = in.read_int("degree");
    if (degree < 0) in.fail("vertex " + to_string(u) + " has a negative degree " + to_string(degree));
    std::vector<AdjEdge>& row = list.adj[u];
    row.reserve(static_cast<std::size_t>(std::min(degree, 1LL << 20)));

    for (long long k = 0; k < degree; ++k) {
      if (!in.has_more_on_line()) {
        in.fail("vertex " + to_string(u) + " declares degree " + to_string(degree) + " but lists only " +
                to_string(k) + " neighbour(s)");
      }
      const long long to = in.read_int("neighbour id");
      if (to < 0 || to >= v) {
        in.fail("neighbour id " + to_string(to) + " of vertex " + to_string(u) + " is out of range [0, " +
                to_string(v - 1) + "]");
      }
      if (to == u && !format.directed) in.fail("self-loop at vertex " + to_string(u) + " is not allowed");
      long long weight = 1;
      if (format.weighted) {
        if (!in.has_more_on_line()) {
          in.fail("missing " + std::string(format.weight_name) + " for edge " + to_string(u) + " -> " + to_string(to));
        }
        weight = in.read_int(format.weight_name);
        if (weight < 0 && !format.allow_negative_weights) {
          in.fail("negative " + std::string(format.weight_name) + " " + to_string(weight) + " on edge " +
                  to_string(u) + " -> " + to_string(to));
        }
      }
      row.push_back({static_cast<int>(to), weight});
    }
    if (in.has_more_on_line()) {
      in.fail("vertex " + to_string(u) + " declares degree " + to_string(degree) + " but lists more " +
              (format.weighted ? "values" : "neighbours") + " than that");
    }
    entries += degree;
  }

  if (!format.directed) check_undirected_lists(list, format, in, entries);
  const long long listed_edges = format.directed ? entries : entries / 2;
  if (listed_edges != e) {
    std::fprintf(stderr,
                 "Warning: %s: header declares E = %lld but the adjacency lists contain %lld %s edges; "
                 "using the listed edges\n",
                 in.path().c_str(), e, listed_edges, format.directed ? "directed" : "undirected");
  }
  list.num_edges = listed_edges;
  return list;
}

void expect_end_of_file(Scanner& in) {
  if (in.next_line()) in.fail("unexpected extra content after the adjacency list");
}

}  // namespace cs509
