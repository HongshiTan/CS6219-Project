#!/usr/bin/env bash
# Loman Lab Zymo Mock Community, R10.3 pore (~4.8 GB compressed).
# Source: https://lomanlab.github.io/mockcommunity/r10.html
set -e
cd "$(dirname "$0")"
URL="https://nanopore.s3.climb.ac.uk/mock/Zymo-GridION-EVEN-3Peaks-R103-merged.fq.gz"
GZ="$(basename "$URL")"
FQ="${GZ%.gz}"
if [[ -s "$FQ" ]]; then
    echo "[loman-r103] already present"
    exit 0
fi
echo "[loman-r103] downloading $URL (~4.8 GB) ..."
curl --fail --location --continue-at - -o "$GZ" "$URL"
echo "[loman-r103] decompressing ..."
gunzip -k "$GZ"
echo "[loman-r103] done"
