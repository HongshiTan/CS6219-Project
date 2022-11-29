#pragma once

#include <algorithm>
#include <random>
#include <set>

static std::random_device rd;

// base class to generate baised distributed sequences
template <class UINT = unsigned, UINT RANGE = 16> class Sequence {

  public:
    Sequence(const unsigned num_strands) {
    }
    Sequence(const unsigned num_strands, std::string &filename) {
    }

    virtual double entropy() {
        std::cout << "not implemented" << std::endl;

        return 0.0;
    }

    virtual UINT operator()() {
        std::cout << "error: operator () not implemented" << std::endl;
        exit(-1);
        return 0;
    }
};
