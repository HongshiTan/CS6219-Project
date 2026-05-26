#!/usr/bin/env bash
# Fetch every dataset in subdirs. Skips ones already present.
# Use SKIP=name1,name2 to opt out of huge ones, e.g.
#   SKIP=loman-zymo-r103,loman-zymo-r10pcr ./fetch_all.sh
set -e
here=$(cd "$(dirname "$0")" && pwd)
IFS=',' read -r -a skip_arr <<< "${SKIP:-}"
for d in "$here"/*/; do
    name=$(basename "$d")
    skip=0
    for s in "${skip_arr[@]}"; do
        [[ "$s" == "$name" ]] && skip=1
    done
    if (( skip )); then
        echo "=== skipping $name ==="
        continue
    fi
    if [[ -x "$d/fetch.sh" ]]; then
        echo "=== fetching $name ==="
        "$d/fetch.sh"
    fi
done
