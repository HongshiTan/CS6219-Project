# Running the DNA-primer TRNG

End-to-end build, fetch, and TestU01 validation across five real DNA primer
datasets. Quick path for a fresh clone:

```bash
# 1. Build third-party deps + project
./bootstrap.sh

# 2. Fetch all datasets (skip the giants with SKIP=...)
cd dataset
SKIP=loman-zymo-r103,loman-zymo-r10pcr ./fetch_all.sh

# 3. Run TestU01 SmallCrush against every fetched dataset
./test_all.sh
```

Everything below is the longer story.

---

## 1. What this does

For each cluster/center pair, or each amplicon read against its known PCR
primer, we:

1. Align the read prefix to the primer with Needleman-Wunsch
   (INS=1, DEL=1, SUB=2 — same recipe as `edit_distance_sts.py`).
2. Tokenize each error event (substitute / insert / delete + position + base)
   into a 16-bit word.
3. Pull tokens with a thread-local RNG to drive `RandonBitGenerator`, which
   parity-folds 256 bits worth of token bits into a 512-bit accumulator and
   runs SHA-256 to emit each 32-byte random block.
4. Hand the byte stream to TestU01 via `unif01_CreateExternGenBits`.

The token stream itself is biased (≈5.5 bits of entropy per 16-bit token on
nanopore data) but von-Neumann debias + SHA-256 conditioning produces a stream
that passes SmallCrush on every dataset.

---

## 2. Build

### Prerequisites (Ubuntu / Debian)

```bash
sudo apt install build-essential cmake git autoconf automake libtool \
                  python3 python3-pip unzip curl
```

### Third-party deps

**TestU01.** The repo's `3rd-party/testu01_install.sh` points at the original
Montréal URL which now 404s. Use the GitHub mirror instead:

```bash
cd 3rd-party
git clone --depth 1 https://github.com/umontreal-simul/TestU01-2009.git TestU01-src
cd TestU01-src
autoreconf -fi          # system autotools (2.71) regenerates configure
./configure --prefix="$(cd .. && pwd)/TestU01"
make -j install
```

**mbedtls.** The library uses Python to generate sources, so do this once
before CMake:

```bash
cd 3rd-party/mbedtls-3.2.1
make -j                 # generates .c files into library/
```

### Project

```bash
mkdir -p build && cd build
cmake ..
make -j rbg rbg_test rbg_test_nosha
```

You'll get three binaries in `build/`:

| Binary | Source | Purpose |
|---|---|---|
| `rbg`             | `src/main.cxx`          | Standalone fill demo, prints entropy of one buffer. |
| `rbg_test`        | `src/test_main.cxx`     | TestU01 driver with full SHA-256 conditioning. |
| `rbg_test_nosha`  | `src/test_main_nosha.cxx` | Same pipeline but emits the parity-folded accumulator directly (used to demonstrate that SHA-256 conditioning is load-bearing — this variant fails SmallCrush). |

A `bootstrap.sh` at the repo root runs all of the above in one shot.

---

## 3. Datasets

Each subdirectory under `dataset/` is self-contained:
`fetch.sh` to obtain it and `manifest.txt` to tell the C++ loader what to do.

| Directory | Tech | Primer (in manifest) | Download | Notes |
|---|---|---|---|---|
| `clustered-nanopore-reads-dataset/` | ONT, synthetic | per-cluster ref (`Centers.txt`) | 30 MB | Shipped in-tree; `nanopore_clusters` loader. |
| `dada2-16s-v4/`         | Illumina MiSeq 2×250 | EMP 515F (19 bp)        | ~35 MB  | Mothur SOP mouse-gut V4 amplicons. |
| `sra-16s-v4/`           | Illumina amplicon    | EMP 515F (19 bp)        | ~90 MB  | ENA DRR014524 by default; override with `SRA_ACC=…`. |
| `loman-zymo-r103/`      | ONT R10.3 native     | ONT 1D adapter (28 bp)  | 4.8 GB.gz / 16 GB | Mock community. |
| `loman-zymo-r10pcr/`    | ONT R10 PCR          | ONT 1D adapter (28 bp)  | 12 GB.gz / 24 GB  | PCR-amplified mock. |

### Fetch

```bash
cd dataset
./fetch_all.sh                                      # everything
SKIP=loman-zymo-r103,loman-zymo-r10pcr ./fetch_all.sh   # skip giants
./clustered-nanopore-reads-dataset/fetch.sh         # one dataset by hand
```

