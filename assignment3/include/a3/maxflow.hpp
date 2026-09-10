// Maximum flow and minimum s-t cut on a directed capacity CSR graph (Dinic's algorithm).
#pragma once

#include <vector>

#include "cs509/csr.hpp"

namespace a3 {

struct CutEdge {
  int u;
  int v;
  long long capacity;
};

struct MaxFlowResult {
  long long max_flow = 0;
  long long cut_capacity = 0;
  std::vector<char> source_side;   // 1 for vertices reachable from the source in the final residual graph
  std::vector<CutEdge> cut_edges;  // original edges from the source side to the sink side
};

// Builds the residual network from CSR, runs Dinic (BFS level graph + blocking flow), then extracts
// the minimum cut from the final residual graph. All three steps belong to the timed algorithm.
MaxFlowResult dinic_max_flow(const cs509::CSRGraph& network, int source, int sink);

}  // namespace a3
