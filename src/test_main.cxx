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

#include "random_bit_generator.hxx"
#include "sequence_real_data.hxx"

extern "C" {
#include "TestU01.h"
}

void print_buf(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %02hhx", buf[i]);
    printf("\n");
}

std::string file_name =
    "../dataset/clustered-nanopore-reads-dataset/Clusters.txt";
DataSequence seq(64, file_name);

RandonBitGenerator rbg(seq);

#define TEMP_BUFFER_SIZE (1024 * 1024)
uint8_t buffer[TEMP_BUFFER_SIZE];

unsigned int test_function(void) {
    static uint32_t counter = 0;
    uint32_t index          = counter % (TEMP_BUFFER_SIZE / sizeof(uint32_t));
    if (index == 0) {
        rbg.fill(buffer, TEMP_BUFFER_SIZE);
    }
    uint32_t rn = ((uint32_t *)buffer)[index];
    counter++;
    return rn;
}

const char *progname;

void usage() {
    printf("%s: [-v] [-r] [seeds...]\n", progname);
    exit(1);
}

int main(int argc, char **argv) {

    std::cout << seq.entropy() << std::endl;

    progname = argv[0];

    unif01_Gen *gen = unif01_CreateExternGenBits("rbg", test_function);

    // Config options for TestU01
    swrite_Basic = TRUE; // Turn off TestU01 verbosity by default
    // reenable by -v option.

    // Config options for tests
    bool testSmallCrush = false;
    bool testCrush      = false;
    bool testBigCrush   = false;
    bool testLinComp    = false;

    // Handle command-line option switches
    while (1) {
        --argc;
        ++argv;
        if ((argc == 0) || (argv[0][0] != '-'))
            break;
        if ((argv[0][1] == '\0') || (argv[0][2] != '\0'))
            usage();
        switch (argv[0][1]) {
            case 's':
                testSmallCrush = true;
                break;
            case 'm':
                testCrush = true;
                break;
            case 'b':
                testBigCrush = true;
                break;
            case 'l':
                testLinComp = true;
                break;
            case 'v':
                swrite_Basic = TRUE;
                break;
            default:
                usage();
        }
    }
    // printf("rng test of %s, with co_num %d\n",
    // local_method[SELECTED_METHOD].name, CO_NUM);

#ifndef PRT
    // Determine a default test if need be

    if (!(testSmallCrush || testCrush || testBigCrush || testLinComp)) {
        testCrush = true;
    }

    if (testSmallCrush) {
        bbattery_SmallCrush(gen);
        fflush(stdout);
    }
    if (testCrush) {
        bbattery_Crush(gen);
        fflush(stdout);
    }
    if (testBigCrush) {
        bbattery_BigCrush(gen);
        fflush(stdout);
    }
    if (testLinComp) {
        scomp_Res *res = scomp_CreateRes();
        swrite_Basic   = TRUE;
        for (int size : {250, 500, 1000, 5000, 25000, 50000, 75000})
            scomp_LinearComp(gen, res, 1, size, 0, 1);
        scomp_DeleteRes(res);
        fflush(stdout);
    }

    // Clean up.

    unif01_DeleteExternGenBits(gen);
#else
    freopen(NULL, "wb", stdout); // Only necessary on Windows, but harmless.
    while (1) {
        uint32_t value =
            test_function() fwrite((void *)&value, sizeof(value), 1, stdout);
    }
#endif
}
