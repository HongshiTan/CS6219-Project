# TestU01 results — DNA-primer TRNG

All five datasets in this directory drive the same primer-error entropy
extractor (`src/sequence_primer_error.hxx`) feeding the SHA-256-conditioned
random-bit generator (`src/random_bit_generator.hxx`). Each test was run via
`./test_all.sh` from a fresh build (`bootstrap.sh`).

## Test host

| | |
|---|---|
| CPU | AMD EPYC 9965 (192 physical cores / 384 threads visible as 255) |
| RAM | 1.5 TiB |
| `OMP_NUM_THREADS` | 32 (set in `test_all.sh`) |
| TestU01 | 1.2.3 (built from `umontreal-simul/TestU01-2009`) |
| Compiler | gcc 13, `-O3 -fopenmp` |
| Driver | `./rbg_test -p -d <dir> {-s | -m}` |

## Per-dataset entropy source

| Dataset | Reads aligned | Tokens extracted | 16-bit-token Shannon entropy |
|---|---:|---:|---:|
| clustered-nanopore-reads-dataset | 269 709 | 1 678 130 | 5.504 |
| dada2-16s-v4                     | 304 720 | 3 770 084 | 4.399 |
| sra-16s-v4                       | 1 265 118 | 14 839 580 | 5.184 |
| loman-zymo-r103                  | 1 160 526 | 15 668 447 | 6.639 |
| loman-zymo-r10pcr                | 3 249 526 | 56 969 209 | 6.994 |

The raw 16-bit token stream is biased (max possible Shannon ≈16 bits;
realized 4.4–7.0). The von-Neumann debias + SHA-256 conditioning in
`RandonBitGenerator` produces the uniform stream that TestU01 sees.

## SmallCrush (15 statistics, ~7-23 s/dataset)

| Dataset | Wall | Verdict |
|---|---:|---|
| clustered-nanopore-reads-dataset | 7 s  | **All 15 tests passed** |
| dada2-16s-v4                     | 8 s  | **All 15 tests passed** |
| sra-16s-v4                       | 12 s | **All 15 tests passed** |
| loman-zymo-r103                  | 16 s | **All 15 tests passed** |
| loman-zymo-r10pcr                | 23 s | **All 15 tests passed** |

Logs: `dataset/<name>/last_s.log`.

For reference, the same SmallCrush on the original serial build
(`-Og`, no OpenMP, biased sampler) takes **4 min 23 s** — the current
build is roughly **30× faster wall time** on the same box.

## Crush (144 statistics, ~22-30 min wall/dataset)

| Dataset | Wall | CPU time (parallel) | Verdict |
|---|---:|---:|---|
| clustered-nanopore-reads-dataset | 21 m 39 s | 7 h 09 m 21 s | **All 144 tests passed** |
| dada2-16s-v4                     | 22 m 44 s | 7 h 39 m 27 s | **All 144 tests passed** |
| sra-16s-v4                       | 27 m 46 s | 10 h 16 m 01 s | **All 144 tests passed** |
| loman-zymo-r103                  | 28 m 26 s | 10 h 32 m 04 s | **All 144 tests passed** |
| loman-zymo-r10pcr                | 30 m 10 s | 11 h 28 m 02 s | **All 144 tests passed** |

Total Crush sweep wall time: **2 h 10 m** (sequential across datasets;
parallel within each via OpenMP). Average per-dataset CPU/wall ratio ≈ 22×.

Logs: `dataset/<name>/last_m.log`.

## Repro

```bash
./bootstrap.sh
cd dataset
./fetch_all.sh                    # SKIP=... to skip giants
./test_all.sh                     # SmallCrush
BATTERY=-m ./test_all.sh          # Crush
BATTERY=-b ./test_all.sh          # BigCrush (not yet run)
```

## Notes

- The no-SHA comparison binary `rbg_test_nosha` fails ≥10 of 15 SmallCrush
  statistics with `eps` p-values, confirming the SHA-256 conditioning is
  load-bearing rather than cosmetic.
- The clustered-nanopore-reads-dataset SmallCrush showed one marginal
  `RandomWalk1 R` p-value (4.2e-4) on one re-run and clean on the next —
  consistent with statistical noise (15 tests × ~8 sub-statistics × 0.2% ≈
  0.24 expected outliers per battery for an ideal source).
- BigCrush (106 heavier tests) would take an estimated 6-12 h wall per
  dataset on this configuration.
