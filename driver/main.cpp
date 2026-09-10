// CS509 common wrapper (Assignment 1).
// Selects an algorithm and an input file from the command line or an interactive menu and
// dispatches to the assignment runner, which reads the file, builds CSR where required,
// times only the algorithm call and prints the result.
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>
#include <string>

#include "a3/runners.hpp"
#include "a4/runners.hpp"
#include "cs509/options.hpp"
#include "cs509/scanner.hpp"

namespace {

struct Algorithm {
  const char* name;
  const char* assignment;
  const char* description;
  cs509::Runner run;
};

const Algorithm kAlgorithms[] = {
    {"kruskal", "A3", "Minimum Spanning Tree - Kruskal", a3::run_kruskal},
    {"prim", "A3", "Minimum Spanning Tree - Prim", a3::run_prim},
    {"mst", "A3", "Kruskal and Prim on the same graph (comparison)", a3::run_mst_compare},
    {"gd", "A3", "Gradient Descent on a polynomial", a3::run_gradient_descent},
    {"maxflow", "A3", "Maxflow-Mincut (Dinic)", a3::run_maxflow},
    {"coloring", "A4", "Greedy Vertex Coloring (Welsh-Powell order)", a4::run_coloring},
    {"pagerank", "A4", "PageRank", a4::run_pagerank},
    {"kmeans", "A4", "K-Means Clustering", a4::run_kmeans},
    {"fastmap", "A4", "FastMap embedding", a4::run_fastmap},
};
const int kAlgorithmCount = static_cast<int>(sizeof(kAlgorithms) / sizeof(kAlgorithms[0]));

const Algorithm* find_algorithm(const std::string& name) {
  for (const Algorithm& algorithm : kAlgorithms) {
    if (name == algorithm.name) return &algorithm;
  }
  return nullptr;
}

void print_algorithms() {
  for (int i = 0; i < kAlgorithmCount; ++i) {
    std::printf("  %d. %-9s [%s] %s\n", i + 1, kAlgorithms[i].name, kAlgorithms[i].assignment,
                kAlgorithms[i].description);
  }
}

void print_usage(const char* program) {
  std::printf("Usage:\n"
              "  %s                                     interactive menu\n"
              "  %s <algorithm> <input-file> [options]\n"
              "  %s --list | --help\n\n"
              "Options:\n"
              "  --runs N   repeat the timed algorithm call N times and report the average time\n"
              "  --quiet    print summaries only (skip per-vertex / per-edge listings)\n"
              "  --seed S   seed for randomized steps (FastMap pivot selection), default 1\n\n"
              "Algorithms:\n",
              program, program, program);
  print_algorithms();
}

int dispatch(const Algorithm& algorithm, const cs509::RunOptions& options) {
  try {
    const int status = algorithm.run(options);
    std::fflush(stdout);
    return status;
  } catch (const cs509::InputError& error) {
    std::fflush(stdout);
    std::fprintf(stderr, "Error: %s\n", error.what());
  } catch (const std::bad_alloc&) {
    std::fflush(stdout);
    std::fprintf(stderr, "Error: out of memory while running '%s' on '%s'\n", algorithm.name,
                 options.input_path.c_str());
  } catch (const std::exception& error) {
    std::fflush(stdout);
    std::fprintf(stderr, "Error: %s\n", error.what());
  }
  return 1;
}

bool parse_count(const std::string& text, long long max, long long& value) {
  if (text.empty() || text.size() > 18) return false;
  value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  return value >= 1 && value <= max;
}

bool parse_seed(const std::string& text, std::uint64_t& seed) {
  if (text.empty() || text[0] == '-') return false;
  char* end = nullptr;
  seed = std::strtoull(text.c_str(), &end, 10);
  return *end == '\0';
}

int run_command_line(int argc, char** argv) {
  const std::string first = argv[1];
  if (first == "--help" || first == "-h") {
    print_usage(argv[0]);
    return 0;
  }
  if (first == "--list") {
    print_algorithms();
    return 0;
  }
  const Algorithm* algorithm = find_algorithm(first);
  if (algorithm == nullptr) {
    std::fprintf(stderr, "Error: unknown algorithm '%s' (run '%s --list' to see the choices)\n", first.c_str(), argv[0]);
    return 2;
  }
  if (argc < 3) {
    std::fprintf(stderr, "Error: missing input file for '%s' (run '%s --help' for usage)\n", algorithm->name, argv[0]);
    return 2;
  }

  cs509::RunOptions options;
  options.input_path = argv[2];
  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--quiet" || arg == "-q") {
      options.quiet = true;
    } else if (arg == "--runs" || arg == "--seed") {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "Error: %s needs a value\n", arg.c_str());
        return 2;
      }
      const std::string value = argv[++i];
      long long runs = 0;
      if (arg == "--runs" && parse_count(value, 1000000, runs)) {
        options.runs = static_cast<int>(runs);
      } else if (arg == "--seed" && parse_seed(value, options.seed)) {
        // parsed into options.seed
      } else {
        std::fprintf(stderr, "Error: invalid value '%s' for %s\n", value.c_str(), arg.c_str());
        return 2;
      }
    } else {
      std::fprintf(stderr, "Error: unknown option '%s' (run '%s --help' for usage)\n", arg.c_str(), argv[0]);
      return 2;
    }
  }
  return dispatch(*algorithm, options);
}

// Reads one line from stdin with surrounding whitespace and quotes removed; false at end of input.
bool prompt(const char* message, std::string& answer) {
  std::printf("%s", message);
  std::fflush(stdout);
  if (!std::getline(std::cin, answer)) return false;
  const auto first = answer.find_first_not_of(" \t\r\"'");
  const auto last = answer.find_last_not_of(" \t\r\"'");
  answer = first == std::string::npos ? std::string() : answer.substr(first, last - first + 1);
  return true;
}

int run_menu() {
  for (;;) {
    std::printf("\n===== CS509 Algorithms (Assignments 3 and 4) =====\n");
    print_algorithms();
    std::printf("  0. Exit\n");

    std::string choice;
    if (!prompt("Select an algorithm (number or name): ", choice) || choice == "0" || choice == "exit") return 0;
    long long number = 0;
    const Algorithm* algorithm =
        parse_count(choice, kAlgorithmCount, number) ? &kAlgorithms[number - 1] : find_algorithm(choice);
    if (algorithm == nullptr) {
      std::printf("Invalid choice '%s'.\n", choice.c_str());
      continue;
    }

    cs509::RunOptions options;
    if (!prompt("Input file path: ", options.input_path)) return 0;
    if (options.input_path.empty()) {
      std::printf("No input file given.\n");
      continue;
    }
    std::string answer;
    if (!prompt("Timed runs [1]: ", answer)) return 0;
    if (!answer.empty()) {
      if (parse_count(answer, 1000000, number)) {
        options.runs = static_cast<int>(number);
      } else {
        std::printf("Invalid run count '%s'; using 1.\n", answer.c_str());
      }
    }
    if (!prompt("Summary only, without per-vertex/edge listings? [y/N]: ", answer)) return 0;
    options.quiet = answer == "y" || answer == "Y" || answer == "yes";

    std::printf("\n");
    dispatch(*algorithm, options);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return run_menu();
  return run_command_line(argc, argv);
}
