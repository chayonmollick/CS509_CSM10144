#!/usr/bin/env python3
"""Runs every Assignment 3/4 test input through bin/cs509 and writes the result tables.

Usage (from anywhere):
    python3 scripts/make_report.py [--skip-verify] [--only a3|a4]

Writes reports/Assignment3_Report.md and reports/Assignment4_Report.md; raw program output goes to
reports/raw/. "Expected" columns come from the independent Python reference implementations in tools/.
"""
import argparse
import os
import platform
import re
import subprocess
import sys
from datetime import datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "bin", "cs509")
RAW = os.path.join(ROOT, "reports", "raw")
A3 = os.path.join("tests", "assignment3")
A4 = os.path.join("tests", "assignment4")
TIMEOUT_S = 1800

# Timed repetitions per input; the wrapper reports the average (documented in the report).
RUNS_TINY = 1000  # gradient descent
RUNS_SMALL = 100  # graphs / datasets with at most 1,000 vertices or points
RUNS_LARGE = 5


def runs_for(size):
    return RUNS_SMALL if size <= 1000 else RUNS_LARGE


def execute(cmd, raw_name):
    """Runs a command from the repo root, saves stdout+stderr, and returns (status, stdout, stderr)."""
    try:
        proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=TIMEOUT_S)
        status, out, err = proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired:
        status, out, err = "timeout", "", "timed out after %d s" % TIMEOUT_S
    with open(os.path.join(RAW, raw_name), "w") as f:
        f.write("$ " + " ".join(cmd) + "\n" + out + ("\n[stderr]\n" + err if err else ""))
    return status, out, err


def field(text, label):
    match = re.search(r"^" + re.escape(label) + r":\s*(.*)$", text, re.M)
    return match.group(1).strip() if match else None


def times_ms(text):
    return [float(t) for t in re.findall(r"^Execution time: ([0-9.]+) ms", text, re.M)]


def header_sizes(text):
    match = re.search(r"^Input: .*\((.*)\)$", text, re.M)
    sizes = {}
    if match:
        for part in match.group(1).split(","):
            key, _, value = part.partition("=")
            sizes[key.strip()] = value.strip()
    return sizes


def verify(enabled, script, *args):
    """Runs tools/<script> and parses its 'KEY value...' lines into a dict (empty if unavailable)."""
    path = os.path.join(ROOT, "tools", script)
    if not enabled or not os.path.exists(path):
        return {}
    name = script.replace(".py", "") + "_" + os.path.basename(args[0]) + ".txt"
    status, out, _ = execute([sys.executable, path] + list(args), name)
    if status != 0:
        return {}
    result = {}
    for line in out.splitlines():
        parts = line.split()
        if parts and parts[0].isupper():
            result[parts[0]] = parts[1:]
    return result


def failure(status, err):
    if status == "timeout":
        return "Fail (timeout)"
    if "out of memory" in err:
        return "Fail (out of memory)"
    if isinstance(status, int) and status < 0:
        return "Fail (killed by signal %d)" % -status
    return "Fail (exit %s)" % status


def fmt_ms(value):
    return "%.4f ms" % value


def table(headers, rows):
    lines = ["| " + " | ".join(headers) + " |", "|" + "---|" * len(headers)]
    lines += ["| " + " | ".join(str(c) for c in row) + " |" for row in rows]
    return "\n".join(lines)


def existing(folder, names):
    return [os.path.join(folder, n) for n in names if os.path.exists(os.path.join(ROOT, folder, n))]


def missing_note(folder, names):
    missing = [n for n in names if not os.path.exists(os.path.join(ROOT, folder, n))]
    return ("\nNot run (input file not present): " + ", ".join(missing) + "\n") if missing else ""


def system_info():
    compiler = subprocess.run(["c++", "--version"], capture_output=True, text=True).stdout.splitlines()
    cpu = platform.processor()
    try:
        if sys.platform == "darwin":
            cpu = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"], capture_output=True, text=True).stdout.strip()
        elif os.path.exists("/proc/cpuinfo"):
            cpu = next(l.split(":", 1)[1].strip() for l in open("/proc/cpuinfo") if l.startswith("model name"))
    except Exception:
        pass
    return ("- Date: %s\n- Machine: %s, %s\n- Compiler: %s (flags: -std=c++17 -O2)\n"
            % (datetime.now().strftime("%Y-%m-%d %H:%M"), platform.platform(), cpu, compiler[0] if compiler else "unknown"))


