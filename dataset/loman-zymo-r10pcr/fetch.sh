#!/usr/bin/env bash
# Loman Lab Zymo Mock Community, R10 PCR (~12 GB compressed).
# Source: https://lomanlab.github.io/mockcommunity/r10.html
# This is a large download; run on demand.
set -e
cd "$(dirname "$0")"
URL="https://s3.climb.ac.uk/nanopore/Zymo-GridION-EVEN-BB-SN-PCR-R10HC-flipflop.fq.gz"
GZ="$(basename "$URL")"
FQ="${GZ%.gz}"
if [[ -s "$FQ" ]]; then
    echo "[loman-r10pcr] already present"
    exit 0
fi
echo "[loman-r10pcr] downloading $URL (~12 GB) ..."
curl --fail --location --continue-at - -o "$GZ" "$URL"
echo "[loman-r10pcr] decompressing ..."
gunzip -k "$GZ"
echo "[loman-r10pcr] done"
