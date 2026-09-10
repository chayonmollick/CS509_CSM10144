// Assignment 4 entry points called by the common wrapper.
#pragma once

#include "cs509/options.hpp"

namespace a4 {

int run_coloring(const cs509::RunOptions& options);
int run_pagerank(const cs509::RunOptions& options);
int run_kmeans(const cs509::RunOptions& options);
int run_fastmap(const cs509::RunOptions& options);

}  // namespace a4
