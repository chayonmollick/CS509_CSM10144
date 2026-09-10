// gen_graph - reproducible random graph generator for the CS509 Assignment 3/4 inputs.
//
// Usage: gen_graph <mst|coloring|pagerank|maxflow> <V> <E> <output-file> [seed]
//
// Output is byte-identical on every platform for a given seed: the RNG is a hand-written
// splitmix64, no <random> distributions / std::shuffle are used, and all numbers are formatted
// with integer arithmetic.
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef __clang__
#pragma clang fp contract(off)
#endif

namespace {

[[noreturn]] void die(const std::string& msg) {
    std::fprintf(stderr, "gen_graph: %s\n", msg.c_str());
    std::exit(1);
}

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed) {}
    uint64_t next() {
        s += 0x9E3779B97F4A7C15ULL;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    uint64_t below(uint64_t n) { return next() % n; }
    double unit() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }
};

class Writer {
public:
    Writer(const char* path, size_t cap) : buf_(cap), pos_(0), path_(path) {
        f_ = std::fopen(path, "wb");
        if (!f_) die("cannot open output file '" + path_ + "': " + std::strerror(errno));
    }
    void reserve(size_t n) {
        if (pos_ + n > buf_.size()) flush();
    }
    void ch(char c) { buf_[pos_++] = c; }
    void u64(uint64_t v) {
        char tmp[24];
        int n = 0;
        do {
            tmp[n++] = static_cast<char>('0' + v % 10);
            v /= 10;
        } while (v != 0);
        while (n > 0) buf_[pos_++] = tmp[--n];
    }
    void i64(int64_t v) {
        if (v < 0) {
            ch('-');
            u64(static_cast<uint64_t>(0) - static_cast<uint64_t>(v));
        } else {
            u64(static_cast<uint64_t>(v));
        }
    }
    void str(const char* s) {
        size_t n = std::strlen(s);
        reserve(n);
        std::memcpy(&buf_[pos_], s, n);
        pos_ += n;
    }
    void flush() {
        if (pos_ != 0 && std::fwrite(buf_.data(), 1, pos_, f_) != pos_)
            die("write to '" + path_ + "' failed");
        pos_ = 0;
    }
    void close() {
        flush();
        if (std::fclose(f_) != 0) die("closing '" + path_ + "' failed");
        f_ = nullptr;
    }

private:
    std::vector<char> buf_;
    size_t pos_;
    std::string path_;
    FILE* f_;
};

bool parse_u64(const char* s, uint64_t& out) {
    if (s == nullptr || *s == '\0') return false;
    uint64_t v = 0;
    for (const char* p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        uint64_t d = static_cast<uint64_t>(*p - '0');
        if (v > (UINT64_MAX - d) / 10) return false;
        v = v * 10 + d;
    }
    out = v;
    return true;
}

uint64_t isqrt(uint64_t n) {
    uint64_t r = 0;
    while ((r + 1) * (r + 1) <= n) ++r;  // n is at most ~1e8 here, so this is cheap enough
    return r;
}

