#!/usr/bin/env bash
# One-shot build: TestU01 from GitHub mirror, mbedtls sources, then the project.
# Re-runs are safe; already-built pieces are skipped.
set -e
here=$(cd "$(dirname "$0")" && pwd)

# 1. TestU01
if [[ ! -s "$here/3rd-party/TestU01/lib/libtestu01.so" ]]; then
    echo "=== building TestU01 ==="
    cd "$here/3rd-party"
    if [[ ! -d TestU01-src ]]; then
        git clone --depth 1 https://github.com/umontreal-simul/TestU01-2009.git TestU01-src
    fi
    cd TestU01-src
    autoreconf -fi
    ./configure --prefix="$here/3rd-party/TestU01"
    make -j install
fi

# 2. mbedtls source generation
if [[ ! -s "$here/3rd-party/mbedtls-3.2.1/library/aes.o" ]]; then
    echo "=== building mbedtls (for source generation) ==="
    cd "$here/3rd-party/mbedtls-3.2.1"
    make -j
fi

# 3. Project
echo "=== configuring + building project ==="
mkdir -p "$here/build"
cd "$here/build"
cmake ..
make -j rbg rbg_test rbg_test_nosha

echo
echo "Built: $here/build/{rbg, rbg_test, rbg_test_nosha}"
echo "Next: cd dataset && ./fetch_all.sh   # then ./test_all.sh"
