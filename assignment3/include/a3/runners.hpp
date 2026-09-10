// Assignment 3 entry points called by the common wrapper.
#pragma once

#include "cs509/options.hpp"

namespace a3 {

int run_kruskal(const cs509::RunOptions& options);
int run_prim(const cs509::RunOptions& options);
int run_mst_compare(const cs509::RunOptions& options);  // Kruskal and Prim on the same CSR graph
int run_gradient_descent(const cs509::RunOptions& options);
int run_maxflow(const cs509::RunOptions& options);

}  // namespace a3
