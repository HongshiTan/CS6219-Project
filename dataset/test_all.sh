#!/usr/bin/env bash
# Run TestU01 SmallCrush against every dataset directory that has been fetched.
# Skips directories whose `reads:` file (per manifest.txt) is missing.
#
# Usage:
#   ./test_all.sh                                 # SHA build, SmallCrush
#   BATTERY=-m ./test_all.sh                      # SHA build, Crush
#   BATTERY=-b ./test_all.sh                      # SHA build, BigCrush
#   RBG_BIN=rbg_test_toeplitz RBG_TAG=toeplitz BATTERY=-b ./test_all.sh
#   THREADS=64 ./test_all.sh
#
# Per-dataset logs land in <dir>/last_<RBG_TAG>_<battery>.log so different
# hash variants don't clobber each other.
set -e
here=$(cd "$(dirname "$0")" && pwd)
build="$here/../build"
rbg_bin="${RBG_BIN:-rbg_test}"
rbg_tag="${RBG_TAG:-sha}"
bin="$build/$rbg_bin"
if [[ ! -x "$bin" ]]; then
    echo "$rbg_bin not found at $bin -- run 'make $rbg_bin' in build/ first" >&2
    exit 1
fi
battery="${BATTERY:--s}"
threads="${OMP_NUM_THREADS:-${THREADS:-32}}"
export OMP_NUM_THREADS="$threads"

# Determine which file the manifest expects to find.
present_file() {
    local dir="$1"
    local manifest="$dir/manifest.txt"
    local loader=$(awk -F': *' '/^loader:/ {print $2; exit}' "$manifest")
    case "$loader" in
        nanopore_clusters)
            local c=$(awk -F': *' '/^centers:/ {print $2; exit}' "$manifest")
            local s=$(awk -F': *' '/^clusters:/ {print $2; exit}' "$manifest")
            [[ -s "$dir/${c:-Centers.txt}" && -s "$dir/${s:-Clusters.txt}" ]]
            ;;
        fastq|fasta)
            local r=$(awk -F': *' '/^reads:/ {print $2; exit}' "$manifest")
            [[ -s "$dir/$r" ]]
            ;;
        *)
            return 1
            ;;
    esac
}

printf "binary=%s  tag=%s  battery=%s  threads=%s\n" "$rbg_bin" "$rbg_tag" "$battery" "$threads"
printf "%-40s  %-9s  %-8s  %s\n" "DATASET" "BATTERY" "THREADS" "RESULT"
printf -- "----------------------------------------------------------------------------------\n"
for d in "$here"/*/; do
    name=$(basename "$d")
    [[ -f "$d/manifest.txt" ]] || continue
    if ! present_file "$d"; then
        printf "%-40s  %-9s  %-8s  %s\n" "$name" "$battery" "$threads" "SKIP (not fetched)"
        continue
    fi
    log=$(mktemp)
    start=$(date +%s)
    if "$bin" -p -d "$d" "$battery" > "$log" 2>&1; then
        elapsed=$(( $(date +%s) - start ))
        if grep -q "All tests were passed" "$log"; then
            verdict="PASS ($(grep -c "All tests" "$log" | head -1), ${elapsed}s)"
        elif grep -q "tests gave p-values outside" "$log"; then
            failed=$(grep -E "tests? gave p-values" "$log" | head -1)
            verdict="WARN ${failed} (${elapsed}s)"
        else
            verdict="DONE (${elapsed}s)"
        fi
    else
        verdict="ERROR (see $log)"
    fi
    printf "%-40s  %-9s  %-8s  %s\n" "$name" "$battery" "$threads" "$verdict"
    cp "$log" "$d/last_${rbg_tag}_${battery#-}.log"
    rm -f "$log"
done
