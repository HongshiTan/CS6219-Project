#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace dataset {

// One read paired with its reference primer. `primer` may either point into
// a single shared primer string (for amplicon-style datasets) or into a
// per-read reference (for clustered-nanopore-reads style datasets). We don't
// own the storage; the loader keeps it alive.
struct ReadRef {
    const char *primer;
    std::size_t primer_len;
    std::string read;        // owned; first `primer_len + slack` bp suffice
};

// Unified container produced by every loader. `reads` is what the entropy
// extractor iterates over. `primer_storage` owns the actual primer strings
// that ReadRef::primer points into.
struct PrimerReads {
    std::vector<std::string> primer_storage;
    std::vector<ReadRef> reads;
    unsigned primer_len = 0;
    unsigned slack      = 0;
    std::string description;
};

// Trim ASCII whitespace from both ends in place.
inline void rtrim(std::string &s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                          s.back() == ' '  || s.back() == '\t'))
        s.pop_back();
}

// Parse a manifest.txt file into a map of key -> value. Lines beginning with
// '#' or empty lines are skipped. Format: "key: value".
inline std::map<std::string, std::string> parse_manifest(const std::string &path) {
    std::map<std::string, std::string> m;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "manifest not found: " << path << std::endl;
        return m;
    }
    std::string line;
    while (std::getline(f, line)) {
        rtrim(line);
        if (line.empty() || line[0] == '#') continue;
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string k = line.substr(0, colon);
        std::string v = line.substr(colon + 1);
        // ltrim v
        std::size_t s = 0;
        while (s < v.size() && (v[s] == ' ' || v[s] == '\t')) s++;
        v.erase(0, s);
        m[k] = v;
    }
    return m;
}

// Convert IUPAC-degenerate bases to a non-degenerate canonical form (picks the
// first matching ACGT). Good enough for alignment scoring.
inline char degenerate_to_acgt(char c) {
    switch (c) {
        case 'A': case 'C': case 'G': case 'T': return c;
        case 'M': return 'A';   // A/C
        case 'R': return 'A';   // A/G
        case 'W': return 'A';   // A/T
        case 'S': return 'C';   // C/G
        case 'Y': return 'C';   // C/T
        case 'K': return 'G';   // G/T
        case 'V': return 'A';   // A/C/G
        case 'H': return 'A';   // A/C/T
        case 'D': return 'A';   // A/G/T
        case 'B': return 'C';   // C/G/T
        case 'N': return 'A';
        default:  return c;
    }
}

inline std::string canonicalize_primer(std::string p) {
    for (auto &c : p) c = degenerate_to_acgt(c);
    return p;
}

// Abstract loader. Each concrete subclass parses one dataset format into
// PrimerReads.
class DatasetLoader {
  public:
    virtual ~DatasetLoader() = default;
    virtual bool load(const std::string &dir,
                      const std::map<std::string, std::string> &manifest,
                      PrimerReads &out) = 0;
};

// Forward decls for factory. Implementations live in the per-format headers.
std::unique_ptr<DatasetLoader> make_nanopore_clusters_loader();
std::unique_ptr<DatasetLoader> make_fastq_loader();
std::unique_ptr<DatasetLoader> make_fasta_loader();

// Build a loader for the dataset rooted at `dataset_dir`. Reads
// `<dir>/manifest.txt`, looks at `loader:`, and dispatches.
inline std::unique_ptr<DatasetLoader> make_loader_for(const std::string &dataset_dir,
                                                     std::map<std::string, std::string> &manifest_out) {
    manifest_out = parse_manifest(dataset_dir + "/manifest.txt");
    auto it = manifest_out.find("loader");
    if (it == manifest_out.end()) {
        std::cerr << "manifest is missing 'loader:' key" << std::endl;
        return nullptr;
    }
    const std::string &kind = it->second;
    if (kind == "nanopore_clusters") return make_nanopore_clusters_loader();
    if (kind == "fastq")             return make_fastq_loader();
    if (kind == "fasta")             return make_fasta_loader();
    std::cerr << "unknown loader kind: " << kind << std::endl;
    return nullptr;
}

// Top-level convenience: load a dataset directory into a PrimerReads.
inline bool load_dataset(const std::string &dataset_dir, PrimerReads &out) {
    std::map<std::string, std::string> manifest;
    auto loader = make_loader_for(dataset_dir, manifest);
    if (!loader) return false;
    bool ok = loader->load(dataset_dir, manifest, out);
    if (!ok) return false;
    auto pl = manifest.find("primer_len");
    auto sl = manifest.find("slack");
    if (pl != manifest.end()) out.primer_len = std::stoul(pl->second);
    if (sl != manifest.end()) out.slack      = std::stoul(sl->second);
    auto dc = manifest.find("description");
    if (dc != manifest.end()) out.description = dc->second;
    return true;
}

}  // namespace dataset
