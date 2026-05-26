#!/usr/bin/env bash
# A single 16S V4 amplicon SRA run, pulled via ENA's HTTPS endpoint.
# Default accession: ERR1850410 (EMP 16S V4, 515F/806R).
# Override with SRA_ACC=<accession> ./fetch.sh
set -e
cd "$(dirname "$0")"
ACC="${SRA_ACC:-DRR014524}"  # 1.3M-read Illumina 16S amplicon, ~50 MB gzipped.
if [[ -s reads.fq ]]; then
    echo "[sra-16s-v4] already present"
    exit 0
fi

prefix6=${ACC:0:6}
# Tail-padded subdir: last 3 digits of the accession, zero-padded to 3.
tail_num=$(echo "$ACC" | sed -E 's/^[A-Z]+([0-9]+)$/\1/')
suf=$(printf "%03d" "$((10#${tail_num: -3}))")

# Query ENA filereport for the canonical fastq URL — robust to schema changes.
echo "[sra-16s-v4] looking up $ACC via ENA filereport ..."
api="https://www.ebi.ac.uk/ena/portal/api/filereport?accession=${ACC}&result=read_run&fields=fastq_ftp&format=tsv"
ftp_urls=$(curl --fail --silent --location "$api" | awk -F'\t' 'NR==2{print $2}')
if [[ -z "$ftp_urls" ]]; then
    echo "[sra-16s-v4] ENA returned no fastq_ftp for $ACC; trying default path"
    ftp_urls="ftp.sra.ebi.ac.uk/vol1/fastq/${prefix6}/${suf}/${ACC}/${ACC}_1.fastq.gz"
fi

# Pick first URL, convert ftp:// or bare ftp.sra.ebi.ac.uk/... to https://.
first=$(echo "$ftp_urls" | tr ';' '\n' | head -1)
first="${first#ftp://}"
url="https://${first}"

echo "[sra-16s-v4] downloading $url"
tmp=$(mktemp -d)
curl --fail --location -o "$tmp/r.fq.gz" "$url"
gunzip -c "$tmp/r.fq.gz" > reads.fq
rm -rf "$tmp"
n=$(grep -c '^@' reads.fq || true)
echo "[sra-16s-v4] done: $n reads"
