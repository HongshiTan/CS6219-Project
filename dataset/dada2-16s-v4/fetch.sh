#!/usr/bin/env bash
# DADA2 tutorial 16S V4 amplicon dataset (Illumina MiSeq 2x250, mouse gut).
# Source: https://benjjneb.github.io/dada2/tutorial_1_8
# Hosted as a tarball on the DADA2 author's site.
set -e
cd "$(dirname "$0")"
URL="https://mothur.s3.us-east-2.amazonaws.com/wiki/miseqsopdata.zip"
if [[ -s forward_reads.fq ]]; then
    echo "[dada2-16s-v4] already present"
    exit 0
fi
echo "[dada2-16s-v4] downloading mothur MiSeq SOP (~30 MB) ..."
tmp=$(mktemp -d)
curl --fail --location -o "$tmp/data.zip" "$URL"
unzip -q -o "$tmp/data.zip" -d "$tmp"
# The archive contains paired-end fastq files like F3D0_S188_L001_R1_001.fastq.
# Concatenate all forward reads into one file.
: > forward_reads.fq
for f in "$tmp"/*/*_R1_001.fastq "$tmp"/MiSeq_SOP/*_R1_001.fastq; do
    [[ -f "$f" ]] || continue
    cat "$f" >> forward_reads.fq
done
rm -rf "$tmp"
n=$(grep -c '^@M' forward_reads.fq || true)
echo "[dada2-16s-v4] done: ~$n reads in forward_reads.fq"
