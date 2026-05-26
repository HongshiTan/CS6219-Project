#pragma once

#include <fstream>
#include <string>

#include "common.hxx"

namespace dataset {

// FASTQ loader: four lines per record (@id, seq, +, qual). We only need the
// seq line. The primer is read from the manifest (single primer shared by
// every read).
class FastqLoader : public DatasetLoader {
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
            std::cerr << "fastq loader requires 'primer:' in manifest" << std::endl;
            return false;
        }
        out.primer_storage.push_back(canonicalize_primer(primer));
        const char *p_ptr   = out.primer_storage.back().data();
        std::size_t p_len   = out.primer_storage.back().size();

        std::ifstream f(reads_path);
        if (!f.is_open()) {
            std::cerr << "cannot open " << reads_path << std::endl;
            return false;
        }

        // How many bases of the read we want to keep. primer_len from manifest
        // may not be parsed yet (it's filled later by load_dataset), so we
        // compute a generous cap here and let the entropy extractor truncate.
        std::size_t keep = p_len + 16;  // primer + plenty of slack

        std::string line;
        std::size_t state = 0;
        while (std::getline(f, line)) {
            rtrim(line);
            if (line.empty()) continue;
            if (state == 0) {
                if (line[0] != '@') continue;  // resync on next @
                state = 1;
            } else if (state == 1) {
                // sequence line
                if (line.size() > keep) line.resize(keep);
                ReadRef rr;
                rr.primer     = p_ptr;
                rr.primer_len = p_len;
                rr.read       = std::move(line);
                out.reads.push_back(std::move(rr));
                state = 2;
            } else if (state == 2) {
                if (line[0] == '+') state = 3;
            } else {  // state == 3, quality line
                state = 0;
            }
        }
        return !out.reads.empty();
    }
};

inline std::unique_ptr<DatasetLoader> make_fastq_loader() {
    return std::make_unique<FastqLoader>();
}

}  // namespace dataset
