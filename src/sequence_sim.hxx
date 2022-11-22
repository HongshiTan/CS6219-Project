#include <algorithm>
#include <random>
#include <set>

#include "sequence.hxx"

template <class UINT = unsigned, UINT RANGE = 16> class SimSequence : public Sequence<UINT, RANGE> {
    std::uniform_int_distribution<UINT> dist;

    std::ranlux48 rng;

    std::set<UINT> partition;
private:
    void init(const unsigned num_strands)
    {
        // sample a distribution
        partition.insert(1 << RANGE);
        while (partition.size() < num_strands) {
            partition.insert(dist(rng));
        }
    }

public:

    //using BaseSequence = Sequence<UINT, RANGE>;
    SimSequence(const unsigned num_strands)
        : Sequence<UINT, RANGE>(num_strands), rng(rd()), dist(1, (1 << RANGE) - 1) {
        init(num_strands);
    }
    SimSequence(const unsigned num_strands, std::string &filename): Sequence<UINT, RANGE>(num_strands) , rng(rd()), dist(1, (1 << RANGE) - 1) {
        init(num_strands);
    }
    double entropy() override {
        double entropy = 0;

        UINT prev      = 0;
        for (UINT p : partition) {
            double prob = double(p - prev) / (1 << RANGE);
            entropy -= prob * std::log2(prob);
            prev = p;
        }

        return entropy;
    }

    UINT operator()() override {
        return std::distance(
                   partition.begin(),
                   std::lower_bound(partition.begin(), partition.end(), dist(rng)));
    }
};

