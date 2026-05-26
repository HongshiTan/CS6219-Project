#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "sequence.hxx"
#include "dataset/common.hxx"
#include "dataset/nanopore_clusters.hxx"
#include "dataset/fastq.hxx"
#include "dataset/fasta.hxx"

// Entropy source: per-read errors against a known DNA reference ("primer").
//
// Generic over whatever dataset loader produced the (primer, read) pairs.
// Each read is aligned against its primer (with the same INS=1/DEL=1/SUB=2
// edit-distance recipe as edit_distance_sts.py) and the traceback emits one
// UINT token per error event. The RandonBitGenerator then debiases + SHA-256-
// conditions the token stream.
template <class UINT = unsigned, UINT RANGE = 16>
class PrimerErrorSequence : public Sequence<UINT, RANGE> {
    std::vector<UINT> tokens;

  public:
    explicit PrimerErrorSequence(const std::string &dataset_dir)
        : Sequence<UINT, RANGE>(0) {

        dataset::PrimerReads pr;
        if (!dataset::load_dataset(dataset_dir, pr)) {
            std::cerr << "PrimerErrorSequence: failed to load " << dataset_dir << std::endl;
            return;
        }

        unsigned primer_len = pr.primer_len ? pr.primer_len
                                            : static_cast<unsigned>(pr.primer_storage.empty() ? 0 : pr.primer_storage[0].size());
        unsigned slack      = pr.slack ? pr.slack : 5;

        std::cout << "[" << dataset_dir << "] " << pr.description << std::endl;
        std::cout << "  reads=" << pr.reads.size()
                  << " primer_len=" << primer_len
                  << " slack=" << slack << std::endl;

        extract_errors_parallel(pr, primer_len, slack);

        std::cout << "  tokens=" << tokens.size() << std::endl;
    }

    double entropy() override {
        if (tokens.empty()) return 0.0;
        const std::size_t bins = 1u << RANGE;
        std::vector<std::uint64_t> hist(bins, 0);
        for (UINT t : tokens) hist[t & (bins - 1)]++;
        double h = 0.0;
        double n = static_cast<double>(tokens.size());
        for (auto c : hist) {
            if (!c) continue;
            double p = c / n;
            h -= p * std::log2(p);
        }
        return h;
    }

    UINT operator()() override {
        thread_local std::mt19937_64 t_rng{std::random_device{}()};
        thread_local std::uniform_int_distribution<std::size_t> t_dist(0, tokens.size() - 1);
        return tokens[t_dist(t_rng)];
    }

    std::size_t token_count() const { return tokens.size(); }

  private:
    // Edit-distance with traceback. ref = primer, read = noisy observation.
    static void align_and_tokenize(const char *ref, std::size_t ref_len,
                                   const char *read, std::size_t read_len,
                                   std::vector<UINT> &out) {
        constexpr int INS_COST = 1, DEL_COST = 1, SUB_COST = 2;
        const std::size_t R = ref_len + 1;
        const std::size_t C = read_len + 1;
        std::vector<int> dp(R * C, 0);
        auto at = [&](std::size_t i, std::size_t j) -> int & {
            return dp[i * R + j];
        };
        for (std::size_t i = 1; i < C; i++) at(i, 0) = at(i - 1, 0) + INS_COST;
        for (std::size_t j = 1; j < R; j++) at(0, j) = at(0, j - 1) + DEL_COST;
        for (std::size_t i = 1; i < C; i++) {
            for (std::size_t j = 1; j < R; j++) {
                if (ref[j - 1] == read[i - 1]) {
                    at(i, j) = at(i - 1, j - 1);
                } else {
                    int v = at(i - 1, j) + INS_COST;
                    int s = at(i - 1, j - 1) + SUB_COST;
                    int d = at(i, j - 1) + DEL_COST;
                    if (s < v) v = s;
                    if (d < v) v = d;
                    at(i, j) = v;
                }
            }
        }

        auto base_code = [](char b) -> UINT {
            switch (b) {
                case 'A': return 0;
                case 'T': return 1;
                case 'G': return 2;
                case 'C': return 3;
                default:  return 0;
            }
        };
        // Token layout (bits low to high):
        //   0..1  error type (0=sub, 1=ins, 2=del)
        //   2..7  position within primer (mod 64)
        //   8..9  base involved (sub/ins; 0 for del)
        std::size_t i = read_len, j = ref_len;
        while (i != 0 && j != 0) {
            if (ref[j - 1] == read[i - 1]) {
                i--; j--; continue;
            }
            if (at(i, j) == at(i - 1, j - 1) + SUB_COST) {
                UINT tok = 0u | ((static_cast<UINT>(j - 1) & 0x3F) << 2)
                              | (base_code(read[i - 1]) << 8);
                out.push_back(tok); i--; j--;
            } else if (at(i, j) == at(i - 1, j) + INS_COST) {
                UINT tok = 1u | ((static_cast<UINT>(j - 1) & 0x3F) << 2)
                              | (base_code(read[i - 1]) << 8);
                out.push_back(tok); i--;
            } else {
                UINT tok = 2u | ((static_cast<UINT>(j - 1) & 0x3F) << 2);
                out.push_back(tok); j--;
            }
        }
        while (j != 0) {
            UINT tok = 2u | ((static_cast<UINT>(j - 1) & 0x3F) << 2);
            out.push_back(tok); j--;
        }
        while (i != 0) {
            UINT tok = 1u | (base_code(read[i - 1]) << 8);
            out.push_back(tok); i--;
        }
    }

    void extract_errors_parallel(const dataset::PrimerReads &pr,
                                 unsigned primer_len,
                                 unsigned slack) {
        const std::size_t N = pr.reads.size();
        std::vector<std::vector<UINT>> per_thread;

#ifdef _OPENMP
        per_thread.resize(omp_get_max_threads());
#else
        per_thread.resize(1);
#endif

        #pragma omp parallel for schedule(static, 256)
        for (std::size_t i = 0; i < N; i++) {
            const dataset::ReadRef &rr = pr.reads[i];
#ifdef _OPENMP
            auto &out = per_thread[omp_get_thread_num()];
#else
            auto &out = per_thread[0];
#endif
            std::size_t plen = std::min<std::size_t>(rr.primer_len, primer_len);
            if (plen == 0) continue;
            std::size_t rlen = std::min<std::size_t>(rr.read.size(),
                                                    static_cast<std::size_t>(primer_len) + slack);
            if (rlen == 0) continue;
            align_and_tokenize(rr.primer, plen,
                               rr.read.data(), rlen, out);
        }

        std::size_t total = 0;
        for (auto &v : per_thread) total += v.size();
        tokens.reserve(total);
        for (auto &v : per_thread) {
            tokens.insert(tokens.end(), v.begin(), v.end());
            std::vector<UINT>().swap(v);
        }
    }
};
