#!/usr/bin/env bash
# Microsoft clustered nanopore reads dataset (Centers.txt + Clusters.txt).
# Source: https://github.com/microsoft/clustered-nanopore-reads-dataset
# Shipped in-tree; this script is a no-op recovery path.
set -e
cd "$(dirname "$0")"
if [[ -s Centers.txt && -s Clusters.txt ]]; then
    echo "[clustered-nanopore-reads] already present"
    exit 0
fi
echo "[clustered-nanopore-reads] (re)fetching from GitHub"
tmp=$(mktemp -d)
git clone --depth 1 https://github.com/microsoft/clustered-nanopore-reads-dataset "$tmp/src"
cp "$tmp/src/Centers.txt" "$tmp/src/Clusters.txt" .
rm -rf "$tmp"
echo "[clustered-nanopore-reads] done"
