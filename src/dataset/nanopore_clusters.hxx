#pragma once

#include <fstream>
#include <string>

#include "common.hxx"

namespace dataset {

// Loader for the Microsoft clustered-nanopore-reads format:
//   Centers.txt:  one reference strand per line (cluster i's ground truth).
//   Clusters.txt: blocks of noisy reads, blocks separated by lines that start
//                 with '='. The k-th block corresponds to the k-th center.
class NanoporeClustersLoader : public DatasetLoader {
  public:
    bool load(const std::string &dir,
              const std::map<std::string, std::string> &manifest,
              PrimerReads &out) override {
        auto get = [&](const char *k, const char *dflt) -> std::string {
            auto it = manifest.find(k);
            return it == manifest.end() ? dflt : it->second;
        };
        std::string centers_path  = dir + "/" + get("centers",  "Centers.txt");
        std::string clusters_path = dir + "/" + get("clusters", "Clusters.txt");

        std::ifstream fc(centers_path), fs(clusters_path);
        if (!fc.is_open()) {
            std::cerr << "cannot open " << centers_path << std::endl;
            return false;
        }
        if (!fs.is_open()) {
            std::cerr << "cannot open " << clusters_path << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(fc, line)) {
            rtrim(line);
            if (!line.empty()) out.primer_storage.push_back(std::move(line));
        }
        if (out.primer_storage.empty()) return false;

        std::size_t cidx = 0;
        bool seen_first_delim = false;
        while (std::getline(fs, line)) {
            rtrim(line);
            if (!line.empty() && line[0] == '=') {
                if (seen_first_delim) {
                    cidx++;
                    if (cidx >= out.primer_storage.size()) break;
                }
                seen_first_delim = true;
                continue;
            }
            if (line.empty()) continue;
            ReadRef rr;
            rr.primer     = out.primer_storage[cidx].data();
            rr.primer_len = out.primer_storage[cidx].size();
            rr.read       = std::move(line);
            out.reads.push_back(std::move(rr));
        }
        return !out.reads.empty();
    }
};

inline std::unique_ptr<DatasetLoader> make_nanopore_clusters_loader() {
    return std::make_unique<NanoporeClustersLoader>();
}

}  // namespace dataset
