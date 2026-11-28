#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

CXX="${CXX:-g++}"
"$CXX" -std=c++17 -Wall -Wextra -Werror -I"$SCRIPT_DIR/.." \
    "$SCRIPT_DIR/music_workflow_test.cpp" \
    "$SCRIPT_DIR/../services/MusicWorkflow.cpp" \
    -o "$TEMP_DIR/music-workflow-test"
"$TEMP_DIR/music-workflow-test"
