// Options collected by the common wrapper (menu or command line) and passed to every runner.
#pragma once

#include <cstdint>
#include <string>

namespace cs509 {

struct RunOptions {
  std::string input_path;
  int runs = 1;             // timed repetitions of the algorithm call; the reported time is the average
  bool quiet = false;       // print summaries only (skip per-vertex / per-edge listings)
  std::uint64_t seed = 1;   // seed for randomized steps (FastMap pivot selection)
};

// A runner reads its input, prepares the data structure, times the algorithm and prints the result.
// It returns the process exit status and throws InputError for invalid input.
using Runner = int (*)(const RunOptions&);

}  // namespace cs509
