#define SHA256_DIGEST_SIZE 32
#define SHA256_DIGEST_BITS (SHA256_DIGEST_SIZE * 8)

#define SHA256_BLOCK_BYTES 64
#define SHA256_BLOCK_WORDS (SHA256_BLOCK_BYTES / 8)
#define SHA256_BLOCK_BITS  (SHA256_BLOCK_BYTES * 8)

class RandonBitGenerator {
    Sequence<> &seq;

    std::vector<std::uint8_t> entropy;

    void refresh() {
        static const std::bitset<SHA256_BLOCK_BITS> mask = ((std::uint64_t)-1);
        std::bitset<SHA256_BLOCK_BITS> buf               = 0;
        std::array<std::uint64_t, SHA256_BLOCK_WORDS> raw_entropy;

        entropy.resize(SHA256_DIGEST_SIZE);

        // retrieve and conditioning entropy source from sequencing
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

        // fill-in the block with the bits
        idx = 0;
        while (buf.any() && idx < SHA256_BLOCK_WORDS) {
            raw_entropy[idx] = (buf & mask).to_ullong();
            buf >>= std::numeric_limits<std::uint64_t>::digits;
            idx++;
        }

        // do conditioning
        int ret = mbedtls_sha256(
            reinterpret_cast<const std::uint8_t *>(raw_entropy.data()),
            SHA256_BLOCK_BYTES, &entropy[0], 0);
        if (ret != 0) {
            char buf[BUFSIZ];
            mbedtls_strerror(ret, buf, BUFSIZ);
            fprintf(stderr, "mbedtls_sha256: %s\n", buf);
            abort();
        }
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
};