// Open-addressing hash set of 64-bit keys (never contains UINT64_MAX).
class KeySet {
public:
    explicit KeySet(uint64_t expected) {
        uint64_t cap = 16;
        while (cap < expected * 2 + 16) cap <<= 1;
        t_.assign(static_cast<size_t>(cap), EMPTY);
        mask_ = cap - 1;
    }
    bool insert(uint64_t k) {
        uint64_t i = mix(k) & mask_;
        for (;;) {
            if (t_[i] == EMPTY) {
                t_[i] = k;
                return true;
            }
            if (t_[i] == k) return false;
            i = (i + 1) & mask_;
        }
    }
    bool contains(uint64_t k) const {
        uint64_t i = mix(k) & mask_;
        for (;;) {
            if (t_[i] == EMPTY) return false;
            if (t_[i] == k) return true;
            i = (i + 1) & mask_;
        }
    }

private:
    static constexpr uint64_t EMPTY = UINT64_MAX;
    static uint64_t mix(uint64_t z) {
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    std::vector<uint64_t> t_;
    uint64_t mask_;
};

struct Edge {
    uint32_t u, v;
    int32_t w;
};

struct Builder {
    uint64_t V;
    bool directed;
    KeySet set;
    std::vector<Edge> edges;
    Builder(uint64_t V_, bool directed_, uint64_t E) : V(V_), directed(directed_), set(E) {
        edges.reserve(static_cast<size_t>(E));
    }
    uint64_t key(uint64_t u, uint64_t v) const {
        if (!directed && u > v) std::swap(u, v);
        return u * V + v;
    }
    bool has(uint64_t u, uint64_t v) const { return set.contains(key(u, v)); }
    // Adds (u,v) with weight 0 if it is not a self-loop / duplicate; the caller sets the weight.
    bool add(uint64_t u, uint64_t v) {
        if (u == v) return false;
        if (!set.insert(key(u, v))) return false;
        edges.push_back(Edge{static_cast<uint32_t>(u), static_cast<uint32_t>(v), 0});
        return true;
    }
};

// Draws `need` distinct pairs uniformly from the candidates enumerated by `for_each_candidate`
// that are not in the builder yet (used when the request is dense and rejection sampling would
// stall). Calls on_add(edge index) after each insertion so the caller can draw its weight.
template <class Enumerate, class OnAdd>
void fill_by_enumeration(Builder& b, uint64_t need, Rng& rng, Enumerate for_each_candidate, OnAdd on_add) {
    std::vector<uint64_t> cand;
    for_each_candidate([&](uint64_t u, uint64_t v) {
        if (!b.has(u, v)) cand.push_back(u * b.V + v);
    });
    if (cand.size() < need) die("internal error: not enough candidate pairs");
    for (uint64_t i = 0; i < need; ++i) {
        uint64_t j = i + rng.below(cand.size() - i);
        std::swap(cand[i], cand[j]);
        uint64_t u = cand[i] / b.V;
        uint64_t v = cand[i] % b.V;
        if (!b.add(u, v)) die("internal error: duplicate candidate");
        on_add(b.edges.size() - 1);
    }
}

struct Csr {
    std::vector<uint64_t> off;
    std::vector<uint32_t> nb;
    std::vector<int32_t> wt;
};

Csr build_csr(const Builder& b) {
    Csr c;
    uint64_t V = b.V;
    c.off.assign(static_cast<size_t>(V + 1), 0);
    for (const Edge& e : b.edges) {
        ++c.off[e.u + 1];
        if (!b.directed) ++c.off[e.v + 1];
    }
    for (uint64_t i = 0; i < V; ++i) c.off[i + 1] += c.off[i];
    c.nb.resize(static_cast<size_t>(c.off[V]));
    c.wt.resize(static_cast<size_t>(c.off[V]));
    std::vector<uint64_t> pos(c.off.begin(), c.off.end() - 1);
    for (const Edge& e : b.edges) {
        c.nb[pos[e.u]] = e.v;
        c.wt[pos[e.u]++] = e.w;
        if (!b.directed) {
            c.nb[pos[e.v]] = e.u;
            c.wt[pos[e.v]++] = e.w;
        }
    }
    return c;
}

uint64_t find_root(std::vector<uint32_t>& parent, uint64_t x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

// Weakly connected components (edge directions ignored).
uint64_t count_components(const Builder& b) {
    std::vector<uint32_t> parent(static_cast<size_t>(b.V));
    for (uint64_t i = 0; i < b.V; ++i) parent[i] = static_cast<uint32_t>(i);
    uint64_t comps = b.V;
    for (const Edge& e : b.edges) {
        uint64_t a = find_root(parent, e.u);
        uint64_t c = find_root(parent, e.v);
        if (a != c) {
            parent[a] = static_cast<uint32_t>(c);
            --comps;
        }
    }
    return comps;
}

void write_adjacency(Writer& w, const Builder& b, const Csr& c, bool weighted) {
    w.reserve(64);
    w.u64(b.V);
    w.ch(' ');
    w.u64(b.edges.size());
    w.ch('\n');
    for (uint64_t u = 0; u < b.V; ++u) {
        w.reserve(64);
        w.u64(u);
        w.ch(' ');
        w.u64(c.off[u + 1] - c.off[u]);
        for (uint64_t i = c.off[u]; i < c.off[u + 1]; ++i) {
            w.reserve(64);
            w.ch(' ');
            w.u64(c.nb[i]);
            if (weighted) {
                w.ch(' ');
                w.i64(c.wt[i]);
            }
        }
        w.ch('\n');
    }
}

void usage() {
    std::fprintf(stderr,
                 "usage: gen_graph <mst|coloring|pagerank|maxflow> <V> <E> <output-file> [seed]\n"
                 "  mst       connected weighted undirected graph, V-1 <= E <= V(V-1)/2, weights in [-100,1000]\n"
                 "  coloring  unweighted undirected graph (may be disconnected), 0 <= E <= V(V-1)/2\n"
                 "  pagerank  directed graph with ~1%% dangling vertices, every vertex has an in-arc\n"
                 "  maxflow   directed graph, capacities in [1,100], source 0, sink V-1, E >= V-1\n");
    std::exit(1);
}

const uint64_t MAX_V = 50000000ULL;
const uint64_t MAX_E = 400000000ULL;

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5 || argc > 6) usage();
    std::string type = argv[1];
    uint64_t V = 0, E = 0, seed = 1;
    if (!parse_u64(argv[2], V)) die(std::string("V must be a non-negative integer, got '") + argv[2] + "'");
    if (!parse_u64(argv[3], E)) die(std::string("E must be a non-negative integer, got '") + argv[3] + "'");
    const char* out = argv[4];
    if (argc == 6 && !parse_u64(argv[5], seed)) die(std::string("seed must be a non-negative integer, got '") + argv[5] + "'");
    if (type != "mst" && type != "coloring" && type != "pagerank" && type != "maxflow")
        die("unknown graph type '" + type + "' (expected mst, coloring, pagerank or maxflow)");
    if (V < 1) die("V must be >= 1");
    if (V > MAX_V) die("V must be <= " + std::to_string(MAX_V));
    if (E > MAX_E) die("E must be <= " + std::to_string(MAX_E));

