// gen_points - reproducible K-Means input generator (Gaussian-like blobs) for CS509 Assignment 4.
//
// Usage: gen_points <N> <D> <K> <output-file> [seed] [max_iterations=300] [tolerance=0.0001]
//
// Output is byte-identical on every platform for a given seed (hand-written splitmix64, pure
// IEEE arithmetic, integer-based number formatting).
#include <cerrno>
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
    std::fprintf(stderr, "gen_points: %s\n", msg.c_str());
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
    // Fixed-point with 4 decimals, rounded half away from zero on the scaled value.
    void fixed4(double x) {
        bool neg = x < 0.0;
        double a = neg ? -x : x;
        double scaled = a * 10000.0;
        uint64_t q = static_cast<uint64_t>(scaled + 0.5);
        if (neg && q != 0) ch('-');
        u64(q / 10000);
        ch('.');
        uint64_t f = q % 10000;
        ch(static_cast<char>('0' + f / 1000));
        ch(static_cast<char>('0' + f / 100 % 10));
        ch(static_cast<char>('0' + f / 10 % 10));
        ch(static_cast<char>('0' + f % 10));
    }
    void str(const char* s) {
        size_t n = std::strlen(s);
        reserve(n);
        std::memcpy(&buf_[pos_], s, n);
        pos_ += n;
    }
    void flush() {
        if (pos_ != 0 && std::fwrite(buf_.data(), 1, pos_, f_) != pos_) die("write to '" + path_ + "' failed");
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

void usage() {
    std::fprintf(stderr, "usage: gen_points <N> <D> <K> <output-file> [seed] [max_iterations=300] [tolerance=0.0001]\n");
    std::exit(1);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5 || argc > 8) usage();
    uint64_t N = 0, D = 0, K = 0, seed = 1, max_iter = 300;
    std::string tol = "0.0001";
    if (!parse_u64(argv[1], N) || N < 1) die(std::string("N must be an integer >= 1, got '") + argv[1] + "'");
    if (!parse_u64(argv[2], D) || D < 1) die(std::string("D must be an integer >= 1, got '") + argv[2] + "'");
    if (!parse_u64(argv[3], K) || K < 1) die(std::string("K must be an integer >= 1, got '") + argv[3] + "'");
    const char* out = argv[4];
    if (argc >= 6 && !parse_u64(argv[5], seed)) die(std::string("seed must be a non-negative integer, got '") + argv[5] + "'");
    if (argc >= 7 && (!parse_u64(argv[6], max_iter) || max_iter < 1))
        die(std::string("max_iterations must be an integer >= 1, got '") + argv[6] + "'");
    if (argc >= 8) {
        char* end = nullptr;
        double t = std::strtod(argv[7], &end);
        if (end == argv[7] || *end != '\0' || !(t >= 0.0) || std::strchr(argv[7], 'x') || std::strchr(argv[7], 'X'))
            die(std::string("tolerance must be a non-negative number, got '") + argv[7] + "'");
        tol = argv[7];
    }
    if (K > N) die("K must be <= N");
    if (D > 1000) die("D must be <= 1000");
    if (N > 100000000ULL || N * D > 1000000000ULL) die("N*D must be <= 1e9");

    Rng rng(seed);
    std::vector<double> centers(static_cast<size_t>(K * D));
    for (uint64_t c = 0; c < K; ++c)
        for (uint64_t j = 0; j < D; ++j) {
            double u = rng.unit();
            double v = 200.0 * u;
            centers[c * D + j] = v - 100.0;
        }

    std::vector<uint64_t> sizes(static_cast<size_t>(K), 0);
    Writer w(out, 1u << 22);
    w.reserve(96);
    w.u64(N);
    w.ch(' ');
    w.u64(D);
    w.ch(' ');
    w.u64(K);
    w.ch('\n');
    for (uint64_t i = 0; i < N; ++i) {
        uint64_t blob = rng.below(K);
        ++sizes[blob];
        for (uint64_t j = 0; j < D; ++j) {
            double u1 = rng.unit();
            double u2 = rng.unit();
            double u3 = rng.unit();
            double s = u1 + u2;
            s = s + u3;
            s = s - 1.5;
            double off = 8.0 * s;
            double x = centers[blob * D + j] + off;
            w.reserve(48);
            if (j) w.ch(' ');
            w.fixed4(x);
        }
        w.ch('\n');
    }
    w.reserve(64);
    w.str("MAX_ITERATIONS ");
    w.reserve(32);
    w.u64(max_iter);
    w.ch('\n');
    w.str("TOLERANCE ");
    w.str(tol.c_str());
    w.str("\n");
    w.close();

    uint64_t smallest = UINT64_MAX, largest = 0;
    for (uint64_t s : sizes) {
        if (s < smallest) smallest = s;
        if (s > largest) largest = s;
    }
    std::printf("points N=%llu D=%llu K=%llu blobs=%llu smallest_blob=%llu largest_blob=%llu "
                "max_iterations=%llu tolerance=%s seed=%llu -> %s\n",
                (unsigned long long)N, (unsigned long long)D, (unsigned long long)K, (unsigned long long)K,
                (unsigned long long)smallest, (unsigned long long)largest, (unsigned long long)max_iter, tol.c_str(),
                (unsigned long long)seed, out);
    return 0;
}
