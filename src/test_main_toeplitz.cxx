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

#include "sequence_real_data.hxx"
#include "sequence_primer_error.hxx"
#include "random_bit_generator_toeplitz.hxx"

extern "C" {
#include "TestU01.h"
}




void print_buf(unsigned char *buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %02hhx", buf[i]);
    printf("\n");
}


// Default dataset path (relative to the build directory). Override with -d.
static std::string dataset_dir   = "../dataset/clustered-nanopore-reads-dataset";
static std::string clusters_file = "../dataset/clustered-nanopore-reads-dataset/Clusters.txt";

// One of these is constructed at runtime depending on the -p flag.
static Sequence<> *g_seq = nullptr;
static RandonBitGenerator *g_rbg = nullptr;


#define  TEMP_BUFFER_SIZE  (1024 * 1024)
uint8_t buffer[TEMP_BUFFER_SIZE];

unsigned int test_function (void)
{
    static  uint32_t counter = 0;
    uint32_t index = counter % (TEMP_BUFFER_SIZE / sizeof(uint32_t));
    if (index == 0)
    {
        g_rbg->fill_parallel(buffer, TEMP_BUFFER_SIZE);
    }
    uint32_t rn = ((uint32_t *)buffer)[index];
    counter ++;
    return rn;
}


const char* progname;

void usage()
{
    printf("%s: [-v] [-p] [-d <dataset_dir>] [-s|-m|-b|-l]\n", progname);
    printf("  Toeplitz variant: GF(2) universal-hash conditioner (no SHA-256 in hot path)\n");
    printf("  -p  use primer-error entropy source\n");
    printf("  -d  dataset directory (must contain manifest.txt)\n");
    printf("  -s  SmallCrush     -m  Crush     -b  BigCrush     -l  LinComp\n");
    printf("  -v  verbose TestU01 output\n");
    exit(1);
}


int main (int argc, char** argv) {

    progname = argv[0];

    bool usePrimerErrors = false;

    // Config options for TestU01
    swrite_Basic = TRUE;  // Turn off TestU01 verbosity by default
    // reenable by -v option.

    // Config options for tests
    bool testSmallCrush = false;
    bool testCrush = false;
    bool testBigCrush = false;
    bool testLinComp = false;

    // Handle command-line option switches
    while (1) {
        --argc; ++argv;
        if ((argc == 0) || (argv[0][0] != '-'))
            break;
        if (argv[0][1] == 'd' && argv[0][2] == '\0') {
            --argc; ++argv;
            if (argc == 0) usage();
            dataset_dir = argv[0];
            continue;
        }
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
        case 'p':
            usePrimerErrors = true;
            break;
        default:
            usage();
        }
    }

    if (usePrimerErrors) {
        auto *ps = new PrimerErrorSequence<>(dataset_dir);
        std::cout << "primer-error 16-bit-token entropy = "
                  << ps->entropy() << std::endl;
        g_seq = ps;
    } else {
        auto *ds = new DataSequence<>(64, clusters_file);
        std::cout << "biased-sampler entropy = " << ds->entropy() << std::endl;
        g_seq = ds;
    }
    g_rbg = new RandonBitGenerator(*g_seq);

    unif01_Gen* gen = unif01_CreateExternGenBits("rbg", test_function);

#ifndef PRT
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
        scomp_Res* res = scomp_CreateRes();
        swrite_Basic = TRUE;
        for (int size : {250, 500, 1000, 5000, 25000, 50000, 75000})
            scomp_LinearComp(gen, res, 1, size, 0, 1);
        scomp_DeleteRes(res);
        fflush(stdout);
    }

    unif01_DeleteExternGenBits(gen);
#else
    freopen(NULL, "wb", stdout);  // Only necessary on Windows, but harmless.
    while (1) {
        uint32_t value = test_function()
        fwrite((void*) &value, sizeof(value), 1, stdout);
    }
#endif
}
