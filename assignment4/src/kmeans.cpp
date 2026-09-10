#include "a4/kmeans.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace a4 {

KMeansResult kmeans(const PointSet& points, const KMeansParams& params) {
  const auto n = static_cast<std::size_t>(points.n);
  const auto d = static_cast<std::size_t>(points.d);
  const auto k = static_cast<std::size_t>(params.k);
  const double* x = points.coords.data();

  KMeansResult result;
  // Deterministic seeding: the first K points in input order.
  result.centroids.assign(x, x + k * d);
  result.assignment.assign(n, 0);
  result.cluster_sizes.assign(k, 0);
  std::vector<double> sums(k * d);
  double* centroid = result.centroids.data();

  for (;;) {
    // Assignment step: nearest centroid by squared distance (no sqrt needed to compare);
    // strict '<' keeps the lower cluster index on ties. Cluster sums are accumulated on the way.
    std::fill(sums.begin(), sums.end(), 0.0);
    std::fill(result.cluster_sizes.begin(), result.cluster_sizes.end(), 0);
    for (std::size_t i = 0; i < n; ++i) {
      const double* p = x + i * d;
      std::size_t best = 0;
      double best_distance = std::numeric_limits<double>::infinity();
      for (std::size_t c = 0; c < k; ++c) {
        const double* q = centroid + c * d;
        double distance = 0.0;
        for (std::size_t j = 0; j < d; ++j) {
          const double diff = p[j] - q[j];
          distance += diff * diff;
        }
        if (distance < best_distance) {
          best_distance = distance;
          best = c;
        }
      }
      result.assignment[i] = static_cast<int>(best);
      ++result.cluster_sizes[best];
      double* sum = sums.data() + best * d;
      for (std::size_t j = 0; j < d; ++j) sum[j] += p[j];
    }

    // Update step: move each centroid to its cluster mean; an empty cluster keeps its old centroid.
    double max_shift_squared = 0.0;
    for (std::size_t c = 0; c < k; ++c) {
      if (result.cluster_sizes[c] == 0) continue;
      const auto size = static_cast<double>(result.cluster_sizes[c]);
      double* q = centroid + c * d;
      double shift_squared = 0.0;
      for (std::size_t j = 0; j < d; ++j) {
        const double updated = sums[c * d + j] / size;
        const double diff = updated - q[j];
        shift_squared += diff * diff;
        q[j] = updated;
      }
      max_shift_squared = std::max(max_shift_squared, shift_squared);
    }

    ++result.iterations;
    if (std::sqrt(max_shift_squared) <= params.tolerance) {
      result.converged = true;
      break;
    }
    if (result.iterations >= params.max_iterations) break;
  }

  // WCSS against the final centroids (they moved after the last assignment step).
  double wcss = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double* p = x + i * d;
    const double* q = centroid + static_cast<std::size_t>(result.assignment[i]) * d;
    for (std::size_t j = 0; j < d; ++j) {
      const double diff = p[j] - q[j];
      wcss += diff * diff;
    }
  }
  result.wcss = wcss;
  return result;
}

}  // namespace a4
