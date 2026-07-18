#!/usr/bin/env bash
#
# Convenience wrapper around `make`. Builds the root_lang compiler (rootc).
#
set -euo pipefail
cd "$(dirname "$0")"

echo "Building root_lang compiler..."
make "$@"
echo
echo "Compiler ready at ./bin/rootc"
echo "Usage:  ./bin/rootc yourprogram.rtl && ./yourprogram"
