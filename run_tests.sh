#!/usr/bin/env bash
#
# root_lang example test-suite.
#
# Compiles every example program, runs it, and compares the output against the
# matching .expected file. Exits non-zero if any example fails.

set -uo pipefail
cd "$(dirname "$0")"

ROOTC=./bin/rootc
if [[ ! -x "$ROOTC" ]]; then
    echo "compiler not built; running make..."
    make >/dev/null
fi

pass=0
fail=0

run_case() {
    local src="$1"
    local expected="${src%.rtl}.expected"
    local exe="${src%.rtl}"
    local name
    name="$(basename "$src")"

    if [[ ! -f "$expected" ]]; then
        return
    fi

    if ! "$ROOTC" "$src" >/dev/null 2>compile.log; then
        echo "FAIL  $name (compile error)"
        cat compile.log
        rm -f compile.log
        fail=$((fail + 1))
        return
    fi
    rm -f compile.log

    local wd
    wd="$(dirname "$exe")"
    local base
    base="$(basename "$exe")"
    local actual
    actual="$(cd "$wd" && "./$base" 2>&1)"
    local want
    want="$(cat "$expected")"

    if [[ "$actual" == "$want" ]]; then
        echo "ok    $name"
        pass=$((pass + 1))
    else
        echo "FAIL  $name"
        echo "  expected: $want"
        echo "  actual:   $actual"
        fail=$((fail + 1))
    fi
}

# Discover every example that ships with an expected output.
while IFS= read -r -d '' f; do
    run_case "$f"
done < <(find examples -name '*.rtl' -print0 | sort -z)

echo
echo "passed: $pass   failed: $fail"
[[ "$fail" -eq 0 ]]
