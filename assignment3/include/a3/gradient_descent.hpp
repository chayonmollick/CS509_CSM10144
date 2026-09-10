// Gradient Descent on a one-variable polynomial f(x) = c0 + c1*x + ... + cd*x^d.
#pragma once

#include <vector>

namespace a3 {

struct GradientDescentProblem {
  std::vector<double> coefficients;  // c0 .. cd in ascending power order (d + 1 values)
  double initial_x = 0.0;
  double learning_rate = 0.0;
  double tolerance = 0.0;
  long long max_iterations = 0;

  int degree() const { return static_cast<int>(coefficients.size()) - 1; }
};

struct GradientDescentResult {
  double x = 0.0;
  double fx = 0.0;
  double gradient = 0.0;     // f'(x) at the final x
  long long iterations = 0;  // number of x updates performed
  bool converged = false;    // |f'(x)| <= tolerance
  bool diverged = false;     // x or f'(x) stopped being finite
};

// Horner evaluation of f(x), and of f'(x) = c1 + 2*c2*x + ... + d*cd*x^(d-1) from the same coefficients.
double evaluate_polynomial(const std::vector<double>& coefficients, double x);
double evaluate_derivative(const std::vector<double>& coefficients, double x);

// Repeats x <- x - learning_rate * f'(x) until |f'(x)| <= tolerance or max_iterations updates were made.
GradientDescentResult gradient_descent(const GradientDescentProblem& problem);

}  // namespace a3