TIMING_NOTE = (
    "Timing covers only the algorithm call (clock started immediately before and stopped immediately after it); "
    "file reading, validation, adjacency-list-to-CSR conversion, result verification and printing are excluded. "
    "Each time is the average of repeated runs of the same call: %d runs for gradient descent, %d runs for inputs "
    "with at most 1,000 vertices/points/objects, and %d runs for larger inputs (1 run for fm_04)."
    % (RUNS_TINY, RUNS_SMALL, RUNS_LARGE))


def mst_rows(check):
    names = ["mst_%d.txt" % v for v in (10, 100, 10000, 50000, 100000)]
    rows = []
    for path in existing(A3, names):
        base = os.path.basename(path)
        v = int(re.findall(r"\d+", base)[0])
        status, out, err = execute([BIN, "mst", path, "--quiet", "--runs", str(runs_for(v))], "mst_" + base)
        sizes = header_sizes(out)
        weights = re.findall(r"^Total MST weight: (-?\d+)", out, re.M)
        times = times_ms(out)
        expected = verify(check, "verify_mst.py", path).get("EXPECTED_WEIGHT", ["n/a"])[0]
        if status != 0 or len(weights) != 2 or len(times) != 2:
            rows.append([base, sizes.get("V", v), sizes.get("E", "?"), expected, "-", "-", "-", "-", "-", failure(status, err)])
            continue
        equal = weights[0] == weights[1]
        ok = equal and (expected == "n/a" or expected == weights[0])
        rows.append([base, sizes["V"], sizes["E"], expected, weights[0], weights[1], fmt_ms(times[0]), fmt_ms(times[1]),
                     "Yes" if equal else "No", "Pass" if ok else "Fail"])
    return table(["File", "V", "E", "Exp. Wt.", "Kruskal Wt.", "Prim Wt.", "Kruskal Time", "Prim Time", "Equal?", "Status"],
                 rows) + missing_note(A3, names)


def gd_rows():
    expected_x = {"gd_01.txt": 3.0}
    names = ["gd_0%d.txt" % i for i in range(1, 6)]
    rows = []
    for path in existing(A3, names):
        base = os.path.basename(path)
        params = {}
        with open(os.path.join(ROOT, path)) as f:
            for line in f:
                parts = line.split()
                if parts:
                    params[parts[0]] = parts[1:]
        status, out, err = execute([BIN, "gd", path, "--runs", str(RUNS_TINY)], "gd_" + base)
        x_star = expected_x.get(base, 0.0)
        if status != 0:
            rows.append([base, params["DEGREE"][0], "-", "-", "-", "-", x_star, 0, "-", "-", "-", failure(status, err)])
            continue
        x, fx = float(field(out, "Final x")), float(field(out, "Final f(x)"))
        converged = field(out, "Converged") == "true"
        ok = converged and abs(x - x_star) <= 1e-4 and abs(fx) <= 1e-6
        rows.append([base, field(out, "Degree"), params["INITIAL_X"][0], params["LEARNING_RATE"][0], params["TOLERANCE"][0],
                     params["MAX_ITERATIONS"][0], "%g" % x_star, 0, "%.10f" % x, "%.3e" % fx,
                     "%s / %s" % (field(out, "Iterations"), fmt_ms(times_ms(out)[0])), "Pass" if ok else "Fail"])
    return table(["File", "Degree", "x0", "Rate", "Tol.", "Max Iter.", "Exp. x*", "Exp. f(x*)", "Actual x", "Actual f(x)",
                  "Iter. / Time", "Status"], rows) + missing_note(A3, names)


