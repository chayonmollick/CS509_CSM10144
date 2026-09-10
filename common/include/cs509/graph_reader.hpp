// Adjacency-list file reader shared by every graph algorithm (MST, Maxflow, Coloring, PageRank).
#pragma once

#include "cs509/csr.hpp"
#include "cs509/scanner.hpp"

namespace cs509 {

// Which variant of the "V E" + "u degree neighbour [weight] ..." format a file uses.
struct AdjacencyFormat {
  bool directed = false;               // directed: only outgoing edges are listed and E counts arcs
  bool weighted = false;               // weighted: every neighbour id is followed by an integer weight
  bool allow_negative_weights = true;  // false for capacities
  bool allow_parallel_edges = true;    // undirected only: may a vertex pair be connected twice?
  const char* weight_name = "weight";  // used in error messages, e.g. "capacity"
};

// Reads the "V E" header and exactly V vertex lines (in any order, each vertex once).
// Rejects: out-of-range ids, a degree that differs from the number of listed neighbours,
// missing weights, negative weights when not allowed, and for undirected graphs self-loops,
// edges not listed at both endpoints with the same weight, and (optionally) parallel edges.
// If the header's E differs from the listed edges, a warning is printed and the listed edges are used.
// The scanner is left on the last vertex line so callers can read trailing keyword lines.
AdjacencyList read_adjacency_list(Scanner& in, const AdjacencyFormat& format);

// Fails if the file contains another non-blank line.
void expect_end_of_file(Scanner& in);

}  // namespace cs509
