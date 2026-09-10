// Readers and validators for the Assignment 3 input files.
#pragma once

#include <string>

#include "a3/gradient_descent.hpp"
#include "cs509/csr.hpp"

namespace a3 {

// Weighted undirected adjacency list shared by Kruskal and Prim (weights may be negative or zero).
cs509::AdjacencyList read_mst_graph(const std::string& path);

// Directed capacity adjacency list followed by "SOURCE s" and "SINK t" lines.
struct FlowNetworkInput {
  cs509::AdjacencyList graph;
  int source = -1;
  int sink = -1;
};
FlowNetworkInput read_flow_network(const std::string& path);

// DEGREE, COEFFICIENTS, INITIAL_X, LEARNING_RATE, TOLERANCE and MAX_ITERATIONS lines (any order).
GradientDescentProblem read_gradient_descent_problem(const std::string& path);

}  // namespace a3
