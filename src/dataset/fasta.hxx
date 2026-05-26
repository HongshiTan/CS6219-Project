#pragma once

#include <fstream>
#include <string>

#include "common.hxx"

namespace dataset {

// FASTA loader: '>' header line, then one or more sequence lines (concatenated
// until the next '>' or EOF). One known primer shared by all reads.
class FastaLoader : public DatasetLoader {
  public:
    bool load(const std::string &dir,
              const std::map<std::string, std::string> &manifest,
              PrimerReads &out) override {
        auto get = [&](const char *k) -> std::string {
            auto it = manifest.find(k);
            return it == manifest.end() ? std::string() : it->second;
        };

        std::string reads_path = dir + "/" + get("reads");
        std::string primer     = get("primer");
        if (primer.empty()) {
            std::cerr << "fasta loader requires 'primer:' in manifest" << std::endl;
            return false;
        }
        out.primer_storage.push_back(canonicalize_primer(primer));
        const char *p_ptr   = out.primer_storage.back().data();
        std::size_t p_len   = out.primer_storage.back().size();
        std::size_t keep    = p_len + 16;

        std::ifstream f(reads_path);
        if (!f.is_open()) {
            std::cerr << "cannot open " << reads_path << std::endl;
            return false;
        }

        std::string line, seq;
        auto flush = [&]() {
            if (seq.empty()) return;
            if (seq.size() > keep) seq.resize(keep);
            ReadRef rr;
            rr.primer     = p_ptr;
            rr.primer_len = p_len;
            rr.read       = std::move(seq);
            out.reads.push_back(std::move(rr));
            seq.clear();
        };

        while (std::getline(f, line)) {
            rtrim(line);
            if (line.empty()) continue;
            if (line[0] == '>') {
                flush();
            } else {
                if (seq.size() < keep) {
                    std::size_t take = std::min(line.size(), keep - seq.size());
                    seq.append(line, 0, take);
                }
            }
        }
        flush();
        return !out.reads.empty();
    }
};

inline std::unique_ptr<DatasetLoader> make_fasta_loader() {
    return std::make_unique<FastaLoader>();
}

}  // namespace dataset