### Manifest format

`manifest.txt` is a simple `key: value` file. Lines starting with `#` are
comments. Recognized keys:

| Key | Required | Meaning |
|---|---|---|
| `loader`      | yes | `nanopore_clusters` \| `fastq` \| `fasta` |
| `description` | no  | Free-text label (echoed at startup). |
| `centers`     | nanopore_clusters | File with one reference strand per line. |
| `clusters`    | nanopore_clusters | File with noisy reads, blocks separated by lines beginning with `=`. |
| `reads`       | fastq, fasta | The read records file. |
| `primer`      | fastq, fasta | Known primer sequence (degenerate bases ACGT-NMRYKWSBDHV allowed). |
| `primer_len`  | no  | Bases of the primer to use for alignment (defaults to full primer length). |
| `slack`       | no  | Extra bases on the read side to absorb indels (default 5). |

Adding a new dataset is "make a new subdir + write a manifest + drop the file
in". No C++ change.

---

## 4. Running TestU01

```bash
cd build

# Default: clustered-nanopore-reads-dataset, SmallCrush
./rbg_test -p -s

# Explicit dataset + battery
./rbg_test -p -d ../dataset/dada2-16s-v4   -s    # SmallCrush
./rbg_test -p -d ../dataset/sra-16s-v4     -m    # Crush
./rbg_test -p -d ../dataset/loman-zymo-r10pcr -b # BigCrush

# Compare against the no-SHA variant
./rbg_test_nosha -p -d ../dataset/dada2-16s-v4 -s
```

Flags:

| Flag | Meaning |
|---|---|
| `-p`        | Use the primer-error entropy source (otherwise the legacy biased sampler). |
| `-d <dir>`  | Dataset directory (looks for `<dir>/manifest.txt`). |
| `-s` / `-m` / `-b` | SmallCrush (15 tests) / Crush (96) / BigCrush (106). |
| `-l`        | Linear complexity test (Berlekamp-Massey). |
| `-v`        | Verbose TestU01 output. |

### Test all fetched datasets in one shot

```bash
cd dataset
./test_all.sh                # SmallCrush across every dir that has data
BATTERY=-m ./test_all.sh     # Crush
BATTERY=-b THREADS=64 ./test_all.sh
```

Each per-dataset log lands in `dataset/<name>/last_<battery>.log`.

### Parallelism

`OMP_NUM_THREADS` controls both startup error-extraction and the TestU01
buffer fill. Past ~32 threads you hit diminishing returns because each refill
is only 32 768 independent SHA-256 blocks. Reasonable defaults:

| Stage | Sweet spot |
|---|---|
| Startup (alignment of all reads) | as many threads as cores |
| Buffer fill | 16–32 threads |

---

## 5. Verified results (SmallCrush, 32 threads)

| Dataset | Tokens extracted | Wall | Verdict |
|---|---|---|---|
| `clustered-nanopore-reads-dataset` | 1.68 M | 7 s | All 15 tests pass |
| `dada2-16s-v4`     | depends on read count | 8 s  | All 15 tests pass |
| `sra-16s-v4`       | depends on read count | 12 s | All 15 tests pass |
| `loman-zymo-r103`  | many | 16 s | All 15 tests pass |
| `loman-zymo-r10pcr`| many | 23 s | All 15 tests pass |

For reference, the same SmallCrush run on the unparallelized baseline
(`-Og`, no OpenMP, no primer-error extraction, biased sampler only) took
**4 min 23 s** — the current build is ~30× faster wall time on the same box.

`rbg_test_nosha` fails ≥10 of 15 SmallCrush statistics with `eps` p-values,
which demonstrates the SHA-256 conditioning is doing real work; the biased
raw token stream alone is not sufficient.

---

## 6. Repository layout

