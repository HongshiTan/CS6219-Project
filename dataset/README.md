# DNA primer datasets

Each subdirectory is a self-contained dataset: a `fetch.sh` to obtain it and a
`manifest.txt` that tells the C++ loader how to parse it and which sequence is
the known primer.

**TestU01 results for every dataset are in [`RESULTS.md`](RESULTS.md).**

| Dir | Tech | Size | Primer | Notes |
|-----|------|------|--------|-------|
| `clustered-nanopore-reads-dataset/` | ONT (synthetic) | 30 MB | per-cluster ref (`Centers.txt`) | Shipped in-tree. Multi-cluster format. |
| `dada2-16s-v4/`         | Illumina MiSeq 2×250 | ~25 MB | EMP 515F (19 bp)             | Small, fast iteration. |
| `sra-16s-v4/`           | Illumina (EMP)       | ~hundreds of MB | EMP 515F (19 bp)        | Single ENA run; configurable via `SRA_ACC`. |
| `loman-zymo-r103/`      | ONT R10.3 native     | 4.8 GB compressed | ONT 1D adapter (28 bp)   | Mock community. |
| `loman-zymo-r10pcr/`    | ONT R10 PCR          | 12 GB compressed  | ONT 1D adapter (28 bp)   | PCR-amplified mock. |

## Fetch

```
./fetch_all.sh
# or skip giants:
SKIP=loman-zymo-r103,loman-zymo-r10pcr ./fetch_all.sh
```

## Run TestU01 against a dataset

From `build/`:

```
./rbg_test -p -d ../dataset/<name> -s    # SmallCrush
./rbg_test -p -d ../dataset/<name> -m    # Crush
./rbg_test -p -d ../dataset/<name> -b    # BigCrush
```

## Run all fetched datasets in one shot

```
./test_all.sh                            # SmallCrush across every fetched dir
```

## Manifest format

```
loader: nanopore_clusters | fastq | fasta
description: human-readable text
# loader-specific fields below

# nanopore_clusters:
centers: <file>     # one reference strand per line
clusters: <file>    # blocks of noisy reads separated by lines starting with '='

# fastq / fasta:
reads: <file>       # the read records
primer: <ACGT>      # the known primer (degenerate bases allowed: ACGTNMRYKWSBDHV)

# common:
primer_len: <int>   # how much of the read prefix to align (defaults to primer length)
slack: <int>        # extra bases on the read side to absorb indels
```
