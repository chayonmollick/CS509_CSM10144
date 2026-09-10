#include "cs509/scanner.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace cs509 {
namespace {

constexpr std::size_t kBufferSize = 1 << 16;

inline bool is_blank(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v'; }
inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Powers of ten that are exactly representable as doubles.
constexpr double kExactPow10[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
                                  1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

// Parses plain decimals such as "-12.5", "0.000001" or "3e-4" when the result can be computed
// exactly with one multiplication or division (Clinger's fast path). Returns false for anything
// else so that strtod decides. This keeps multi-hundred-megabyte distance matrices quick to read.
bool parse_double_fast(const char* s, std::size_t n, double& out) {
  std::size_t i = 0;
  const bool negative = n > 0 && s[0] == '-';
  if (n > 0 && (s[0] == '-' || s[0] == '+')) ++i;

  unsigned long long mantissa = 0;
  int significant_digits = 0;
  int exponent = 0;
  bool any_digit = false;
  auto append = [&](char c) {
    if (mantissa == 0 && c == '0') return true;  // leading zeros carry no precision
    if (++significant_digits > 18) return false;
    mantissa = mantissa * 10 + static_cast<unsigned>(c - '0');
    return true;
  };

  for (; i < n && is_digit(s[i]); ++i, any_digit = true) {
    if (!append(s[i])) return false;
  }
  if (i < n && s[i] == '.') {
    for (++i; i < n && is_digit(s[i]); ++i, any_digit = true, --exponent) {
      if (!append(s[i])) return false;
    }
  }
  if (!any_digit) return false;

  if (i < n && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    const bool negative_exponent = i < n && s[i] == '-';
    if (i < n && (s[i] == '-' || s[i] == '+')) ++i;
    if (i == n) return false;
    int value = 0;
    for (; i < n && is_digit(s[i]); ++i) {
      if (value > 1000) return false;
      value = value * 10 + (s[i] - '0');
    }
    exponent += negative_exponent ? -value : value;
  }

  if (i != n || mantissa > (1ULL << 53) || exponent < -22 || exponent > 22) return false;
  double value = static_cast<double>(mantissa);
  value = exponent < 0 ? value / kExactPow10[-exponent] : value * kExactPow10[exponent];
  out = negative ? -value : value;
  return true;
}

}  // namespace

Scanner::Scanner(const std::string& path) : path_(path), buffer_(kBufferSize) {
  file_ = std::fopen(path.c_str(), "rb");
  if (file_ == nullptr) {
    throw InputError("cannot open input file '" + path + "': " + std::strerror(errno));
  }
}

Scanner::~Scanner() {
  if (file_ != nullptr) std::fclose(file_);
}

int Scanner::peek() {
  if (pos_ == len_) {
    len_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
    pos_ = 0;
    if (len_ == 0) return EOF;
  }
  return static_cast<unsigned char>(buffer_[pos_]);
}

bool Scanner::next_line() {
  if (in_line_) {
    int c = peek();
    while (c != EOF && c != '\n') {
      ++pos_;
      c = peek();
    }
    if (c == '\n') ++pos_;
    in_line_ = false;
  }
  for (;;) {
    int c = peek();
    while (is_blank(c)) {
      ++pos_;
      c = peek();
    }
    if (c == EOF) return false;
    ++line_;
    if (c != '\n') {
      in_line_ = true;
      return true;
    }
    ++pos_;  // blank line
  }
}

bool Scanner::has_more_on_line() {
  if (!in_line_) return false;
  int c = peek();
  while (is_blank(c)) {
    ++pos_;
    c = peek();
  }
  return c != EOF && c != '\n';
}

std::size_t Scanner::read_token(const char* what) {
  if (!has_more_on_line()) fail(std::string("missing value for ") + what);
  std::size_t n = 0;
  for (int c = peek(); c != EOF && c != '\n' && !is_blank(c); c = peek()) {
    if (n + 1 >= sizeof(token_)) fail(std::string("value for ") + what + " is too long");
    token_[n++] = static_cast<char>(c);
    ++pos_;
  }
  token_[n] = '\0';
  return n;
}

long long Scanner::read_int(const char* what) {
  const std::size_t n = read_token(what);
  std::size_t i = 0;
  const bool negative = token_[0] == '-';
  if (token_[0] == '-' || token_[0] == '+') ++i;
  if (i == n) fail(std::string("expected an integer for ") + what + ", found '" + token_ + "'");

  const unsigned long long limit = negative ? 9223372036854775808ULL : 9223372036854775807ULL;
  unsigned long long value = 0;
  for (; i < n; ++i) {
    if (!is_digit(token_[i])) fail(std::string("expected an integer for ") + what + ", found '" + token_ + "'");
    const unsigned digit = static_cast<unsigned>(token_[i] - '0');
    if (value > (limit - digit) / 10) fail(std::string("integer for ") + what + " is out of range: '" + token_ + "'");
    value = value * 10 + digit;
  }
  return negative ? static_cast<long long>(0ULL - value) : static_cast<long long>(value);
}

double Scanner::read_double(const char* what) {
  const std::size_t n = read_token(what);
  double value = 0.0;
  if (parse_double_fast(token_, n, value)) return value;

  char* end = nullptr;
  value = std::strtod(token_, &end);
  if (end != token_ + n) fail(std::string("expected a number for ") + what + ", found '" + token_ + "'");
  if (!std::isfinite(value)) fail(std::string("value for ") + what + " is not a finite number: '" + token_ + "'");
  return value;
}

std::string Scanner::read_word(const char* what) {
  const std::size_t n = read_token(what);
  return std::string(token_, n);
}

void Scanner::expect_line_end(const char* context) {
  if (has_more_on_line()) {
    const std::string extra = read_word("extra value");
    fail("unexpected extra value '" + extra + "' in " + context);
  }
}

void Scanner::fail(const std::string& message) const {
  throw InputError(path_ + ":" + std::to_string(line_) + ": " + message);
}

void Scanner::fail_file(const std::string& message) const { throw InputError(path_ + ": " + message); }

}  // namespace cs509