    const uint64_t undirected_max = V * (V - 1) / 2;
    const uint64_t directed_max = V * (V - 1);
    Rng rng(seed);

    if (type == "mst" || type == "coloring") {
        bool mst = type == "mst";
        if (mst && E < V - 1)
            die("mst needs E >= V-1 = " + std::to_string(V - 1) + " for a connected graph (got E=" + std::to_string(E) + ")");
        if (E > undirected_max)
            die("E=" + std::to_string(E) + " exceeds V(V-1)/2 = " + std::to_string(undirected_max) + " (no self-loops/duplicates)");
        Builder b(V, false, E);
        auto weight = [&](size_t idx) {
            if (mst) b.edges[idx].w = static_cast<int32_t>(static_cast<int64_t>(rng.below(1101)) - 100);
        };
        if (mst && V >= 2) {
            std::vector<uint32_t> p(static_cast<size_t>(V));
            for (uint64_t i = 0; i < V; ++i) p[i] = static_cast<uint32_t>(i);
            for (uint64_t i = V - 1; i >= 1; --i) std::swap(p[i], p[rng.below(i + 1)]);
            for (uint64_t i = 1; i < V; ++i) {
                uint64_t j = rng.below(i);
                if (!b.add(p[i], p[j])) die("internal error: spanning tree edge rejected");
                weight(b.edges.size() - 1);
            }
        }
        uint64_t need = E - b.edges.size();
        uint64_t avail = undirected_max - b.edges.size();
        if (need > 0 && need * 2 > avail) {
            fill_by_enumeration(
                b, need, rng,
                [&](auto emit) {
                    for (uint64_t u = 0; u < V; ++u)
                        for (uint64_t v = u + 1; v < V; ++v) emit(u, v);
                },
                weight);
        } else {
            while (b.edges.size() < E) {
                uint64_t u = rng.below(V);
                uint64_t v = rng.below(V);
                if (b.add(u, v)) weight(b.edges.size() - 1);
            }
        }
        Csr c = build_csr(b);
        Writer w(out, 1u << 22);
        write_adjacency(w, b, c, mst);
        w.close();

        uint64_t max_deg = 0, min_deg = UINT64_MAX, isolated = 0;
        for (uint64_t u = 0; u < V; ++u) {
            uint64_t d = c.off[u + 1] - c.off[u];
            if (d > max_deg) max_deg = d;
            if (d < min_deg) min_deg = d;
            if (d == 0) ++isolated;
        }
        uint64_t comps = count_components(b);
        if (mst) {
            int64_t wmin = 0, wmax = 0;
            for (size_t i = 0; i < b.edges.size(); ++i) {
                if (i == 0 || b.edges[i].w < wmin) wmin = b.edges[i].w;
                if (i == 0 || b.edges[i].w > wmax) wmax = b.edges[i].w;
            }
            std::printf("mst V=%llu E=%llu connected=%s components=%llu min_degree=%llu max_degree=%llu "
                        "weight_min=%lld weight_max=%lld seed=%llu -> %s\n",
                        (unsigned long long)V, (unsigned long long)E, comps == 1 ? "yes" : "no",
                        (unsigned long long)comps, (unsigned long long)min_deg, (unsigned long long)max_deg,
                        (long long)wmin, (long long)wmax, (unsigned long long)seed, out);
        } else {
            std::printf("coloring V=%llu E=%llu components=%llu isolated=%llu max_degree=%llu seed=%llu -> %s\n",
                        (unsigned long long)V, (unsigned long long)E, (unsigned long long)comps,
                        (unsigned long long)isolated, (unsigned long long)max_deg, (unsigned long long)seed, out);
        }
        return 0;
    }

