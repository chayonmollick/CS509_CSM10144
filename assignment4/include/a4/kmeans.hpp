// Assignment 4: K-Means clustering with Lloyd's algorithm.
#pragma once

#include <vector>

namespace a4 {

// N points of dimension D stored row-major: point i is coords[i*D] .. coords[i*D + D - 1].
// One flat array keeps the points contiguous, which matters for the distance loops.
struct PointSet {
  int n = 0;
  int d = 0;
  std::vector<double> coords;
};

struct KMeansParams {
  int k = 1;
  long long max_iterations = 300;
  double tolerance = 1e-4;  // converged once no centroid moves farther than this (Euclidean)
};

struct KMeansResult {
  std::vector<int> assignment;           // cluster index of each point
  std::vector<double> centroids;         // K*D, row-major
  std::vector<long long> cluster_sizes;  // points per cluster in the final assignment
  double wcss = 0.0;                     // sum of squared distances from each point to its final centroid
  long long iterations = 0;
  bool converged = false;
};

// Lloyd's algorithm seeded with the first K points (input order). Each iteration assigns every point
// to its nearest centroid (squared Euclidean distance, ties to the lower index), then moves each
// centroid to the mean of its points; an empty cluster keeps its previous centroid.
KMeansResult kmeans(const PointSet& points, const KMeansParams& params);

}  // namespace a4
