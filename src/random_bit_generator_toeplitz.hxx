#define SHA256_DIGEST_SIZE 32
#define SHA256_DIGEST_BITS (SHA256_DIGEST_SIZE * 8)

#define SHA256_BLOCK_BYTES 64
#define SHA256_BLOCK_WORDS (SHA256_BLOCK_BYTES / 8)
#define SHA256_BLOCK_BITS  (SHA256_BLOCK_BYTES * 8)

// Toeplitz universal-hash extractor parameters.
// M output bits, N input bits, seed is N+M-1 bits (top bit of the 96-byte
// storage is unused).
#define TOEPLITZ_M        256
#define TOEPLITZ_N        512
#define TOEPLITZ_S_BITS   (TOEPLITZ_N + TOEPLITZ_M - 1)   // 767
#define TOEPLITZ_S_BYTES  96
#define TOEPLITZ_S_WORDS  (TOEPLITZ_S_BYTES / 8)          // 12

#ifdef _OPENMP
#include <omp.h>
#endif

namespace toeplitz_detail {

// Derive the public, fixed 767-bit Toeplitz seed deterministically from
// SHA-256("DNA-TRNG-Toeplitz-Seed-v1" || k zero bytes) for k = 0, 1, 2, ...
// concatenated, taking the first 96 bytes. SHA-256 is used ONCE at static
// init; it is never on the hot path.
inline std::array<std::uint64_t, TOEPLITZ_S_WORDS> derive_seed() {
    static const char label[] = "DNA-TRNG-Toeplitz-Seed-v1";
    const size_t label_len = sizeof(label) - 1;

    std::array<std::uint8_t, TOEPLITZ_S_BYTES> bytes{};
    std::vector<std::uint8_t> msg(label, label + label_len);

    size_t off = 0;
    for (size_t k = 0; off < TOEPLITZ_S_BYTES; k++) {
        std::uint8_t digest[SHA256_DIGEST_SIZE];
        int ret = mbedtls_sha256(msg.data(), msg.size(), digest, 0);
        if (ret != 0) {
            char errbuf[BUFSIZ];
            mbedtls_strerror(ret, errbuf, BUFSIZ);
            fprintf(stderr, "toeplitz seed init: %s\n", errbuf);
            std::abort();
        }
        const size_t take = std::min<size_t>(SHA256_DIGEST_SIZE,
                                             TOEPLITZ_S_BYTES - off);
        std::memcpy(bytes.data() + off, digest, take);
        off += take;
        msg.push_back(0x00);
    }

    std::array<std::uint64_t, TOEPLITZ_S_WORDS> words{};
    std::memcpy(words.data(), bytes.data(), TOEPLITZ_S_BYTES);
    return words;
}

inline const std::array<std::uint64_t, TOEPLITZ_S_WORDS>& seed_words() {
    static const auto v = derive_seed();
    return v;
}

}  // namespace toeplitz_detail

class RandonBitGenerator {
    Sequence<> &seq;

    std::vector<std::uint8_t> entropy;

    // Toeplitz variant: replace SHA-256 with a GF(2) Toeplitz universal hash.
    // The parity-fold loop is bit-identical to the SHA-256 variant; only the
    // post-processing step changes.
    static void refresh_block(Sequence<>& seq, std::uint8_t *out) {
        static const std::bitset<SHA256_BLOCK_BITS> mask = ((std::uint64_t)-1);
        std::bitset<SHA256_BLOCK_BITS> buf               = 0;
        std::array<std::uint64_t, SHA256_BLOCK_WORDS> x_words{};

        unsigned idx = 0;
        while (idx < SHA256_DIGEST_BITS) {
            unsigned word = seq();

            while (true) {
                buf ^= (word & 1);
                word >>= 1;
                if (word == 0) {
                    break;
                }
                buf = buf << 1;
                idx++;
            }
        }

        idx = 0;
        while (buf.any() && idx < SHA256_BLOCK_WORDS) {
            x_words[idx] = (buf & mask).to_ullong();
            buf >>= std::numeric_limits<std::uint64_t>::digits;
            idx++;
        }

        // y = T * x over GF(2).  Row i of T is the 512-bit window
        // s[i .. i+511] of the public seed; y[i] = parity(window_i AND x).
        const auto& s = toeplitz_detail::seed_words();
        std::array<std::uint64_t, TOEPLITZ_M / 64> y_words{};
        for (unsigned i = 0; i < TOEPLITZ_M; i++) {
            const unsigned q = i / 64;
            const unsigned r = i % 64;
            unsigned acc = 0;
            for (unsigned j = 0; j < SHA256_BLOCK_WORDS; j++) {
                std::uint64_t w;
                if (r == 0) {
                    w = s[q + j];
                } else {
                    w = (s[q + j] >> r) | (s[q + j + 1] << (64 - r));
                }
                acc ^= __builtin_parityll(w & x_words[j]);
            }
            y_words[i / 64] |= (static_cast<std::uint64_t>(acc) & 1ULL)
                               << (i % 64);
        }
        std::memcpy(out, y_words.data(), SHA256_DIGEST_SIZE);
    }

    void refresh() {
        entropy.resize(SHA256_DIGEST_SIZE);
        refresh_block(seq, entropy.data());
    }

  public:
    RandonBitGenerator(Sequence<> &seq) : seq(seq) {
    }

    void fill(std::uint8_t *buf, size_t len) {
        if (entropy.empty()) {
            refresh();
        }

        while (entropy.size() < len) {
            memmove(buf, entropy.data(), entropy.size());
            buf += entropy.size();
            len -= entropy.size();

            entropy.clear();
            refresh();
        }

        memmove(buf, entropy.data(), len);
        entropy.erase(entropy.begin(), entropy.begin() + len);
    }

    // Parallel fill: independent 32-byte blocks are generated concurrently.
    void fill_parallel(std::uint8_t *buf, size_t len) {
        constexpr size_t BLK = SHA256_DIGEST_SIZE;
        const size_t n_full = len / BLK;
        const size_t tail   = len % BLK;

        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < n_full; i++) {
            refresh_block(seq, buf + i * BLK);
        }

        if (tail) {
            std::uint8_t tmp[BLK];
            refresh_block(seq, tmp);
            std::memcpy(buf + n_full * BLK, tmp, tail);
        }
    }
};
