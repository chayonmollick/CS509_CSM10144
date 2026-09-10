#include "a3/gradient_descent.hpp"

#include <cmath>
#include <cstddef>

namespace a3 {

double evaluate_polynomial(const std::vector<double>& coefficients, double x) {
  double value = 0.0;
  for (std::size_t k = coefficients.size(); k-- > 0;) value = value * x + coefficients[k];
  return value;
}

double evaluate_derivative(const std::vector<double>& coefficients, double x) {
  double value = 0.0;
  for (std::size_t k = coefficients.size(); k-- > 1;) value = value * x + static_cast<double>(k) * coefficients[k];
  return value;
}

GradientDescentResult gradient_descent(const GradientDescentProblem& problem) {
  GradientDescentResult result;
  double x = problem.initial_x;
  double gradient = evaluate_derivative(problem.coefficients, x);
  long long iterations = 0;

  while (std::fabs(gradient) > problem.tolerance && iterations < problem.max_iterations) {
    x -= problem.learning_rate * gradient;
    gradient = evaluate_derivative(problem.coefficients, x);
    ++iterations;
    if (!std::isfinite(x) || !std::isfinite(gradient)) {
      result.diverged = true;
      break;
    }
  }

  result.x = x;
  result.gradient = gradient;
  result.fx = evaluate_polynomial(problem.coefficients, x);
  result.iterations = iterations;
  result.converged = !result.diverged && std::fabs(gradient) <= problem.tolerance;
  return result;
}

}  // namespace a3
