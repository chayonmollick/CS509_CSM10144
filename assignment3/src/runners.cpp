#include "a3/runners.hpp"

#include <cstddef>
#include <cstdio>

#include "a3/gradient_descent.hpp"
#include "a3/inputs.hpp"
#include "a3/maxflow.hpp"
#include "a3/mst.hpp"
#include "cs509/csr.hpp"
#include "cs509/timer.hpp"

namespace a3 {
namespace {

void print_graph_header(const cs509::RunOptions& options, const cs509::CSRGraph& graph) {
  std::printf("Input: %s (V = %d, E = %lld)\n", options.input_path.c_str(), graph.num_vertices, graph.num_edges);
}

// Reads the MST adjacency list and converts it with the Assignment 2 helper (preprocessing, never timed).
cs509::CSRGraph load_mst_graph(const cs509::RunOptions& options) {
  const cs509::AdjacencyList list = read_mst_graph(options.input_path);
  return cs509::adjacency_list_to_csr(list);
}

void print_mst(const char* name, const MSTResult& mst, const cs509::CSRGraph& graph, double ms,
               const cs509::RunOptions& options) {
  std::printf("Algorithm: %s\n", name);
  if (options.quiet) {
    std::printf("MST edges: %zu (listing suppressed by --quiet)\n", mst.edges.size());
  } else {
    std::printf("MST edges:\n");
    for (const MSTEdge& edge : mst.edges) std::printf("%d %d %lld\n", edge.u, edge.v, edge.weight);
  }
  if (mst.spanning) {
    std::printf("Total MST weight: %lld\n", mst.total_weight);
  } else {
    std::printf("Total MST weight: undefined - the graph is disconnected (%zu of %d tree edges found)\n",
                mst.edges.size(), graph.num_vertices - 1);
  }
  cs509::print_execution_time(ms, options.runs);
}

int report_disconnected(const cs509::RunOptions& options) {
  std::fflush(stdout);
  std::fprintf(stderr, "Error: %s: the graph is not connected, so it has no spanning tree\n",
               options.input_path.c_str());
  return 1;
}

void print_vertex_side(const char* label, const std::vector<char>& source_side, char side) {
  std::printf("%s", label);
  for (std::size_t v = 0; v < source_side.size(); ++v) {
    if (source_side[v] == side) std::printf(" %zu", v);
  }
  std::printf("\n");
}

}  // namespace

int run_kruskal(const cs509::RunOptions& options) {
  const cs509::CSRGraph graph = load_mst_graph(options);
  print_graph_header(options, graph);
  double ms = 0.0;
  const MSTResult mst = cs509::run_timed(options.runs, ms, [&] { return kruskal_mst(graph); });
  print_mst("Kruskal's MST", mst, graph, ms, options);
  return mst.spanning ? 0 : report_disconnected(options);
}

int run_prim(const cs509::RunOptions& options) {
  const cs509::CSRGraph graph = load_mst_graph(options);
  print_graph_header(options, graph);
  double ms = 0.0;
  const MSTResult mst = cs509::run_timed(options.runs, ms, [&] { return prim_mst(graph, 0); });
  print_mst("Prim's MST", mst, graph, ms, options);
  return mst.spanning ? 0 : report_disconnected(options);
}

int run_mst_compare(const cs509::RunOptions& options) {
  const cs509::CSRGraph graph = load_mst_graph(options);
  print_graph_header(options, graph);
  double kruskal_ms = 0.0;
  double prim_ms = 0.0;
  const MSTResult kruskal = cs509::run_timed(options.runs, kruskal_ms, [&] { return kruskal_mst(graph); });
  const MSTResult prim = cs509::run_timed(options.runs, prim_ms, [&] { return prim_mst(graph, 0); });

  print_mst("Kruskal's MST", kruskal, graph, kruskal_ms, options);
  std::printf("\n");
  print_mst("Prim's MST", prim, graph, prim_ms, options);
  if (!kruskal.spanning || !prim.spanning) return report_disconnected(options);

  const bool equal = kruskal.total_weight == prim.total_weight;
  std::printf("\nComparison: Kruskal weight %lld, Prim weight %lld, equal: %s\n", kruskal.total_weight,
              prim.total_weight, equal ? "Yes" : "No");
  return equal ? 0 : 1;
}

int run_gradient_descent(const cs509::RunOptions& options) {
  const GradientDescentProblem problem = read_gradient_descent_problem(options.input_path);
  std::printf("Input: %s\n", options.input_path.c_str());
  double ms = 0.0;
  const GradientDescentResult result = cs509::run_timed(options.runs, ms, [&] { return gradient_descent(problem); });

  std::printf("Algorithm: Gradient Descent\n");
  std::printf("Degree: %d\n", problem.degree());
  std::printf("Final x: %.12f\n", result.x);
  std::printf("Final f(x): %.12f\n", result.fx);
  std::printf("Final f'(x): %.3e\n", result.gradient);
  std::printf("Iterations: %lld\n", result.iterations);
  std::printf("Converged: %s\n", result.converged ? "true" : "false");
  if (result.diverged) std::printf("Note: the iterates stopped being finite (learning rate too large?)\n");
  cs509::print_execution_time(ms, options.runs);
  return 0;
}

int run_maxflow(const cs509::RunOptions& options) {
  FlowNetworkInput input = read_flow_network(options.input_path);
  const cs509::CSRGraph network = cs509::adjacency_list_to_csr(input.graph);  // preprocessing, never timed
  input.graph = cs509::AdjacencyList();                                        // only CSR is used from here on
  print_graph_header(options, network);

  double ms = 0.0;
  const MaxFlowResult result =
      cs509::run_timed(options.runs, ms, [&] { return dinic_max_flow(network, input.source, input.sink); });
  const bool equal = result.max_flow == result.cut_capacity;

  std::printf("Algorithm: Maxflow-Mincut\n");
  std::printf("Source: %d\n", input.source);
  std::printf("Sink: %d\n", input.sink);
  std::printf("Maximum flow: %lld\n", result.max_flow);
  std::printf("Minimum cut capacity: %lld\n", result.cut_capacity);
  std::printf("Flow equals cut capacity: %s\n", equal ? "Yes" : "No");
  if (options.quiet) {
    std::size_t source_count = 0;
    for (char side : result.source_side) source_count += side ? 1 : 0;
    std::printf("Source side: %zu vertices (listing suppressed by --quiet)\n", source_count);
    std::printf("Sink side: %zu vertices (listing suppressed by --quiet)\n", result.source_side.size() - source_count);
    std::printf("Cut edges: %zu (listing suppressed by --quiet)\n", result.cut_edges.size());
  } else {
    print_vertex_side("Source side:", result.source_side, 1);
    print_vertex_side("Sink side:", result.source_side, 0);
    std::printf("Cut edges:\n");
    for (const CutEdge& edge : result.cut_edges) std::printf("%d %d %lld\n", edge.u, edge.v, edge.capacity);
  }
  cs509::print_execution_time(ms, options.runs);
  return equal ? 0 : 1;
}

}  // namespace a3
