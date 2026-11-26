#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

build_kiddos() {
    local SCRIPT_DIR PROJECT_ROOT WORKSPACE
    local BUILD SDL CONFIG SYS OUT
    local -a SOURCES
    local STAMP TMP NEXT BACKUP
    local path lib

    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)" || {
        echo "ERROR: Cannot locate script directory."
        return 1
    }
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)" || {
        echo "ERROR: Cannot locate project root."
        return 1
    }
    WORKSPACE="${KIDDOS_BUILD_WORKSPACE:-$PROJECT_ROOT/../Build-Workspace}"

    SOURCES=(
        "$SCRIPT_DIR/main.cpp"
        "$SCRIPT_DIR/app/Application.cpp"
        "$SCRIPT_DIR/ui/UI.cpp"
        "$SCRIPT_DIR/screens/GameScreens.cpp"
        "$SCRIPT_DIR/screens/MovieScreens.cpp"
        "$SCRIPT_DIR/screens/TVShowScreens.cpp"
        "$SCRIPT_DIR/screens/MusicScreens.cpp"
        "$SCRIPT_DIR/screens/SystemScreens.cpp"
        "$SCRIPT_DIR/screens/ParentScreen.cpp"
        "$SCRIPT_DIR/screens/WifiScreen.cpp"
        "$SCRIPT_DIR/services/GameCatalog.cpp"
        "$SCRIPT_DIR/services/LibraryData.cpp"
        "$SCRIPT_DIR/services/MovieCatalog.cpp"
        "$SCRIPT_DIR/services/TVShowCatalog.cpp"
        "$SCRIPT_DIR/services/TVShowLauncher.cpp"
        "$SCRIPT_DIR/services/MovieLauncher.cpp"
        "$SCRIPT_DIR/services/MusicCatalog.cpp"
        "$SCRIPT_DIR/services/MusicLauncher.cpp"
        "$SCRIPT_DIR/services/GameLauncher.cpp"
        "$SCRIPT_DIR/services/BatteryService.cpp"
        "$SCRIPT_DIR/services/WifiService.cpp"
        "$SCRIPT_DIR/services/PowerService.cpp"
    )
    BUILD="$WORKSPACE/build/arm64"
    SDL="$WORKSPACE/build/deps/SDL2-2.30.3"
    CONFIG="$BUILD/sdl2-config/include-config-release/SDL2"
    SYS="$WORKSPACE/build/device-sysroot/usr/lib"
    OUT="$BUILD/kiddos-ui"

    STAMP="$(date +%Y%m%d-%H%M%S)"
    TMP="$BUILD/kiddos-ui.tmp.$$"
    NEXT="$BUILD/kiddos-ui.next.$$"
    BACKUP="$BUILD/kiddos-ui.pre-build-$STAMP"

    echo "===== CHECKING BUILD INPUTS ====="
    for path in "${SOURCES[@]}" \
        "$SDL/include" "$CONFIG/SDL_config.h" \
        "$SYS/libSDL2-2.0.so.0.3000.3" \
        "$SYS/libSDL2_image-2.0.so.0.800.2" \
        "$SYS/libSDL2_ttf-2.0.so.0.2200.0"; do
        if [[ ! -e "$path" ]]; then
            echo "ERROR: Missing required input: $path"
            return 1
        fi
    done

    if ! command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
        echo "ERROR: ARM64 compiler not found."
        return 1
    fi
    if ! command -v readelf >/dev/null 2>&1; then
        echo "ERROR: readelf not found."
        return 1
    fi

    if ! mkdir -p "$BUILD"; then
        echo "ERROR: Cannot create build directory."
        return 1
    fi

    echo "Sources: ${SOURCES[*]}"
    echo "Workspace: $WORKSPACE"
    aarch64-linux-gnu-g++ --version

    echo
    echo "===== COMPILING ARM64 ====="
    if ! aarch64-linux-gnu-g++ \
        -std=c++17 -O2 -mcpu=cortex-a35 \
        -DKIDDOS_VERSION=\"0.1.0\" \
        -I"$SDL/include" \
        -I"$CONFIG" \
        -I/usr/include/SDL2 \
        "${SOURCES[@]}" \
        -L"$SYS" \
        -Wl,-rpath-link,"$SYS" \
        -Wl,--allow-shlib-undefined \
        -Wl,--dynamic-linker=/usr/lib/ld-linux-aarch64.so.1 \
        -Wl,-rpath,/usr/lib \
        -Wl,--no-as-needed \
        -l:libSDL2_image-2.0.so.0.800.2 \
        -l:libSDL2_ttf-2.0.so.0.2200.0 \
        -l:libSDL2-2.0.so.0.3000.3 \
        -o "$TMP"; then
        echo "ERROR: Compilation failed. Existing build untouched."
        rm -f "$TMP" "$NEXT"
        return 1
    fi

    echo
    echo "===== VERIFYING OUTPUT ====="
    if ! file "$TMP"; then
        echo "ERROR: Cannot inspect compiled output."
        rm -f "$TMP" "$NEXT"
        return 1
    fi
    readelf -h "$TMP" | grep -E 'Class:|Machine:'
    readelf -l "$TMP" | grep 'Requesting program interpreter'
    readelf -d "$TMP" | grep NEEDED

    if ! readelf -h "$TMP" | grep -q 'AArch64'; then
        echo "ERROR: Output is not AArch64."
        rm -f "$TMP" "$NEXT"
        return 1
    fi

    for lib in libSDL2_image-2.0.so.0 libSDL2_ttf-2.0.so.0 libSDL2-2.0.so.0; do
        if ! readelf -d "$TMP" | grep -q "$lib"; then
            echo "ERROR: Missing expected dependency: $lib"
            rm -f "$TMP" "$NEXT"
            return 1
        fi
    done

    echo
    echo "===== PREPARING BUILD OUTPUT ====="
    if [[ -f "$OUT" ]]; then
        if ! cp -a "$OUT" "$BACKUP"; then
            echo "ERROR: Could not back up previous build."
            rm -f "$TMP" "$NEXT"
            return 1
        fi
        echo "Previous build backed up to: $BACKUP"
    fi

    if ! install -m 755 "$TMP" "$NEXT"; then
        echo "ERROR: Could not prepare verified output."
        rm -f "$TMP" "$NEXT"
        return 1
    fi

    if ! mv -f "$NEXT" "$OUT"; then
        echo "ERROR: Could not install new build."
        rm -f "$TMP" "$NEXT"
        return 1
    fi

    rm -f "$TMP"

    echo
    echo "===== BUILD SUCCESS ====="
    file "$OUT"
    sha256sum "$OUT"
    return 0
}

build_kiddos
BUILD_STATUS=$?
if [[ "$BUILD_STATUS" -ne 0 ]]; then
    echo "Build finished with errors (status $BUILD_STATUS)."
fi

[[ "$BUILD_STATUS" -eq 0 ]]
