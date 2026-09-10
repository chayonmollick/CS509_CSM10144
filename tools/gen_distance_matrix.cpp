// gen_distance_matrix - reproducible FastMap input generator for CS509 Assignment 4.
//
// Usage: gen_distance_matrix <N> <K> <output-file> [seed]
//
// Objects are random points in a 4-D latent space with axis ranges [0,100], [0,60], [0,30],
// [0,10]; the file holds their exact Euclidean distance matrix with 2 decimals. Only the points
// are kept in memory and rows are streamed, so N = 10,000 (1e8 numbers) is fine.
// Output is byte-identical on every platform for a given seed.
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef __clang__
#pragma clang fp contract(off)
#endif

namespace {

[[noreturn]] void die(const std::string& msg) {
    std::fprintf(stderr, "gen_distance_matrix: %s\n", msg.c_str());
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
    double unit() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }
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

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::fprintf(stderr, "usage: gen_distance_matrix <N> <K> <output-file> [seed]   (1 <= K < N)\n");
        return 1;
    }
    uint64_t N = 0, K = 0, seed = 1;
    if (!parse_u64(argv[1], N)) die(std::string("N must be a non-negative integer, got '") + argv[1] + "'");
    if (!parse_u64(argv[2], K)) die(std::string("K must be a non-negative integer, got '") + argv[2] + "'");
    const char* out = argv[3];
    if (argc == 5 && !parse_u64(argv[4], seed)) die(std::string("seed must be a non-negative integer, got '") + argv[4] + "'");
    if (K < 1) die("K must be >= 1");
    if (K >= N) die("K must be < N");
    if (N > 60000) die("N must be <= 60000 (the matrix would exceed ~25 GB)");

    static const double RANGE[4] = {100.0, 60.0, 30.0, 10.0};
    Rng rng(seed);
    std::vector<double> p(static_cast<size_t>(N * 4));
    for (uint64_t i = 0; i < N; ++i)
        for (int a = 0; a < 4; ++a) p[i * 4 + static_cast<uint64_t>(a)] = RANGE[a] * rng.unit();

    FILE* f = std::fopen(out, "wb");
    if (!f) die(std::string("cannot open output file '") + out + "': " + std::strerror(errno));
    const size_t CAP = size_t(1) << 25;
    std::vector<char> buf(CAP);
    char* b = buf.data();
    size_t pos = 0;
    auto flush = [&]() {
        if (pos != 0 && std::fwrite(b, 1, pos, f) != pos) die(std::string("write to '") + out + "' failed");
        pos = 0;
    };
    {
        char tmp[64];
        int n = std::snprintf(tmp, sizeof tmp, "%llu %llu\n", (unsigned long long)N, (unsigned long long)K);
        std::memcpy(b + pos, tmp, static_cast<size_t>(n));
        pos += static_cast<size_t>(n);
    }

    double dmin = 0.0, dmax = 0.0, dsum = 0.0;
    bool first = true;
    for (uint64_t i = 0; i < N; ++i) {
        if (pos + N * 8 + 16 > CAP) flush();
        const double* pi = &p[i * 4];
        for (uint64_t j = 0; j < N; ++j) {
            if (j) b[pos++] = ' ';
            if (i == j) {
                std::memcpy(b + pos, "0.00", 4);
                pos += 4;
                continue;
            }
            // Canonical (min,max) ordering so (i,j) and (j,i) are computed identically.
            const double* pa = i < j ? pi : &p[j * 4];
            const double* pb = i < j ? &p[j * 4] : pi;
            double d0 = pa[0] - pb[0];
            double d1 = pa[1] - pb[1];
            double d2 = pa[2] - pb[2];
            double d3 = pa[3] - pb[3];
            double ss = d0 * d0;
            ss = ss + d1 * d1;
            ss = ss + d2 * d2;
            ss = ss + d3 * d3;
            double d = std::sqrt(ss);
            if (i < j) {
                if (first || d < dmin) dmin = d;
                if (first || d > dmax) dmax = d;
                first = false;
                dsum += d;
            }
            double scaled = d * 100.0;
            uint32_t q = static_cast<uint32_t>(scaled + 0.5);
            uint32_t ip = q / 100;
            uint32_t fp = q % 100;
            if (ip >= 100) {
                b[pos++] = static_cast<char>('0' + ip / 100);
                b[pos++] = static_cast<char>('0' + ip / 10 % 10);
                b[pos++] = static_cast<char>('0' + ip % 10);
            } else if (ip >= 10) {
                b[pos++] = static_cast<char>('0' + ip / 10);
                b[pos++] = static_cast<char>('0' + ip % 10);
            } else {
                b[pos++] = static_cast<char>('0' + ip);
            }
            b[pos++] = '.';
            b[pos++] = static_cast<char>('0' + fp / 10);
            b[pos++] = static_cast<char>('0' + fp % 10);
        }
        b[pos++] = '\n';
    }
    flush();
    if (std::fclose(f) != 0) die(std::string("closing '") + out + "' failed");

    double pairs = static_cast<double>(N) * static_cast<double>(N - 1) / 2.0;
    std::printf("distance_matrix N=%llu K=%llu latent_dims=4 (ranges 100/60/30/10) min_offdiag=%.2f max=%.2f "
                "mean=%.2f seed=%llu -> %s\n",
                (unsigned long long)N, (unsigned long long)K, dmin, dmax, dsum / pairs, (unsigned long long)seed, out);
    return 0;
}