def maxflow_rows(check):
    names = ["maxflow_%d.txt" % v for v in (10, 100, 1000, 10000, 50000, 100000)]
    rows = []
    for path in existing(A3, names):
        base = os.path.basename(path)
        v = int(re.findall(r"\d+", base)[0])
        status, out, err = execute([BIN, "maxflow", path, "--quiet", "--runs", str(runs_for(v))], "maxflow_" + base)
        sizes = header_sizes(out)
        expected = verify(check, "verify_maxflow.py", path).get("EXPECTED_FLOW", ["n/a"])[0]
        if status != 0:
            rows.append([base, v, sizes.get("E", "?"), "-", "-", expected, "-", "-", "-", failure(status, err)])
            continue
        flow, cut = field(out, "Maximum flow"), field(out, "Minimum cut capacity")
        ok = flow == cut and (expected == "n/a" or expected == flow)
        rows.append([base, sizes["V"], sizes["E"], field(out, "Source"), field(out, "Sink"), expected, flow, cut,
                     fmt_ms(times_ms(out)[0]), "Pass" if ok else "Fail"])
    return table(["File", "V", "E", "Source", "Sink", "Exp. Flow", "Actual Flow", "Cut Capacity", "Time", "Status"],
                 rows) + missing_note(A3, names)


def coloring_rows(check):
    names = ["color_%d.txt" % v for v in (10, 100, 10000, 50000, 100000)]
    rows = []
    for path in existing(A4, names):
        base = os.path.basename(path)
        v = int(re.findall(r"\d+", base)[0])
        raw = "coloring_" + base
        status, out, err = execute([BIN, "coloring", path, "--runs", str(runs_for(v))], raw)
        sizes = header_sizes(out)
        if status != 0:
            rows.append([base, v, sizes.get("E", "?"), "-", "-", "-", "-", failure(status, err)])
            continue
        valid = field(out, "Valid coloring") == "Yes"
        independent = verify(check, "verify_coloring.py", path, os.path.join(RAW, raw)).get("VALID", ["n/a"])[0]
        valid_text = "Yes" if valid else "No"
        if independent != "n/a":
            valid_text += " (independent check: %s)" % independent
        ok = valid and independent in ("yes", "n/a")
        rows.append([base, sizes["V"], sizes["E"], field(out, "Colors used"), field(out, "Max degree") or "-", valid_text,
                     fmt_ms(times_ms(out)[0]), "Pass" if ok else "Fail"])
    return table(["File", "V", "E", "Colors Used", "Max Degree", "Valid?", "Time", "Status"], rows) + missing_note(A4, names)


def pagerank_rows(check):
    names = ["pagerank_%d.txt" % v for v in (10, 100, 1000, 10000, 50000, 100000)]
    rows = []
    for path in existing(A4, names):
        base = os.path.basename(path)
        v = int(re.findall(r"\d+", base)[0])
        status, out, err = execute([BIN, "pagerank", path, "--quiet", "--runs", str(runs_for(v))], "pagerank_" + base)
        sizes = header_sizes(out)
        if status != 0:
            rows.append([base, v, sizes.get("E", "?"), "-", "-", "-", "-", "-", failure(status, err)])
            continue
        top = field(out, "Top vertex")
        total = float(field(out, "Sum of ranks"))
        converged = field(out, "Converged") == "true"
        ref = verify(check, "verify_pagerank.py", path)
        ref_top = ref.get("TOP_VERTEX", ["n/a"])[0]
        ok = converged and abs(total - 1.0) <= 1e-6 and (ref_top == "n/a" or ref_top == top.split()[0])
        rows.append([base, sizes["V"], sizes["E"], field(out, "Damping"), top, ref_top, field(out, "Dangling vertices"),
                     "%.6f" % total, "%s / %s" % (field(out, "Iterations"), fmt_ms(times_ms(out)[0])),
                     "Pass" if ok else "Fail"])
    return table(["File", "V", "E", "Damping", "Top Vertex", "Exp. Top", "Dangling", "Sum of Ranks", "Iter. / Time", "Status"],
                 rows) + missing_note(A4, names)


def kmeans_rows(check):
    names = ["km_0%d.txt" % i for i in range(1, 5)]
    rows = []
    for path in existing(A4, names):
        base = os.path.basename(path)
        with open(os.path.join(ROOT, path)) as f:
            n, d, k = f.readline().split()
            max_iter = next((l.split()[1] for l in f if l.startswith("MAX_ITERATIONS")), "?")
        status, out, err = execute([BIN, "kmeans", path, "--quiet", "--runs", str(runs_for(int(n)))], "kmeans_" + base)
        if status != 0:
            rows.append([base, n, d, k, max_iter, "-", "-", "-", "-", failure(status, err)])
            continue
        wcss = float(field(out, "WCSS"))
        ref = verify(check, "verify_kmeans.py", path)
        ref_wcss = ref.get("WCSS", ["n/a"])[0]
        ok = field(out, "Converged") == "true" and (
            ref_wcss == "n/a" or abs(float(ref_wcss) - wcss) <= 1e-6 * max(1.0, abs(wcss)))
        rows.append([base, n, d, k, max_iter, field(out, "Iterations"), "%.6f" % wcss, ref_wcss,
                     fmt_ms(times_ms(out)[0]), "Pass" if ok else "Fail"])
    return table(["File", "N", "D", "K", "Max Iter.", "Actual Iter.", "WCSS", "Exp. WCSS", "Time", "Status"],
                 rows) + missing_note(A4, names)


