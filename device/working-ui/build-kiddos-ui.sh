#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WORKSPACE="${KIDDOS_BUILD_WORKSPACE:-$(cd "$PROJECT_ROOT/../Build-Workspace" && pwd)}"

SRC="$SCRIPT_DIR/main.cpp"
BUILD="$WORKSPACE/build/arm64"
SDL="$WORKSPACE/build/deps/SDL2-2.30.3"
CONFIG="$BUILD/sdl2-config/include-config-release/SDL2"
SYS="$WORKSPACE/build/device-sysroot/usr/lib"
OUT="$BUILD/kiddos-ui"

STAMP="$(date +%Y%m%d-%H%M%S)"
TMP="$BUILD/kiddos-ui.tmp.$$"
NEXT="$BUILD/kiddos-ui.next.$$"
BACKUP="$BUILD/kiddos-ui.pre-build-$STAMP"

cleanup() {
    rm -f "$TMP" "$NEXT"
}
trap cleanup EXIT

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

echo "===== BUILD INPUTS ====="
for path in "$SRC" "$SDL/include" "$CONFIG/SDL_config.h" \
            "$SYS/libSDL2-2.0.so.0.3000.3" \
            "$SYS/libSDL2_image-2.0.so.0.800.2" \
            "$SYS/libSDL2_ttf-2.0.so.0.2200.0"; do
    [[ -e "$path" ]] || fail "Missing required input: $path"
done

command -v aarch64-linux-gnu-g++ >/dev/null ||
    fail "ARM64 compiler not found"
command -v readelf >/dev/null ||
    fail "readelf not found"

mkdir -p "$BUILD"

echo "Source: $SRC"
echo "Workspace: $WORKSPACE"
echo "Compiler: $(aarch64-linux-gnu-g++ --version | head -1)"

echo
echo "===== COMPILING ARM64 ====="
aarch64-linux-gnu-g++ \
    -std=c++17 -O2 -mcpu=cortex-a35 \
    -I"$SDL/include" \
    -I"$CONFIG" \
    -I/usr/include/SDL2 \
    "$SRC" \
    -L"$SYS" \
    -Wl,-rpath-link,"$SYS" \
    -Wl,--allow-shlib-undefined \
    -Wl,--dynamic-linker=/usr/lib/ld-linux-aarch64.so.1 \
    -Wl,-rpath,/usr/lib \
    -Wl,--no-as-needed \
    -l:libSDL2_image-2.0.so.0.800.2 \
    -l:libSDL2_ttf-2.0.so.0.2200.0 \
    -l:libSDL2-2.0.so.0.3000.3 \
    -o "$TMP"

echo
echo "===== VERIFYING OUTPUT ====="
file "$TMP"
readelf -h "$TMP" | grep -E 'Class:|Machine:'
readelf -l "$TMP" | grep 'Requesting program interpreter'
readelf -d "$TMP" | grep NEEDED

readelf -h "$TMP" | grep -q 'AArch64' ||
    fail "Output is not AArch64"

for lib in libSDL2_image-2.0.so.0 libSDL2_ttf-2.0.so.0 libSDL2-2.0.so.0; do
    readelf -d "$TMP" | grep -q "$lib" ||
        fail "Missing expected dependency: $lib"
done

echo
echo "===== INSTALLING BUILD OUTPUT ====="
if [[ -f "$OUT" ]]; then
    cp -a "$OUT" "$BACKUP"
    echo "Previous build backed up to: $BACKUP"
fi

install -m 755 "$TMP" "$NEXT"
mv -f "$NEXT" "$OUT"

echo
echo "===== BUILD SUCCESS ====="
file "$OUT"
sha256sum "$OUT"
