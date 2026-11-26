#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror \
  -I"$SCRIPT_DIR/.." \
  "$SCRIPT_DIR/library_data_test.cpp" \
  "$SCRIPT_DIR/../services/LibraryData.cpp" \
  -o "$TEMP_DIR/library-data-test"
"$TEMP_DIR/library-data-test"
