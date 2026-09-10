// Assignment 4 runners: read and validate the input, build CSR for the graph algorithms, time only
// the algorithm call, then verify and print outside the timed region.
#include "a4/runners.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "a4/coloring.hpp"
#include "a4/fastmap.hpp"
#include "a4/inputs.hpp"
#include "a4/kmeans.hpp"
#include "a4/pagerank.hpp"
#include "cs509/csr.hpp"
#include "cs509/timer.hpp"

namespace a4 {
namespace {

// Prints "<title>:" before a per-item listing, or a one-line note when --quiet hides the listing.
void print_listing_title(const char* title, bool quiet, long long count, const char* items) {
  if (quiet) {
    std::printf("%s: (listing of %lld %s suppressed by --quiet)\n", title, count, items);
  } else {
    std::printf("%s:\n", title);
  }
}

}  // namespace

int run_coloring(const cs509::RunOptions& options) {
  // Assignment 2 CSR conversion (untimed); the adjacency list is released right after it.
  const cs509::CSRGraph graph = cs509::adjacency_list_to_csr(read_coloring_input(options.input_path));

  double average_ms = 0.0;
  const ColoringResult result =
      cs509::run_timed(options.runs, average_ms, [&] { return welsh_powell_coloring(graph); });

  const bool valid = is_valid_coloring(graph, result.colors);
  long long max_degree = 0;
  for (int u = 0; u < graph.num_vertices; ++u) max_degree = std::max(max_degree, graph.degree(u));

  std::printf("Input: %s (V = %d, E = %lld)\n", options.input_path.c_str(), graph.num_vertices, graph.num_edges);
  std::printf("Algorithm: Greedy Vertex Coloring\n");
  print_listing_title("Vertex colors", options.quiet, graph.num_vertices, "vertices");
  if (!options.quiet) {
    for (int v = 0; v < graph.num_vertices; ++v) std::printf("%d %d\n", v, result.colors[v]);
  }
  std::printf("Colors used: %d\n", result.colors_used);
  std::printf("Max degree: %lld\n", max_degree);
  std::printf("Valid coloring: %s\n", valid ? "Yes" : "No");
  cs509::print_execution_time(average_ms, options.runs);
  return valid ? 0 : 1;
}

int run_pagerank(const cs509::RunOptions& options) {
  PageRankInput input = read_pagerank_input(options.input_path);
  const cs509::CSRGraph graph = cs509::adjacency_list_to_csr(input.graph);
  input.graph = cs509::AdjacencyList();  // only the CSR is needed from here on

  double average_ms = 0.0;
  const PageRankResult result =
      cs509::run_timed(options.runs, average_ms, [&] { return pagerank(graph, input.params); });

  const std::vector<double>& ranks = result.ranks;
  double sum = 0.0;
  int top = 0;
  long long dangling = 0;
  for (int v = 0; v < graph.num_vertices; ++v) {
    sum += ranks[v];
    if (ranks[v] > ranks[top]) top = v;  // ties keep the smaller id
    if (graph.degree(v) == 0) ++dangling;
  }

  std::printf("Input: %s (V = %d, E = %lld)\n", options.input_path.c_str(), graph.num_vertices, graph.num_edges);
  std::printf("Algorithm: PageRank\n");
  std::printf("Damping: %g\n", input.params.damping);
  print_listing_title("Vertex ranks", options.quiet, graph.num_vertices, "vertices");
  if (!options.quiet) {
    for (int v = 0; v < graph.num_vertices; ++v) std::printf("%d %.6f\n", v, ranks[v]);
  }
  std::printf("Sum of ranks: %.6f\n", sum);
  std::printf("Top vertex: %d (rank %.6f)\n", top, ranks[top]);
  std::printf("Dangling vertices: %lld\n", dangling);
  std::printf("Iterations: %lld\n", result.iterations);
  std::printf("Converged: %s\n", result.converged ? "true" : "false");
  cs509::print_execution_time(average_ms, options.runs);
  return 0;
}

int run_kmeans(const cs509::RunOptions& options) {
  const KMeansInput input = read_kmeans_input(options.input_path);

  double average_ms = 0.0;
  const KMeansResult result =
      cs509::run_timed(options.runs, average_ms, [&] { return kmeans(input.points, input.params); });

  const int n = input.points.n;
  const int d = input.points.d;
  const int k = input.params.k;
  std::printf("Input: %s (N = %d, D = %d)\n", options.input_path.c_str(), n, d);
  std::printf("Algorithm: K-Means Clustering\n");
  std::printf("K: %d\n", k);
  print_listing_title("Point assignments", options.quiet, n, "points");
  if (!options.quiet) {
    for (int i = 0; i < n; ++i) std::printf("%d %d\n", i, result.assignment[i]);
  }
  std::printf("Final centroids:\n");
  for (int c = 0; c < k; ++c) {
    std::printf("%d:", c);
    for (int j = 0; j < d; ++j) std::printf(" %.6f", result.centroids[static_cast<std::size_t>(c) * d + j]);
    std::printf("\n");
  }
  std::printf("Cluster sizes:");
  for (int c = 0; c < k; ++c) std::printf(" %lld", result.cluster_sizes[c]);
  std::printf("\n");
  std::printf("WCSS: %.6f\n", result.wcss);
  std::printf("Iterations: %lld\n", result.iterations);
  std::printf("Converged: %s\n", result.converged ? "true" : "false");
  cs509::print_execution_time(average_ms, options.runs);
  return 0;
}

int run_fastmap(const cs509::RunOptions& options) {
  const FastMapInput input = read_fastmap_input(options.input_path);

  double average_ms = 0.0;
  const FastMapResult result =
      cs509::run_timed(options.runs, average_ms, [&] { return fastmap(input.distances, input.k, options.seed); });

  const double error = average_distance_error(input.distances, result);

  const int n = result.n;
  const int k = result.k;
  std::printf("Input: %s (N = %d)\n", options.input_path.c_str(), n);
  std::printf("Algorithm: FastMap\n");
  std::printf("Target dimensions: %d\n", k);
  std::printf("Pivot strategy: random start object (seed %llu), 2 farthest-point rounds per dimension\n",
              static_cast<unsigned long long>(options.seed));
  std::printf("Pivots per dimension:\n");
  for (int c = 0; c < k; ++c) {
    if (result.pivots[c].first < 0) {
      std::printf("Dim %d: none (all residual distances are 0)\n", c + 1);
    } else {
      std::printf("Dim %d: %d %d\n", c + 1, result.pivots[c].first, result.pivots[c].second);
    }
  }
  print_listing_title("Object coordinates", options.quiet, n, "objects");
  if (!options.quiet) {
    for (int i = 0; i < n; ++i) {
      std::printf("%d:", i);
      for (int c = 0; c < k; ++c) std::printf(" %.6f", result.coords[static_cast<std::size_t>(i) * k + c]);
      std::printf("\n");
    }
  }
  std::printf("Avg. distance error: %.6f\n", error);
  cs509::print_execution_time(average_ms, options.runs);
  return 0;
}

}  // namespace a4