    if (type == "pagerank") {
        uint64_t nd = 0;
        if (V >= 10) nd = V / 100 < 1 ? 1 : V / 100;
        if (V == 1) nd = 1;
        uint64_t ns = V - nd;
        if (V == 1) {
            if (E != 0) die("pagerank with V=1 must have E=0 (no self-loops)");
        } else {
            if (E < V)
                die("pagerank needs E >= V = " + std::to_string(V) +
                    " (every non-dangling vertex needs an out-arc and every vertex an in-arc)");
            if (E > ns * (V - 1))
                die("E=" + std::to_string(E) + " exceeds the maximum " + std::to_string(ns * (V - 1)) +
                    " arcs from the " + std::to_string(ns) + " non-dangling vertices");
        }
        std::vector<uint32_t> perm(static_cast<size_t>(V));
        for (uint64_t i = 0; i < V; ++i) perm[i] = static_cast<uint32_t>(i);
        for (uint64_t i = V - 1; i >= 1 && V > 1; --i) std::swap(perm[i], perm[rng.below(i + 1)]);
        std::vector<char> dangling(static_cast<size_t>(V), 0);
        for (uint64_t i = 0; i < nd; ++i) dangling[perm[i]] = 1;
        std::vector<uint32_t> S(perm.begin() + static_cast<std::ptrdiff_t>(nd), perm.end());
        // Popularity order (independent permutation) so the top-ranked vertices are not simply 0,1,2...
        std::vector<uint32_t> pop(static_cast<size_t>(V));
        for (uint64_t i = 0; i < V; ++i) pop[i] = static_cast<uint32_t>(i);
        for (uint64_t i = V - 1; i >= 1 && V > 1; --i) std::swap(pop[i], pop[rng.below(i + 1)]);

        Builder b(V, true, E);
        if (V >= 2) {
            // Cycle through the non-dangling vertices: each gets one out-arc and one in-arc.
            for (uint64_t i = 0; i < ns; ++i)
                if (!b.add(S[i], S[(i + 1) % ns])) die("internal error: cycle arc rejected");
            // One in-arc for every dangling vertex.
            for (uint64_t i = 0; i < nd; ++i) {
                uint64_t s = S[rng.below(ns)];
                if (!b.add(s, perm[i])) die("internal error: dangling in-arc rejected");
            }
        }
        uint64_t need = E - b.edges.size();
        uint64_t avail = ns * (V - 1) - b.edges.size();
        auto no_weight = [](size_t) {};
        if (need > 0 && need * 2 > avail) {
            fill_by_enumeration(
                b, need, rng,
                [&](auto emit) {
                    for (uint64_t u = 0; u < V; ++u) {
                        if (dangling[u]) continue;
                        for (uint64_t v = 0; v < V; ++v)
                            if (v != u) emit(u, v);
                    }
                },
                no_weight);
        } else {
            while (b.edges.size() < E) {
                uint64_t s = S[rng.below(ns)];
                uint64_t t;
                if (rng.next() & 1) {
                    double r = rng.unit();
                    double x = static_cast<double>(V) * r;
                    x = x * r;
                    uint64_t idx = static_cast<uint64_t>(x);
                    if (idx >= V) idx = V - 1;
                    t = pop[idx];
                } else {
                    t = rng.below(V);
                }
                b.add(s, t);
            }
        }
        Csr c = build_csr(b);
        Writer w(out, 1u << 22);
        write_adjacency(w, b, c, false);
        w.str("DAMPING 0.85\nTOLERANCE 0.000001\nMAX_ITERATIONS 1000\n");
        w.close();

        std::vector<uint64_t> indeg(static_cast<size_t>(V), 0);
        for (const Edge& e : b.edges) ++indeg[e.v];
        uint64_t max_out = 0, max_in = 0, min_in = UINT64_MAX, dang = 0, max_in_v = 0;
        for (uint64_t u = 0; u < V; ++u) {
            uint64_t d = c.off[u + 1] - c.off[u];
            if (d == 0) ++dang;
            if (d > max_out) max_out = d;
            if (indeg[u] > max_in) {
                max_in = indeg[u];
                max_in_v = u;
            }
            if (indeg[u] < min_in) min_in = indeg[u];
        }
        std::printf("pagerank V=%llu E=%llu dangling=%llu max_out_degree=%llu max_in_degree=%llu (vertex %llu) "
                    "min_in_degree=%llu weak_components=%llu seed=%llu -> %s\n",
                    (unsigned long long)V, (unsigned long long)E, (unsigned long long)dang,
                    (unsigned long long)max_out, (unsigned long long)max_in, (unsigned long long)max_in_v,
                    (unsigned long long)min_in, (unsigned long long)count_components(b), (unsigned long long)seed, out);
        return 0;
    }

