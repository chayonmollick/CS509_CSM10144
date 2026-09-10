// Assignment 4: FastMap (Faloutsos & Lin, 1995) embedding of N objects from their pairwise distances.
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace a4 {

// Symmetric N x N distance matrix with a zero diagonal. Only the strict upper triangle is stored
// (N(N-1)/2 doubles, about 400 MB for N = 10,000), so large inputs never need a full N^2 matrix.
class DistanceMatrix {
 public:
  explicit DistanceMatrix(int n = 0) : n_(n) {
    // reserve() only claims address space; pages are touched as rows are actually read,
    // so a header that overstates N does not commit memory up front.
    if (n > 1) upper_.reserve(static_cast<std::size_t>(n) * static_cast<std::size_t>(n - 1) / 2);
  }

  int size() const { return n_; }

  // Appends the next upper-triangle entry in row-major order: (0,1) .. (0,N-1), (1,2) .. (1,N-1), ...
  void append_upper(double distance) { upper_.push_back(distance); }

  double at(int i, int j) const {
    if (i == j) return 0.0;
    if (i > j) std::swap(i, j);
    return upper_[index(i, j)];
  }

  // Entries (i, i+1) .. (i, N-1) are contiguous; row-wise scans read them through this pointer.
  const double* upper_row(int i) const { return upper_.data() + index(i, i + 1); }

 private:
  // Position of (i, j), i < j: rows 0..i-1 hold (N-1) + (N-2) + ... + (N-i) = i(2N-i-1)/2 entries.
  std::size_t index(int i, int j) const {
    const auto a = static_cast<std::size_t>(i);
    return a * (2 * static_cast<std::size_t>(n_) - a - 1) / 2 + static_cast<std::size_t>(j - i - 1);
  }

  int n_;
  std::vector<double> upper_;
};

struct FastMapResult {
  int n = 0;
  int k = 0;
  std::vector<double> coords;               // N*K, row-major: object i is coords[i*K] .. coords[i*K + K - 1]
  std::vector<std::pair<int, int>> pivots;  // pivot objects (a, b) per dimension; (-1, -1) if never reached
};

// Embeds the objects into k dimensions. For each dimension a start object is drawn from a splitmix64
// stream seeded with `seed`, followed by two farthest-point rounds (a = far(o), b = far(a), twice).
// If the residual distance between the pivots is 0, this and all later coordinates stay 0.
FastMapResult fastmap(const DistanceMatrix& distances, int k, std::uint64_t seed);

// Mean over all pairs i < j of |D(i,j) - ||x_i - x_j|||. O(N^2 K); used for reporting, not timed.
double average_distance_error(const DistanceMatrix& distances, const FastMapResult& embedding);

}  // namespace a4
