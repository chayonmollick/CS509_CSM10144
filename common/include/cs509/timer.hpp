// Timing helpers: only the algorithm call itself is measured.
#pragma once

#include <chrono>
#include <cstdio>
#include <optional>
#include <utility>

namespace cs509 {

// Calls `algorithm` `runs` (>= 1) times and returns the result of the last run.
// The clock is started immediately before and stopped immediately after each call, so
// file reading, CSR conversion, verification and printing are never included.
// `average_ms` receives the mean time per call in milliseconds.
template <typename Algorithm>
auto run_timed(int runs, double& average_ms, Algorithm&& algorithm) -> decltype(algorithm()) {
  using Clock = std::chrono::steady_clock;
  std::optional<decltype(algorithm())> result;
  double total_ms = 0.0;
  for (int run = 0; run < runs; ++run) {
    result.reset();  // release the previous run's output before timing the next one
    const auto start = Clock::now();
    result.emplace(algorithm());
    const auto stop = Clock::now();
    total_ms += std::chrono::duration<double, std::milli>(stop - start).count();
  }
  average_ms = total_ms / runs;
  return std::move(*result);
}

inline void print_execution_time(double average_ms, int runs) {
  if (runs == 1) {
    std::printf("Execution time: %.6f ms\n", average_ms);
  } else {
    std::printf("Execution time: %.6f ms (average of %d runs)\n", average_ms, runs);
  }
}

}  // namespace cs509
