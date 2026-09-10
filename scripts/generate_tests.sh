#!/usr/bin/env bash
# Regenerates the CS509 Assignment 3/4 test inputs with fixed seeds.
# Output is byte-identical on macOS and Linux (the generators use their own RNG and formatting).
#
# Usage: scripts/generate_tests.sh [--large] [--optional]
#   --large     also write tests/assignment4/fm_04.txt (N=10,000 distance matrix, ~700 MB, gitignored)
#   --optional  also write the 100,000-vertex maxflow/pagerank inputs (gitignored)
# (The gradient-descent gd_*.txt inputs are maintained separately and are not touched.)
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

LARGE=0
OPTIONAL=0
for arg in "$@"; do
    case "$arg" in
        --large) LARGE=1 ;;
        --optional) OPTIONAL=1 ;;
        -h | --help)
            sed -n '2,8p' "$0"
            exit 0
            ;;
        *)
            echo "generate_tests.sh: unknown option '$arg' (use --large and/or --optional)" >&2
            exit 1
            ;;
    esac
done

TOOLS=(gen_graph gen_points gen_distance_matrix)
build_direct() {
    mkdir -p bin
    for t in "${TOOLS[@]}"; do
        echo "c++ -std=c++17 -O2 -Wall -Wextra -pedantic tools/$t.cpp -o bin/$t"
        c++ -std=c++17 -O2 -Wall -Wextra -pedantic "tools/$t.cpp" -o "bin/$t"
    done
}
if ! make tools; then
    echo "generate_tests.sh: 'make tools' failed; compiling the generators directly" >&2
    build_direct
fi
for t in "${TOOLS[@]}"; do
    if [[ ! -x "bin/$t" ]]; then
        build_direct
        break
    fi
done

A3=tests/assignment3
A4=tests/assignment4
mkdir -p "$A3" "$A4"
START=$SECONDS

graph() { bin/gen_graph "$@"; }

echo "== Assignment 3: MST (E = 3V)"
graph mst 10 30 "$A3/mst_10.txt" 1101
graph mst 100 300 "$A3/mst_100.txt" 1102
graph mst 10000 30000 "$A3/mst_10000.txt" 1103
graph mst 50000 150000 "$A3/mst_50000.txt" 1104
graph mst 100000 300000 "$A3/mst_100000.txt" 1105

echo "== Assignment 3: max flow (E = 4V)"
graph maxflow 10 40 "$A3/maxflow_10.txt" 2101
graph maxflow 100 400 "$A3/maxflow_100.txt" 2102
graph maxflow 1000 4000 "$A3/maxflow_1000.txt" 2103
graph maxflow 10000 40000 "$A3/maxflow_10000.txt" 2104
graph maxflow 50000 200000 "$A3/maxflow_50000.txt" 2105
if ((OPTIONAL)); then
    graph maxflow 100000 400000 "$A3/maxflow_100000.txt" 2106
fi

echo "== Assignment 4: graph coloring (E = 3V)"
graph coloring 10 30 "$A4/color_10.txt" 3101
graph coloring 100 300 "$A4/color_100.txt" 3102
graph coloring 10000 30000 "$A4/color_10000.txt" 3103
graph coloring 50000 150000 "$A4/color_50000.txt" 3104
graph coloring 100000 300000 "$A4/color_100000.txt" 3105

echo "== Assignment 4: PageRank (E = 4V)"
graph pagerank 10 40 "$A4/pagerank_10.txt" 4101
graph pagerank 100 400 "$A4/pagerank_100.txt" 4102
graph pagerank 1000 4000 "$A4/pagerank_1000.txt" 4103
graph pagerank 10000 40000 "$A4/pagerank_10000.txt" 4104
graph pagerank 50000 200000 "$A4/pagerank_50000.txt" 4105
if ((OPTIONAL)); then
    graph pagerank 100000 400000 "$A4/pagerank_100000.txt" 4106
fi

echo "== Assignment 4: K-Means (max_iterations 300, tolerance 0.0001)"
bin/gen_points 100 2 3 "$A4/km_01.txt" 5101 300 0.0001
bin/gen_points 1000 2 5 "$A4/km_02.txt" 5102 300 0.0001
bin/gen_points 10000 5 8 "$A4/km_03.txt" 5103 300 0.0001
bin/gen_points 100000 5 10 "$A4/km_04.txt" 5104 300 0.0001

echo "== Assignment 4: FastMap distance matrices"
bin/gen_distance_matrix 10 2 "$A4/fm_01.txt" 6101
bin/gen_distance_matrix 100 2 "$A4/fm_02.txt" 6102
bin/gen_distance_matrix 1000 3 "$A4/fm_03.txt" 6103
if ((LARGE)); then
    bin/gen_distance_matrix 10000 3 "$A4/fm_04.txt" 6104
fi

echo "== Done in $((SECONDS - START)) s"
