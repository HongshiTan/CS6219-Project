#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <mbedtls/error.h>
#include <mbedtls/sha256.h>

#include "sequence_sim.hxx"
#include "sequence_real_data.hxx"
#include "random_bit_generator.hxx"

#define SHA256_DIGEST_SIZE 32
#define SHA256_DIGEST_BITS (SHA256_DIGEST_SIZE * 8)

#define SHA256_BLOCK_BYTES 64
#define SHA256_BLOCK_WORDS (SHA256_BLOCK_BYTES / 8)
#define SHA256_BLOCK_BITS  (SHA256_BLOCK_BYTES * 8)


void print_buf(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %02hhx", buf[i]);
    printf("\n");
}

int main (int argc, char** argv) {
    std::string filename;
    if (argc > 1)
    {
        filename = argv[1];
    }

    DataSequence seq(16, filename);
    std::cout << seq.entropy() << std::endl;
#if 1
    RandonBitGenerator rbg(seq);

    std::uint8_t rnd[BUFSIZ];
    rbg.fill(rnd, BUFSIZ);

    unsigned cnt[256] = {0};
    for (auto c : rnd) {
        cnt[c]++;
    }
    double entropy = 0;
    for (auto c : cnt) {
        double prob = double(c) / double(BUFSIZ);
        entropy -= prob * std::log2(prob);
    }

    std::cout << entropy << std::endl;
#endif
    return 0;
}
