#include "a4/inputs.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

#include "cs509/graph_reader.hpp"
#include "cs509/scanner.hpp"

namespace a4 {
namespace {

using cs509::Scanner;
using std::to_string;

const long long kIntMax = std::numeric_limits<int>::max();

std::string format_number(double value) {
  char text[64];
  std::snprintf(text, sizeof(text), "%g", value);
  return text;
}

bool looks_numeric(const std::string& word) {
  const char c = word.empty() ? '\0' : word[0];
  return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';
}

// Same acceptance rules as Scanner::read_double, for a token that was already read as a word.
double parse_double(const Scanner& in, const std::string& token, const char* what) {
  char* end = nullptr;
  const double value = std::strtod(token.c_str(), &end);
  if (token.empty() || end != token.c_str() + token.size()) {
    in.fail(std::string("expected a number for ") + what + ", found '" + token + "'");
  }
  if (!std::isfinite(value)) in.fail(std::string("value for ") + what + " is not a finite number: '" + token + "'");
  return value;
}

long long read_max_iterations(Scanner& in) {
  const long long value = in.read_int("MAX_ITERATIONS");
  if (value <= 0) in.fail("MAX_ITERATIONS must be a positive integer, found " + to_string(value));
  return value;
}

double read_tolerance(Scanner& in) {
  const double value = in.read_double("TOLERANCE");
  if (!(value > 0.0)) in.fail("TOLERANCE must be positive, found " + format_number(value));
  return value;
}

// Reads "KEYWORD value" lines up to the end of the file. Every name in `names` must appear exactly
// once, in any order; read_value(index) parses and validates the value of names[index].
// `numeric_hint` explains a line that starts with a number (usually one data line too many).
template <typename ReadValue>
void read_keyword_lines(Scanner& in, const std::vector<const char*>& names, const std::string& numeric_hint,
                        ReadValue&& read_value) {
  std::string expected;
  for (const char* name : names) expected += (expected.empty() ? "" : ", ") + std::string(name);

  std::vector<char> seen(names.size(), 0);
  while (in.next_line()) {
    const std::string word = in.read_word("keyword");
    std::size_t index = 0;
    while (index < names.size() && word != names[index]) ++index;
    if (index == names.size()) {
      if (looks_numeric(word)) {
        in.fail("expected a keyword line (" + expected + ") but found the number '" + word + "'; " + numeric_hint);
      }
      in.fail("unknown keyword '" + word + "' (expected " + expected + ")");
    }
    if (seen[index]) in.fail("duplicate " + word + " line (each keyword must appear exactly once)");
    seen[index] = 1;
    read_value(index);
    in.expect_line_end(("the " + word + " line").c_str());
  }
  for (std::size_t index = 0; index < names.size(); ++index) {
    if (!seen[index]) in.fail_file("missing " + std::string(names[index]) + " line");
  }
}

}  // namespace

cs509::AdjacencyList read_coloring_input(const std::string& path) {
  Scanner in(path);
  cs509::AdjacencyFormat format;
  format.directed = false;
  format.weighted = false;
  format.allow_parallel_edges = false;
  cs509::AdjacencyList graph = cs509::read_adjacency_list(in, format);
  cs509::expect_end_of_file(in);
  return graph;
}

PageRankInput read_pagerank_input(const std::string& path) {
  Scanner in(path);
  cs509::AdjacencyFormat format;
  format.directed = true;  // self-loops and parallel arcs are legal links
  format.weighted = false;

  PageRankInput input;
  input.graph = cs509::read_adjacency_list(in, format);
  const std::string hint = "is there a vertex line beyond V = " + to_string(input.graph.num_vertices) + "?";
  read_keyword_lines(in, {"DAMPING", "TOLERANCE", "MAX_ITERATIONS"}, hint, [&](std::size_t index) {
    if (index == 0) {
      const double damping = in.read_double("DAMPING");
      if (!(damping > 0.0 && damping < 1.0)) {
        in.fail("DAMPING must be strictly between 0 and 1, found " + format_number(damping));
      }
      input.params.damping = damping;
    } else if (index == 1) {
      input.params.tolerance = read_tolerance(in);
    } else {
      input.params.max_iterations = read_max_iterations(in);
    }
  });
  return input;
}

KMeansInput read_kmeans_input(const std::string& path) {
  Scanner in(path);
  if (!in.next_line()) in.fail("file is empty; expected the header line 'N D K'");
  const long long n = in.read_int("N (number of points)");
  const long long d = in.read_int("D (dimension)");
  const long long k = in.read_int("K (number of clusters)");
  in.expect_line_end("the header line 'N D K'");
  if (n <= 0) in.fail("N must be positive, found " + to_string(n));
  if (d <= 0) in.fail("D must be positive, found " + to_string(d));
  if (k <= 0) in.fail("K must be positive, found " + to_string(k));
  if (n >= kIntMax) in.fail("N is too large: " + to_string(n));
  if (d >= kIntMax) in.fail("D is too large: " + to_string(d));
  if (k > n) in.fail("K = " + to_string(k) + " is larger than the number of points N = " + to_string(n));

  KMeansInput input;
  input.points.n = static_cast<int>(n);
  input.points.d = static_cast<int>(d);
  input.params.k = static_cast<int>(k);
  std::vector<double>& coords = input.points.coords;
  coords.reserve(static_cast<std::size_t>(std::min(n * d, 1LL << 24)));  // grows further if the data is really there

  for (long long i = 0; i < n; ++i) {
    if (!in.next_line()) {
      in.fail_file("expected " + to_string(n) + " point lines but found only " + to_string(i));
    }
    // The first token is read as a word so that reaching a keyword line early gives a clear message.
    const std::string first = in.read_word("point coordinate");
    if (first == "MAX_ITERATIONS" || first == "TOLERANCE") {
      in.fail("expected " + to_string(n) + " point lines but found only " + to_string(i) + " before the " + first +
              " line");
    }
    coords.push_back(parse_double(in, first, "point coordinate"));
    for (long long j = 1; j < d; ++j) {
      if (!in.has_more_on_line()) {
        in.fail("point " + to_string(i) + " has " + to_string(j) + " coordinate(s) but D = " + to_string(d));
      }
      coords.push_back(in.read_double("point coordinate"));
    }
    if (in.has_more_on_line()) {
      in.fail("point " + to_string(i) + " has more than D = " + to_string(d) + " coordinates");
    }
  }

  const std::string hint = "the file has more than N = " + to_string(n) + " point lines";
  read_keyword_lines(in, {"MAX_ITERATIONS", "TOLERANCE"}, hint, [&](std::size_t index) {
    if (index == 0) {
      input.params.max_iterations = read_max_iterations(in);
    } else {
      input.params.tolerance = read_tolerance(in);
    }
  });
  return input;
}

FastMapInput read_fastmap_input(const std::string& path) {
  Scanner in(path);
  if (!in.next_line()) in.fail("file is empty; expected the header line 'N K'");
  const long long n = in.read_int("N (number of objects)");
  const long long k = in.read_int("K (target dimensions)");
  in.expect_line_end("the header line 'N K'");
  if (n <= 0) in.fail("N must be positive, found " + to_string(n));
  if (n >= kIntMax) in.fail("N is too large: " + to_string(n));
  if (k <= 0 || k >= n) in.fail("K must satisfy 0 < K < N = " + to_string(n) + ", found K = " + to_string(k));

  FastMapInput input;
  input.k = static_cast<int>(k);
  input.distances = DistanceMatrix(static_cast<int>(n));
  DistanceMatrix& matrix = input.distances;

  // Row i is checked as it streams in: entries right of the diagonal are stored, entries left of
  // it are compared with the stored mirror, so the full N x N matrix never exists in memory.
  for (long long i = 0; i < n; ++i) {
    if (!in.next_line()) {
      in.fail_file("expected " + to_string(n) + " matrix rows but found only " + to_string(i) +
                   " (the distance matrix must be N x N)");
    }
    for (long long j = 0; j < n; ++j) {
      if (!in.has_more_on_line()) {
        in.fail("row " + to_string(i) + " has " + to_string(j) + " value(s) but N = " + to_string(n) +
                " (the distance matrix must be square)");
      }
      const double value = in.read_double("distance");
      if (value < 0.0) {
        in.fail("negative distance " + format_number(value) + " at d(" + to_string(i) + ", " + to_string(j) + ")");
      }
      if (j > i) {
        matrix.append_upper(value);
      } else if (j == i) {
        if (value != 0.0) {
          in.fail("diagonal entry d(" + to_string(i) + ", " + to_string(i) + ") is " + format_number(value) +
                  " but must be 0");
        }
      } else {
        const double mirror = matrix.at(static_cast<int>(j), static_cast<int>(i));
        if (std::fabs(value - mirror) > 1e-9 * std::max(1.0, std::fabs(mirror))) {
          in.fail("the matrix is not symmetric: d(" + to_string(i) + ", " + to_string(j) + ") = " +
                  format_number(value) + " but d(" + to_string(j) + ", " + to_string(i) + ") = " +
                  format_number(mirror));
        }
      }
    }
    if (in.has_more_on_line()) {
      in.fail("row " + to_string(i) + " has more than N = " + to_string(n) +
              " values (the distance matrix must be square)");
    }
  }
  if (in.next_line()) {
    in.fail("unexpected extra line after the " + to_string(n) + " matrix rows (the distance matrix must be N x N)");
  }
  return input;
}

}  // namespace a4
