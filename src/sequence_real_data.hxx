#include <algorithm>
#include <random>
#include <set>
#include <iostream>
#include <fstream>
#include <vector>

#include "sequence.hxx"

template <class UINT = unsigned, UINT RANGE = 16> class DataSequence : public Sequence<UINT, RANGE> {
    std::uniform_int_distribution<UINT> dist;
    std::ranlux48 rng;
    std::set<UINT> partition;

    std::vector<UINT> data;
private:
    bool with_file_input = false;
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
    DataSequence(const unsigned num_strands)
        : Sequence<UINT, RANGE>(num_strands), rng(rd()), dist(1, (1 << RANGE) - 1) {
        init(num_strands);
    }
    DataSequence(const unsigned num_strands, std::string &filename)
        : Sequence<UINT, RANGE>(num_strands), rng(rd()), dist(1, (1 << RANGE) - 1) {


        std::ifstream fhandle(filename.c_str());

        if (!fhandle.is_open()) {
            std::cout << "failed to open file" << filename << std::endl;
            std::cout << "rollback to simulation"  << std::endl;
            init(num_strands);
            return;
        }

        std::string line;
        unsigned int strand_counter = 0;

        unsigned int sequence_counter = 0;


        while (std::getline(fhandle, line)) {
            if (line[0] != '=')
            {
                UINT number = 0;
                UINT base = 0;
                for (int i = 0; i < RANGE / 2; i++)
                {
                    switch (line[i])
                    {
                    case 'A':
                        base =  0b00;
                        break;
                    case 'T':
                        base =  0b01;
                        break;
                    case 'G':
                        base =  0b10;
                        break;
                    case 'C':
                        base =  0b11;
                        break;
                    }
                    number |= (base << (((RANGE / 2) - 1 - i) << 1));
                }
                data.push_back(number % RANGE); // only care corrsponding number of bits
                sequence_counter ++;
            }
            else
            {
                // the same sequences are grouped and spilt by '='
                strand_counter ++;
                if (strand_counter >= num_strands)
                    break;
            }

        }
        fhandle.close();
        std::uniform_int_distribution<UINT> sampling_distribution(0, sequence_counter - 1);
        dist = sampling_distribution;
        with_file_input = true;

    }
    double entropy() override {
        if (with_file_input){
            std::cout << "entropy calculation for real data is not implemented" << std::endl;
            return 0.0;
        }

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
        if (with_file_input){
            UINT rn_index = dist(rd);
            return data[rn_index];
        }
        else{
            return std::distance(
                   partition.begin(),
                   std::lower_bound(partition.begin(), partition.end(), dist(rng)));
        }
    }
};

