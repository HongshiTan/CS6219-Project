#pragma once

#include <algorithm>
#include <random>
#include <set>

static std::random_device rd;

template <class UINT = unsigned, UINT RANGE = 16> class Sequence {
    std::uniform_int_distribution<UINT> dist;

    std::set<UINT> partition;
    std::ranlux48 rng;

  public:
    Sequence(const unsigned num_strands)
        : rng(rd()), dist(1, (1 << RANGE) - 1) {

        // sample a distribution
        partition.insert(1 << RANGE);
        while (partition.size() < num_strands) {
            partition.insert(dist(rng));
        }
    }

    double entropy() const {
        double entropy = 0;

        UINT prev      = 0;
        for (UINT p : partition) {
            double prob = double(p - prev) / (1 << RANGE);
            entropy -= prob * std::log2(prob);
            prev = p;
        }

        return entropy;
    }

    UINT operator()() {
        return std::distance(
            partition.begin(),
            std::lower_bound(partition.begin(), partition.end(), dist(rng)));
    }
};
