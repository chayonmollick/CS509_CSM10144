// Assignment 4 input readers: parse and validate the four file formats before any timing starts.
// Every reader throws cs509::InputError with the file name and line number on invalid input.
#pragma once

#include <string>

#include "a4/fastmap.hpp"
#include "a4/kmeans.hpp"
#include "a4/pagerank.hpp"
#include "cs509/csr.hpp"

namespace a4 {

// "V E" header and undirected, unweighted adjacency lines. Self-loops, parallel edges, out-of-range
// ids, degree/neighbour-count mismatches and one-sided edges are rejected by the common reader.
cs509::AdjacencyList read_coloring_input(const std::string& path);

struct PageRankInput {
  cs509::AdjacencyList graph;
  PageRankParams params;
};

// Directed adjacency list followed by DAMPING, TOLERANCE and MAX_ITERATIONS lines (any order, each once).
PageRankInput read_pagerank_input(const std::string& path);

struct KMeansInput {
  PointSet points;
  KMeansParams params;
};

// "N D K", N lines of exactly D numbers, then MAX_ITERATIONS and TOLERANCE lines (any order, each once).
KMeansInput read_kmeans_input(const std::string& path);

struct FastMapInput {
  DistanceMatrix distances;
  int k = 0;
};

// "N K" followed by N rows of N non-negative distances forming a symmetric matrix with a zero diagonal.
FastMapInput read_fastmap_input(const std::string& path);

}  // namespace a4
