// Buffered, line-aware tokenizer shared by every CS509 input reader.
#pragma once

#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace cs509 {

// Raised for a missing/unreadable file or any malformed or invalid input value.
class InputError : public std::runtime_error {
 public:
  explicit InputError(const std::string& message) : std::runtime_error(message) {}
};

// Reads a text file one line at a time. Tokens are separated by spaces or tabs and never
// cross a newline, so readers can enforce rules such as "exactly d+1 values on this line".
// The file is streamed through a fixed buffer, so very large inputs are never held in memory.
//
//   Scanner in(path);
//   if (!in.next_line()) in.fail("file is empty");
//   long long v = in.read_int("V");
//   in.expect_line_end("header");
class Scanner {
 public:
  explicit Scanner(const std::string& path);  // throws InputError if the file cannot be opened
  ~Scanner();
  Scanner(const Scanner&) = delete;
  Scanner& operator=(const Scanner&) = delete;

  // Discards the rest of the current line and moves to the next non-blank line.
  // Returns false at end of file.
  bool next_line();

  // True when at least one more token remains on the current line.
  bool has_more_on_line();

  // Read the next token of the current line; `what` names the value in error messages.
  long long read_int(const char* what);
  double read_double(const char* what);  // rejects inf/nan
  std::string read_word(const char* what);

  // Fails if any token remains on the current line.
  void expect_line_end(const char* context);

  // Throw InputError("<path>:<line>: message") or InputError("<path>: message").
  [[noreturn]] void fail(const std::string& message) const;
  [[noreturn]] void fail_file(const std::string& message) const;

  const std::string& path() const { return path_; }
  std::size_t line() const { return line_; }

 private:
  int peek();
  std::size_t read_token(const char* what);  // copies the token into token_ and returns its length

  std::string path_;
  std::FILE* file_ = nullptr;
  std::vector<char> buffer_;
  std::size_t pos_ = 0;
  std::size_t len_ = 0;
  std::size_t line_ = 0;
  bool in_line_ = false;
  char token_[128];
};

}  // namespace cs509