```
.
├── CMakeLists.txt
├── RUN.md                                # this document
├── bootstrap.sh                          # one-shot build script
├── README.md                             # original project README
├── edit_distance_sts.py                  # primer error histogram (legacy)
├── 3rd-party/
│   ├── mbedtls-3.2.1/                    # vendored
│   ├── testu01_install.sh                # original (URL 404s; use mirror)
│   ├── TestU01-src/                      # not in repo: clone of the mirror
│   └── TestU01/                          # not in repo: installed prefix
├── src/
│   ├── main.cxx                          # rbg standalone demo
│   ├── test_main.cxx                     # rbg_test (TestU01 driver)
│   ├── test_main_nosha.cxx               # rbg_test_nosha (no-SHA variant)
│   ├── sequence.hxx                      # abstract base
│   ├── sequence_sim.hxx                  # simulated biased sampler
│   ├── sequence_real_data.hxx            # original real-data sampler
│   ├── sequence_primer_error.hxx        # primer-error entropy source
│   ├── random_bit_generator.hxx         # SHA-256 conditioned RBG + fill_parallel
│   ├── random_bit_generator_nosha.hxx   # no-SHA RBG (raw parity-fold output)
│   └── dataset/
│       ├── common.hxx                    # PrimerReads, abstract DatasetLoader, factory
│       ├── nanopore_clusters.hxx         # loader for Centers.txt/Clusters.txt
│       ├── fastq.hxx                     # FASTQ loader (Illumina + ONT)
│       └── fasta.hxx                     # FASTA loader
└── dataset/
    ├── README.md
    ├── fetch_all.sh
    ├── test_all.sh
    ├── clustered-nanopore-reads-dataset/ # in-tree
    │   ├── Centers.txt, Clusters.txt
    │   ├── fetch.sh, manifest.txt
    │   └── last_s.log                    # generated by test_all.sh
    ├── dada2-16s-v4/                     # fetch.sh + manifest; data populated by fetch.sh
    ├── sra-16s-v4/                       # ditto
    ├── loman-zymo-r103/                  # ditto (4.8 GB compressed)
    └── loman-zymo-r10pcr/                # ditto (12 GB compressed)
```

---

## 7. Architecture notes

### Primer-error entropy extraction (`src/sequence_primer_error.hxx`)

- Receives a `dataset::PrimerReads` (primers + reads + alignment hyperparams).
- For every read, runs Needleman-Wunsch (`align_and_tokenize`) and emits one
  16-bit token per error event:
  - bits 0..1 = error type (0=sub, 1=ins, 2=del)
  - bits 2..7 = position within the primer (mod 64)
  - bits 8..9 = base involved (A=0, T=1, G=2, C=3); ignored for deletions
- All reads are aligned in parallel via `#pragma omp parallel for schedule(static, 256)`,
  appending into per-thread vectors that are then concatenated. After
  construction, the token vector is immutable and `operator()` samples it
  with a `thread_local std::mt19937_64` — safe for concurrent calls.

### RBG (`src/random_bit_generator.hxx`)

- `refresh_block(seq, out)` is stateless: pulls ≥256 bits of entropy from
  `seq()`, parity-folds them into a 512-bit `std::bitset`, and SHA-256s the
  64-byte accumulator into 32 bytes of `out`.
- `fill_parallel(buf, len)` shards `len / 32` blocks across OpenMP threads;
  each block is independent so there's no synchronization in the hot path.
- The TestU01 callback (`test_function`) refills a 1 MB scratch buffer once
  per 32 768 32-bit pulls.

### Dataset loaders (`src/dataset/*.hxx`)

- `DatasetLoader` is an abstract base; concrete subclasses parse one format.
- `dataset::load_dataset(dir, out)` reads `<dir>/manifest.txt`, calls
  `make_loader_for(...)`, and populates `PrimerReads`. Adding a new format
  means writing one header + one factory function.
- IUPAC degenerate bases in the primer are canonicalized to ACGT during
  loading (`degenerate_to_acgt`).

---

## 8. Troubleshooting

| Symptom | Fix |
|---|---|
| `TestU01.zip` 404 | Use the umontreal-simul GitHub mirror as in the build section. |
| `mbedtls` "No SOURCES given to target" in CMake | Run `make` in `3rd-party/mbedtls-3.2.1/` first to generate sources. |
| autoreconf complains about `aclocal-1.10` missing | Run `autoreconf -fi` in `3rd-party/TestU01-src/` to regenerate against system autotools. |
| `Could not connect` on FTP fetches | The SRA fetch script uses HTTPS to ENA, not FTP. If you forked an older copy, re-pull `sra-16s-v4/fetch.sh`. |
| Single marginal test (`p≈4e-4`) in SmallCrush | Re-run; statistical noise — 15 tests × ~8 sub-statistics × 0.2% ≈ 0.24 expected outliers per run. |
| `rbg_test_nosha` fails most tests | Expected — see §5. |
