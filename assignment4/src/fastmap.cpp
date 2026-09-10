#include "a4/fastmap.hpp"

#include <cmath>

namespace a4 {
namespace {

// splitmix64 (Steele, Lea & Flood): tiny, well mixed and identical on every platform and compiler,
// so a given --seed always selects the same pivots (std::uniform_int_distribution does not promise that).
std::uint64_t splitmix64(std::uint64_t& state) {
  std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// Squared distance between objects i and j in the residual space of dimension c:
//   D_c(i,j)^2 = D(i,j)^2 - sum_{m<c} (x_i[m] - x_j[m])^2, clamped at 0 against rounding.
// This is exactly the paper's recursive update D'(i,j)^2 = D(i,j)^2 - (x_i - x_j)^2 applied after
// each finished dimension (equivalent to deflating the matrix), but computed on demand from the
// original matrix and the coordinates found so far. FastMap only looks at O(N) pairs per dimension,
// so this costs O(N K^2) in total instead of O(N^2) rewrites per dimension or a second N x N matrix.
double residual_squared(const DistanceMatrix& distances, const double* coords, std::size_t k, int i, int j,
                        std::size_t c) {
  if (i == j) return 0.0;
  const double original = distances.at(i, j);
  double value = original * original;
  const double* xi = coords + static_cast<std::size_t>(i) * k;
  const double* xj = coords + static_cast<std::size_t>(j) * k;
  for (std::size_t m = 0; m < c; ++m) {
    const double diff = xi[m] - xj[m];
    value -= diff * diff;
  }
  return value > 0.0 ? value : 0.0;
}

// Object with the largest residual distance from `from`; ties go to the smallest index.
int farthest(const DistanceMatrix& distances, const double* coords, std::size_t k, int from, std::size_t c) {
  int best = 0;
  double best_squared = -1.0;
  for (int i = 0; i < distances.size(); ++i) {
    const double squared = residual_squared(distances, coords, k, from, i, c);
    if (squared > best_squared) {
      best_squared = squared;
      best = i;
    }
  }
  return best;
}

}  // namespace

FastMapResult fastmap(const DistanceMatrix& distances, int k, std::uint64_t seed) {
  const int n = distances.size();
  const auto dims = static_cast<std::size_t>(k);
  FastMapResult result;
  result.n = n;
  result.k = k;
  result.coords.assign(static_cast<std::size_t>(n) * dims, 0.0);
  result.pivots.assign(dims, std::make_pair(-1, -1));
  const double* x = result.coords.data();  // columns < c are final while column c is written
  std::uint64_t state = seed;

  for (std::size_t c = 0; c < dims; ++c) {
    // Choose-distant-objects heuristic: random start, then two farthest-point rounds.
    const int start = static_cast<int>(splitmix64(state) % static_cast<std::uint64_t>(n));
    int a = farthest(distances, x, dims, start, c);
    int b = farthest(distances, x, dims, a, c);
    a = farthest(distances, x, dims, b, c);
    b = farthest(distances, x, dims, a, c);
    result.pivots[c] = std::make_pair(a, b);

    const double ab_squared = residual_squared(distances, x, dims, a, b, c);
    if (ab_squared <= 0.0) break;  // every residual distance is 0: remaining coordinates stay 0

    // Law of cosines: projection of object i onto the line through the pivots.
    const double two_ab = 2.0 * std::sqrt(ab_squared);
    for (int i = 0; i < n; ++i) {
      const double ai_squared = residual_squared(distances, x, dims, a, i, c);
      const double bi_squared = residual_squared(distances, x, dims, b, i, c);
      result.coords[static_cast<std::size_t>(i) * dims + c] = (ai_squared + ab_squared - bi_squared) / two_ab;
    }
  }
  return result;
}

double average_distance_error(const DistanceMatrix& distances, const FastMapResult& embedding) {
  const int n = distances.size();
  if (n < 2) return 0.0;
  const auto k = static_cast<std::size_t>(embedding.k);
  const double* x = embedding.coords.data();

  double total = 0.0;
  for (int i = 0; i + 1 < n; ++i) {
    const double* row = distances.upper_row(i);  // row[j - i - 1] = D(i, j)
    const double* xi = x + static_cast<std::size_t>(i) * k;
    for (int j = i + 1; j < n; ++j) {
      const double* xj = x + static_cast<std::size_t>(j) * k;
      double squared = 0.0;
      for (std::size_t m = 0; m < k; ++m) {
        const double diff = xi[m] - xj[m];
        squared += diff * diff;
      }
      total += std::fabs(row[j - i - 1] - std::sqrt(squared));
    }
  }
  return total / (static_cast<double>(n) * (n - 1) / 2.0);
}

}  // namespace a4