def fastmap_rows():
    names = ["fm_0%d.txt" % i for i in range(1, 5)]
    rows = []
    for path in existing(A4, names):
        base = os.path.basename(path)
        with open(os.path.join(ROOT, path)) as f:
            n, k = f.readline().split()
        runs = 1 if int(n) > 1000 else runs_for(int(n))
        status, out, err = execute([BIN, "fastmap", path, "--quiet", "--runs", str(runs)], "fastmap_" + base)
        if status != 0:
            rows.append([base, n, k, "-", "-", "-", failure(status, err)])
            continue
        pivots = "; ".join("%s" % " ".join(p) for p in re.findall(r"^Dim \d+: (\d+) (\d+)", out, re.M))
        rows.append([base, n, field(out, "Target dimensions"), pivots, field(out, "Avg. distance error"),
                     fmt_ms(times_ms(out)[0]), "Pass"])
    return table(["File", "N", "Target k", "Pivots (per dim)", "Avg. Distance Error", "Time", "Status"],
                 rows) + missing_note(A4, names)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-verify", action="store_true", help="do not run the Python reference implementations")
    parser.add_argument("--only", choices=["a3", "a4"])
    args = parser.parse_args()
    if not os.path.exists(BIN):
        sys.exit("bin/cs509 not found - run 'make' first")
    os.makedirs(RAW, exist_ok=True)
    check = not args.skip_verify
    info = system_info()

    if args.only in (None, "a3"):
        with open(os.path.join(ROOT, "reports", "Assignment3_Report.md"), "w") as f:
            f.write("# CS509 Assignment 3 - Results\n\n" + info + "\n" + TIMING_NOTE + "\n\n")
            f.write("## 1. MST - Kruskal and Prim\n\nWeighted undirected connected graphs (CSR). "
                    "Exp. Wt. = independent Python Kruskal (tools/verify_mst.py).\n\n" + mst_rows(check) + "\n\n")
            f.write("## 2. Gradient Descent\n\n" + gd_rows() + "\n\n")
            f.write("## 3. Maxflow-Mincut (Dinic)\n\nDirected capacity networks (CSR). "
                    "Exp. Flow = independent Python max-flow (tools/verify_maxflow.py).\n\n" + maxflow_rows(check) + "\n")
        print("wrote reports/Assignment3_Report.md")

    if args.only in (None, "a4"):
        with open(os.path.join(ROOT, "reports", "Assignment4_Report.md"), "w") as f:
            f.write("# CS509 Assignment 4 - Results\n\n" + info + "\n" + TIMING_NOTE + "\n\n")
            f.write("## 1. Greedy Vertex Coloring (Welsh-Powell order)\n\nUnweighted undirected graphs (CSR). "
                    "Validity is checked by the program and independently by tools/verify_coloring.py.\n\n"
                    + coloring_rows(check) + "\n\n")
            f.write("## 2. PageRank\n\nUnweighted directed graphs (CSR). Exp. Top = independent Python power "
                    "iteration (tools/verify_pagerank.py).\n\n" + pagerank_rows(check) + "\n\n")
            f.write("## 3. K-Means Clustering\n\nExp. WCSS = independent Python Lloyd's algorithm "
                    "(tools/verify_kmeans.py).\n\n" + kmeans_rows(check) + "\n\n")
            f.write("## 4. FastMap\n\nPivot selection: random start object (seed 1), then 2 rounds of "
                    "farthest-object search per dimension. Avg. distance error = mean |d(i,j) - ||x_i - x_j|| | "
                    "over all pairs.\n\n" + fastmap_rows() + "\n")
        print("wrote reports/Assignment4_Report.md")


if __name__ == "__main__":
    main()
