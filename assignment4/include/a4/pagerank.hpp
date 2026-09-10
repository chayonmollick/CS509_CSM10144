// Assignment 4: PageRank by power iteration on a directed CSR graph.
#pragma once

#include <vector>

#include "cs509/csr.hpp"

namespace a4 {

struct PageRankParams {
  double damping = 0.85;           // d, strictly between 0 and 1
  double tolerance = 1e-4;         // stop once the L1 change between iterations is <= tolerance
  long long max_iterations = 100;  // stop after this many iterations even if not converged
};

struct PageRankResult {
  std::vector<double> ranks;
  long long iterations = 0;
  bool converged = false;
  double final_change = 0.0;  // L1 distance between the last two rank vectors
};

// PR(v) = (1-d)/N + d * dangling_mass/N + d * sum_{u->v} PR(u)/outdeg(u), starting from PR = 1/N.
// Vertices without out-arcs spread their rank evenly over all vertices, so the ranks keep summing to 1.
// Every iteration is computed only from the previous one (Jacobi style, two arrays).
PageRankResult pagerank(const cs509::CSRGraph& graph, const PageRankParams& params);

}  // namespace a4
