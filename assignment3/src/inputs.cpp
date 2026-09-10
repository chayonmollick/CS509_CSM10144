#include "a3/inputs.hpp"

#include <cstddef>
#include <cstdio>
#include <string>

#include "cs509/graph_reader.hpp"
#include "cs509/scanner.hpp"

namespace a3 {
namespace {

using std::to_string;

std::string format_number(double value) {
  char text[64];
  std::snprintf(text, sizeof(text), "%g", value);
  return text;
}

// Marks a keyword as seen, rejecting a second occurrence.
void mark_seen(cs509::Scanner& in, bool& seen, const std::string& keyword) {
  if (seen) in.fail("duplicate " + keyword + " line");
  seen = true;
}

}  // namespace

cs509::AdjacencyList read_mst_graph(const std::string& path) {
  cs509::Scanner in(path);
  cs509::AdjacencyFormat format;
  format.weighted = true;
  cs509::AdjacencyList graph = cs509::read_adjacency_list(in, format);
  cs509::expect_end_of_file(in);
  return graph;
}

FlowNetworkInput read_flow_network(const std::string& path) {
  cs509::Scanner in(path);
  cs509::AdjacencyFormat format;
  format.directed = true;
  format.weighted = true;
  format.allow_negative_weights = false;
  format.weight_name = "capacity";

  FlowNetworkInput input;
  input.graph = cs509::read_adjacency_list(in, format);

  long long source = -1;
  long long sink = -1;
  bool have_source = false;
  bool have_sink = false;
  while (in.next_line()) {
    const std::string keyword = in.read_word("keyword");
    if (keyword == "SOURCE") {
      mark_seen(in, have_source, keyword);
      source = in.read_int("SOURCE");
    } else if (keyword == "SINK") {
      mark_seen(in, have_sink, keyword);
      sink = in.read_int("SINK");
    } else {
      in.fail("unexpected '" + keyword + "'; expected SOURCE or SINK after the adjacency list");
    }
    in.expect_line_end(keyword.c_str());
  }

  const long long v = input.graph.num_vertices;
  const std::string range = "; it must be in [0, " + to_string(v - 1) + "]";
  if (!have_source) in.fail_file("missing 'SOURCE s' line");
  if (!have_sink) in.fail_file("missing 'SINK t' line");
  if (source < 0 || source >= v) in.fail_file("invalid source vertex " + to_string(source) + range);
  if (sink < 0 || sink >= v) in.fail_file("invalid sink vertex " + to_string(sink) + range);
  if (source == sink) in.fail_file("source and sink must be different vertices (both are " + to_string(source) + ")");
  input.source = static_cast<int>(source);
  input.sink = static_cast<int>(sink);
  return input;
}

GradientDescentProblem read_gradient_descent_problem(const std::string& path) {
  cs509::Scanner in(path);
  GradientDescentProblem problem;
  long long degree = 0;
  bool have_degree = false;
  bool have_coefficients = false;
  bool have_initial_x = false;
  bool have_learning_rate = false;
  bool have_tolerance = false;
  bool have_max_iterations = false;

  while (in.next_line()) {
    const std::string keyword = in.read_word("keyword");
    if (keyword == "DEGREE") {
      mark_seen(in, have_degree, keyword);
      degree = in.read_int("DEGREE");
    } else if (keyword == "COEFFICIENTS") {
      mark_seen(in, have_coefficients, keyword);
      while (in.has_more_on_line()) problem.coefficients.push_back(in.read_double("coefficient"));
    } else if (keyword == "INITIAL_X") {
      mark_seen(in, have_initial_x, keyword);
      problem.initial_x = in.read_double("INITIAL_X");
    } else if (keyword == "LEARNING_RATE") {
      mark_seen(in, have_learning_rate, keyword);
      problem.learning_rate = in.read_double("LEARNING_RATE");
    } else if (keyword == "TOLERANCE") {
      mark_seen(in, have_tolerance, keyword);
      problem.tolerance = in.read_double("TOLERANCE");
    } else if (keyword == "MAX_ITERATIONS") {
      mark_seen(in, have_max_iterations, keyword);
      problem.max_iterations = in.read_int("MAX_ITERATIONS");
    } else {
      in.fail("unknown keyword '" + keyword +
              "' (expected DEGREE, COEFFICIENTS, INITIAL_X, LEARNING_RATE, TOLERANCE or MAX_ITERATIONS)");
    }
    in.expect_line_end(keyword.c_str());
  }

  if (!have_degree) in.fail_file("missing DEGREE line");
  if (!have_coefficients) in.fail_file("missing COEFFICIENTS line");
  if (!have_initial_x) in.fail_file("missing INITIAL_X line");
  if (!have_learning_rate) in.fail_file("missing LEARNING_RATE line");
  if (!have_tolerance) in.fail_file("missing TOLERANCE line");
  if (!have_max_iterations) in.fail_file("missing MAX_ITERATIONS line");

  if (degree < 1) in.fail_file("invalid DEGREE " + to_string(degree) + "; the degree must be a positive integer");
  if (problem.coefficients.size() != static_cast<std::size_t>(degree) + 1) {
    in.fail_file("DEGREE " + to_string(degree) + " needs exactly " + to_string(degree + 1) +
                 " coefficients (c0 .. cd), but COEFFICIENTS lists " + to_string(problem.coefficients.size()));
  }
  if (problem.learning_rate <= 0.0) {
    in.fail_file("LEARNING_RATE must be positive, found " + format_number(problem.learning_rate));
  }
  if (problem.tolerance <= 0.0) in.fail_file("TOLERANCE must be positive, found " + format_number(problem.tolerance));
  if (problem.max_iterations <= 0) {
    in.fail_file("MAX_ITERATIONS must be positive, found " + to_string(problem.max_iterations));
  }
  return problem;
}

}  // namespace a3