    // maxflow
    if (V < 2) die("maxflow needs V >= 2 (source 0 and sink V-1 must differ)");
    if (E < V - 1) die("maxflow needs E >= V-1 = " + std::to_string(V - 1) + " to guarantee a source-sink path");
    if (E > directed_max) die("E=" + std::to_string(E) + " exceeds V(V-1) = " + std::to_string(directed_max));
    const uint64_t sink = V - 1;
    Builder b(V, true, E);
    auto cap = [&](size_t idx) { b.edges[idx].w = static_cast<int32_t>(1 + rng.below(100)); };
    {
        std::vector<uint32_t> q;
        for (uint64_t i = 1; i + 1 < V; ++i) q.push_back(static_cast<uint32_t>(i));
        for (uint64_t i = q.size(); i >= 2; --i) std::swap(q[i - 1], q[rng.below(i)]);
        uint64_t prev = 0;
        for (uint32_t x : q) {
            if (!b.add(prev, x)) die("internal error: path arc rejected");
            cap(b.edges.size() - 1);
            prev = x;
        }
        if (!b.add(prev, sink)) die("internal error: path arc rejected");
        cap(b.edges.size() - 1);
    }
    uint64_t k = isqrt(V);
    uint64_t rem = E - b.edges.size();
    uint64_t k_src = k;
    if (k_src > V - 2) k_src = V - 2;
    if (k_src > rem) k_src = rem;
    for (uint64_t added = 0; added < k_src;) {
        uint64_t t = 1 + rng.below(V - 1);
        if (b.add(0, t)) {
            cap(b.edges.size() - 1);
            ++added;
        }
    }
    rem = E - b.edges.size();
    uint64_t sink_in = 0;
    for (const Edge& e : b.edges)
        if (e.v == sink) ++sink_in;
    uint64_t k_sink = k;
    if (k_sink > (V - 1) - sink_in) k_sink = (V - 1) - sink_in;
    if (k_sink > rem) k_sink = rem;
    for (uint64_t added = 0; added < k_sink;) {
        uint64_t u = rng.below(V - 1);
        if (b.add(u, sink)) {
            cap(b.edges.size() - 1);
            ++added;
        }
    }
    uint64_t need = E - b.edges.size();
    uint64_t avail = directed_max - b.edges.size();
    if (need > 0 && need * 2 > avail) {
        fill_by_enumeration(
            b, need, rng,
            [&](auto emit) {
                for (uint64_t u = 0; u < V; ++u)
                    for (uint64_t v = 0; v < V; ++v)
                        if (u != v) emit(u, v);
            },
            cap);
    } else {
        while (b.edges.size() < E) {
            uint64_t u = rng.below(V);
            uint64_t v = rng.below(V);
            if (b.add(u, v)) cap(b.edges.size() - 1);
        }
    }
    Csr c = build_csr(b);
    Writer w(out, 1u << 22);
    write_adjacency(w, b, c, true);
    w.reserve(64);
    w.str("SOURCE 0\nSINK ");
    w.reserve(32);
    w.u64(sink);
    w.ch('\n');
    w.close();

    uint64_t src_cap = 0, sink_cap = 0, src_out = 0, snk_in = 0, max_out = 0;
    for (const Edge& e : b.edges) {
        if (e.u == 0) {
            src_cap += static_cast<uint64_t>(e.w);
            ++src_out;
        }
        if (e.v == sink) {
            sink_cap += static_cast<uint64_t>(e.w);
            ++snk_in;
        }
    }
    for (uint64_t u = 0; u < V; ++u)
        if (c.off[u + 1] - c.off[u] > max_out) max_out = c.off[u + 1] - c.off[u];
    std::printf("maxflow V=%llu E=%llu source=0 sink=%llu source_out_arcs=%llu sink_in_arcs=%llu "
                "flow_upper_bound=%llu max_out_degree=%llu seed=%llu -> %s\n",
                (unsigned long long)V, (unsigned long long)E, (unsigned long long)sink, (unsigned long long)src_out,
                (unsigned long long)snk_in, (unsigned long long)(src_cap < sink_cap ? src_cap : sink_cap),
                (unsigned long long)max_out, (unsigned long long)seed, out);
    return 0;
}
