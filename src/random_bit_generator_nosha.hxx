#define SHA256_DIGEST_SIZE 32
#define SHA256_DIGEST_BITS (SHA256_DIGEST_SIZE * 8)

#define SHA256_BLOCK_BYTES 64
#define SHA256_BLOCK_WORDS (SHA256_BLOCK_BYTES / 8)
#define SHA256_BLOCK_BITS  (SHA256_BLOCK_BYTES * 8)

#ifdef _OPENMP
#include <omp.h>
#endif

class RandonBitGenerator {
    Sequence<> &seq;

    std::vector<std::uint8_t> entropy;

    // No-SHA variant: emit the 256-bit parity-folded accumulator directly
    // (first 32 bytes of raw_entropy) instead of running it through SHA-256.
    // Source sampling and parity-fold loop are identical to the SHA build.
    static void refresh_block(Sequence<>& seq, std::uint8_t *out) {
        static const std::bitset<SHA256_BLOCK_BITS> mask = ((std::uint64_t)-1);
        std::bitset<SHA256_BLOCK_BITS> buf               = 0;
        std::array<std::uint64_t, SHA256_BLOCK_WORDS> raw_entropy{};

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
            raw_entropy[idx] = (buf & mask).to_ullong();
            buf >>= std::numeric_limits<std::uint64_t>::digits;
            idx++;
        }

        std::memcpy(out, raw_entropy.data(), SHA256_DIGEST_SIZE);
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
