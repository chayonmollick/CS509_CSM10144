// Assignment 4: greedy vertex coloring with Welsh-Powell vertex ordering.
#pragma once

#include <vector>

#include "cs509/csr.hpp"

namespace a4 {

struct ColoringResult {
  std::vector<int> colors;  // colors[v] is v's 0-based color
  int colors_used = 0;
};

// Colors an undirected CSR graph: vertices are visited by non-increasing degree (ties: smaller id
// first) and each takes the smallest color not used by an already-colored neighbour. O(V + E).
ColoringResult welsh_powell_coloring(const cs509::CSRGraph& graph);

// True when every vertex has a color and no edge joins two vertices of the same color.
bool is_valid_coloring(const cs509::CSRGraph& graph, const std::vector<int>& colors);

}  // namespace a4
