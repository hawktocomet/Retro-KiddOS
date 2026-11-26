#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

deploy_kiddos() {
    local SCRIPT_DIR PROJECT_ROOT LOCAL
    local REMOTE TARGET STAGED REMOTE_DIR
    local CONFIG_LOCAL CONFIG_TARGET CONFIG_STAGED CONFIG_BACKUP
    local HELPER_LOCAL HELPER_TARGET HELPER_STAGED HELPER_BACKUP
    local GB_LOCAL GBC_LOCAL GB_STAGED GBC_STAGED JOY_LOCAL JOY_STAGED
    local GB_EXPECTED GBC_EXPECTED JOY_EXPECTED HELPER_EXPECTED REMOTE_FILE EXPECTED_HASH ACTUAL_HASH PROFILE
    local EXPECTED ACTUAL BACKUP WORK CHUNK CONFIG_EXPECTED DEPLOY_ID
    local STATUS

    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)" || {
        echo "ERROR: Cannot locate script directory."
        return 1
    }
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)" || {
        echo "ERROR: Cannot locate project root."
        return 1
    }

    REMOTE="${KIDDOS_DEVICE:-root@192.168.0.89}"
    TARGET="/storage/roms/.kiddos/kiddos-ui"
    STAGED="${TARGET}.new"
    HELPER_LOCAL="$SCRIPT_DIR/wifi_helper.py"
    HELPER_TARGET="/storage/roms/.kiddos/wifi_helper.py"
    HELPER_STAGED="/tmp/kiddos-deploy-stage/wifi_helper.py.new"
    REMOTE_DIR="/tmp/kiddos-deploy-stage"
    LOCAL="$PROJECT_ROOT/../Build-Workspace/build/arm64/kiddos-ui"
    CONFIG_LOCAL="$SCRIPT_DIR/mpv-input.conf"
    CONFIG_TARGET="/storage/.config/kiddos/mpv-input.conf"
    CONFIG_STAGED="/tmp/kiddos-deploy-stage/mpv-input.conf.new"
    DEPLOY_ID="$(date +%Y%m%d-%H%M%S)"
    BACKUP="${TARGET}.pre-deploy-${DEPLOY_ID}"
    HELPER_BACKUP="${HELPER_TARGET}.pre-deploy-${DEPLOY_ID}"
    CONFIG_BACKUP="${CONFIG_TARGET}.pre-deploy-${DEPLOY_ID}"
    GB_LOCAL="$SCRIPT_DIR/retroarch-profiles/gb.cfg"
    GBC_LOCAL="$SCRIPT_DIR/retroarch-profiles/gbc.cfg"
    JOY_LOCAL="$SCRIPT_DIR/retroarch-profiles/GO-Super Gamepad.cfg"
    GB_STAGED="$REMOTE_DIR/gb.cfg.new"
    GBC_STAGED="$REMOTE_DIR/gbc.cfg.new"
    JOY_STAGED="$REMOTE_DIR/go-super-gamepad.cfg.new"

    if [[ ! -f "$LOCAL" ]]; then
        echo "ERROR: Build not found: $LOCAL"
        return 1
    fi
    if [[ ! -f "$CONFIG_LOCAL" ]]; then
        echo "ERROR: MPV input config not found: $CONFIG_LOCAL"
        return 1
    fi
    if [[ ! -f "$HELPER_LOCAL" ]]; then
        echo "ERROR: Wi-Fi helper not found: $HELPER_LOCAL"
        return 1
    fi
    if [[ ! -f "$GB_LOCAL" || ! -f "$GBC_LOCAL" || ! -f "$JOY_LOCAL" ]]; then
        echo "ERROR: A required RetroArch profile source is missing."
        return 1
    fi
    for command_name in ssh scp split sha256sum file mktemp; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "ERROR: Required command not found: $command_name"
            return 1
        fi
    done

    EXPECTED="$(sha256sum "$LOCAL" | cut -d ' ' -f 1)"
    CONFIG_EXPECTED="$(sha256sum "$CONFIG_LOCAL" | cut -d ' ' -f 1)"
    HELPER_EXPECTED="$(sha256sum "$HELPER_LOCAL" | cut -d ' ' -f 1)"
    GB_EXPECTED="$(sha256sum "$GB_LOCAL" | cut -d ' ' -f 1)"
    GBC_EXPECTED="$(sha256sum "$GBC_LOCAL" | cut -d ' ' -f 1)"
    JOY_EXPECTED="$(sha256sum "$JOY_LOCAL" | cut -d ' ' -f 1)"
    WORK="$(mktemp -d)" || {
        echo "ERROR: Could not create temporary transfer directory."
        return 1
    }

    echo "===== LOCAL BUILD ====="
    file "$LOCAL"
    echo "SHA-256: $EXPECTED"
    echo "Device: $REMOTE"

    if ! ssh "$REMOTE" "mkdir -p '/storage/roms/.kiddos' '$REMOTE_DIR/parts' /storage/.config/kiddos /storage/joypads && rm -f '$REMOTE_DIR'/parts/chunk-* '$STAGED' '$HELPER_STAGED' '$CONFIG_STAGED' '$GB_STAGED' '$GBC_STAGED' '$JOY_STAGED'"; then
        echo "ERROR: Could not prepare device staging directory."
        rm -rf "$WORK"
        return 1
    fi

    echo
    echo "===== SPLITTING INTO 24 KB CHUNKS ====="
    if ! split -b 24000 -d -a 3 "$LOCAL" "$WORK/chunk-"; then
        echo "ERROR: Could not split executable."
        rm -rf "$WORK"
        return 1
    fi

    for CHUNK in "$WORK"/chunk-*; do
        echo "Sending $(basename "$CHUNK")..."
        if ! scp -q "$CHUNK" "$REMOTE:$REMOTE_DIR/parts/"; then
            echo "ERROR: Transfer failed: $CHUNK"
            rm -rf "$WORK"
            return 1
        fi
    done

    if ! scp -q "$HELPER_LOCAL" "$REMOTE:$HELPER_STAGED"; then
        echo "ERROR: Wi-Fi helper transfer failed."
        rm -rf "$WORK"
        return 1
    fi

    if ! scp -q "$CONFIG_LOCAL" "$REMOTE:$CONFIG_STAGED"; then
        echo "ERROR: MPV config transfer failed."
        rm -rf "$WORK"
        return 1
    fi
    if ! scp -q "$GB_LOCAL" "$REMOTE:$GB_STAGED"; then
        echo "ERROR: GB profile transfer failed."
        rm -rf "$WORK"
        return 1
    fi
    if ! scp -q "$GBC_LOCAL" "$REMOTE:$GBC_STAGED"; then
        echo "ERROR: GBC profile transfer failed."
        rm -rf "$WORK"
        return 1
    fi

    if ! scp -q "$JOY_LOCAL" "$REMOTE:$JOY_STAGED"; then
        echo "ERROR: Controller profile transfer failed."
        rm -rf "$WORK"
        return 1
    fi

    echo
    echo "===== ASSEMBLING AND VERIFYING STAGED FILE ====="
    ACTUAL="$(ssh "$REMOTE" "cat '$REMOTE_DIR'/parts/chunk-* > '$STAGED' && chmod 755 '$STAGED' && sha256sum '$STAGED' | cut -d ' ' -f 1")"
    STATUS=$?

    if [[ "$STATUS" -ne 0 || "$ACTUAL" != "$EXPECTED" ]]; then
        echo "ERROR: Staged checksum verification failed."
        echo "Expected: $EXPECTED"
        echo "Received: ${ACTUAL:-<none>}"
        rm -rf "$WORK"
        return 1
    fi
    echo "Staged checksum verified: $ACTUAL"

    HELPER_ACTUAL="$(ssh "$REMOTE" "sha256sum '$HELPER_STAGED' | cut -d ' ' -f 1")"
    STATUS=$?
    if [[ "$STATUS" -ne 0 || "$HELPER_ACTUAL" != "$HELPER_EXPECTED" ]]; then
        echo "ERROR: Wi-Fi helper checksum verification failed."
        echo "Expected: $HELPER_EXPECTED"
        echo "Received: ${HELPER_ACTUAL:-<none>}"
        rm -rf "$WORK"
        return 1
    fi
    echo "Wi-Fi helper checksum verified: $HELPER_ACTUAL"

    echo
    echo "===== VERIFYING STAGED PROFILES ====="
    for PROFILE in "$GB_STAGED:$GB_EXPECTED" "$GBC_STAGED:$GBC_EXPECTED" "$JOY_STAGED:$JOY_EXPECTED"; do
        REMOTE_FILE="${PROFILE%%:*}"
        EXPECTED_HASH="${PROFILE##*:}"
        ACTUAL_HASH="$(ssh "$REMOTE" "sha256sum '$REMOTE_FILE' | cut -d ' ' -f 1")"
        STATUS=$?
        if [[ "$STATUS" -ne 0 || "$ACTUAL_HASH" != "$EXPECTED_HASH" ]]; then
            echo "ERROR: Profile checksum verification failed: $REMOTE_FILE"
            echo "Expected: $EXPECTED_HASH"
            echo "Received: ${ACTUAL_HASH:-<none>}"
            rm -rf "$WORK"
            return 1
        fi
        echo "Verified: $REMOTE_FILE"
    done

    echo
    echo "===== INSTALLING AND LAUNCHING ====="
    ssh "$REMOTE" sh -s -- "$TARGET" "$STAGED" "$BACKUP" "$EXPECTED" "$HELPER_TARGET" "$HELPER_STAGED" "$HELPER_BACKUP" "$HELPER_EXPECTED" "$CONFIG_TARGET" "$CONFIG_STAGED" "$CONFIG_BACKUP" "$CONFIG_EXPECTED" "/storage/.config/kiddos/retroarch" "$GB_STAGED" "$GBC_STAGED" "$GB_EXPECTED" "$GBC_EXPECTED" "$JOY_STAGED" "$JOY_EXPECTED" <<'REMOTE_SCRIPT'
TARGET="$1"
STAGED="$2"
BACKUP="$3"
EXPECTED="$4"
HELPER_TARGET="$5"
HELPER_STAGED="$6"
HELPER_BACKUP="$7"
HELPER_EXPECTED="$8"
CONFIG_TARGET="$9"
CONFIG_STAGED="${10}"
CONFIG_BACKUP="${11}"
CONFIG_EXPECTED="${12}"
PROFILE_DIR="${13}"
GB_STAGED="${14}"
GBC_STAGED="${15}"
GB_EXPECTED="${16}"
GBC_EXPECTED="${17}"
JOY_STAGED="${18}"
JOY_EXPECTED="${19}"
GB_TARGET="$PROFILE_DIR/gb.cfg"
GBC_TARGET="$PROFILE_DIR/gbc.cfg"
JOY_TARGET="/storage/joypads/GO-Super Gamepad.cfg"
PROFILE_BACKUP_ID="$(date +%Y%m%d-%H%M%S)"
GB_BACKUP="$GB_TARGET.pre-deploy-$PROFILE_BACKUP_ID"
GBC_BACKUP="$GBC_TARGET.pre-deploy-$PROFILE_BACKUP_ID"
JOY_BACKUP="$JOY_TARGET.pre-deploy-$PROFILE_BACKUP_ID"
CONFIG_HAD_BACKUP=0
EXECUTABLE_HAD_BACKUP=0
HELPER_HAD_BACKUP=0
GB_HAD_BACKUP=0
GBC_HAD_BACKUP=0
JOY_HAD_BACKUP=0
UNIT="kiddos-ui"

launch_kiddos() {
    systemd-run --quiet --no-block --service-type=exec \
        --unit="$UNIT" \
        --property=Restart=on-failure \
        --property=RestartSec=2 \
        --setenv=HOME=/storage "$TARGET"
}

restore_previous() {
    echo "Attempting automatic rollback..."
    if ! systemctl stop "$UNIT.service"; then
        echo "CRITICAL: Could not stop KiddOS for automatic rollback."
        return 1
    fi
    if systemctl is-active --quiet "$UNIT.service"; then
        echo "CRITICAL: KiddOS is still active; refusing to overwrite executable."
        return 1
    fi
    if [ "$EXECUTABLE_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$BACKUP" "$TARGET"; then
            echo "CRITICAL: Could not restore executable backup."
            return 1
        fi
    else
        if ! rm -f "$TARGET"; then
            echo "CRITICAL: Could not remove first-install executable."
            return 1
        fi
    fi
    if [ "$HELPER_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$HELPER_BACKUP" "$HELPER_TARGET"; then
            echo "CRITICAL: Could not restore Wi-Fi helper backup."
            return 1
        fi
    else
        if ! rm -f "$HELPER_TARGET"; then
            echo "CRITICAL: Could not remove first-install Wi-Fi helper."
            return 1
        fi
    fi
    if [ "$CONFIG_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$CONFIG_BACKUP" "$CONFIG_TARGET"; then
            echo "CRITICAL: Could not restore MPV config backup."
            return 1
        fi
    else
        if ! rm -f "$CONFIG_TARGET"; then
            echo "CRITICAL: Could not remove MPV config; previous state had no config."
            return 1
        fi
    fi
    if [ "$GB_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$GB_BACKUP" "$GB_TARGET"; then
            echo "CRITICAL: Could not restore GB profile."
            return 1
        fi
    else
        if ! rm -f "$GB_TARGET"; then
            echo "CRITICAL: Could not remove newly installed GB profile."
            return 1
        fi
    fi
    if [ "$GBC_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$GBC_BACKUP" "$GBC_TARGET"; then
            echo "CRITICAL: Could not restore GBC profile."
            return 1
        fi
    else
        if ! rm -f "$GBC_TARGET"; then
            echo "CRITICAL: Could not remove newly installed GBC profile."
            return 1
        fi
    fi
    if [ "$JOY_HAD_BACKUP" -eq 1 ]; then
        if ! cp -p "$JOY_BACKUP" "$JOY_TARGET"; then
            echo "CRITICAL: Could not restore controller profile."
            return 1
        fi
    else
        if ! rm -f "$JOY_TARGET"; then
            echo "CRITICAL: Could not remove newly installed controller profile."
            return 1
        fi
    fi
    if [ "$EXECUTABLE_HAD_BACKUP" -eq 0 ]; then
        echo "FIRST-INSTALL ROLLBACK: New executable and helper removed."
        echo "No previous KiddOS installation existed to relaunch."
        return 0
    fi
    echo "Previous executable, Wi-Fi helper, MPV config, GB/GBC profiles, and controller profile states restored."
    if ! launch_kiddos; then
        echo "CRITICAL: Could not relaunch previous executable."
        return 1
    fi
    sleep 4
    if systemctl is-active --quiet "$UNIT.service"; then
        echo "ROLLBACK SUCCESS: Previous KiddOS is running."
        sha256sum "$TARGET"
        return 0
    fi
    echo "CRITICAL: Previous executable restored, but service is not active."
    return 1
}

deploy_main() {
    if [ ! -f "$HELPER_STAGED" ] || [ "$(sha256sum "$HELPER_STAGED" | cut -d " " -f 1)" != "$HELPER_EXPECTED" ]; then
        echo "ERROR: Staged Wi-Fi helper checksum verification failed."
        return 1
    fi
    if [ ! -x "$STAGED" ]; then
        echo "ERROR: Verified staged executable missing."
        return 1
    fi
    if [ -e "$BACKUP" ] || [ -e "$HELPER_BACKUP" ] || [ -e "$CONFIG_BACKUP" ] ||
       [ -e "$GB_BACKUP" ] || [ -e "$GBC_BACKUP" ] || [ -e "$JOY_BACKUP" ]; then
        echo "ERROR: A backup path already exists."
        return 1
    fi
    if [ ! -f "$CONFIG_STAGED" ] || [ "$(sha256sum "$CONFIG_STAGED" | cut -d " " -f 1)" != "$CONFIG_EXPECTED" ]; then
        echo "ERROR: Staged MPV config checksum verification failed."
        return 1
    fi
    if [ ! -f "$GB_STAGED" ] || [ "$(sha256sum "$GB_STAGED" | cut -d " " -f 1)" != "$GB_EXPECTED" ]; then
        echo "ERROR: Staged GB profile checksum verification failed."
        return 1
    fi
    if [ ! -f "$GBC_STAGED" ] || [ "$(sha256sum "$GBC_STAGED" | cut -d " " -f 1)" != "$GBC_EXPECTED" ]; then
        echo "ERROR: Staged GBC profile checksum verification failed."
        return 1
    fi
    if [ ! -f "$JOY_STAGED" ] || [ "$(sha256sum "$JOY_STAGED" | cut -d " " -f 1)" != "$JOY_EXPECTED" ]; then
        echo "ERROR: Staged controller profile checksum verification failed."
        return 1
    fi
    ACTUAL="$(sha256sum "$STAGED" | cut -d " " -f 1)"
    if [ "$ACTUAL" != "$EXPECTED" ]; then
        echo "ERROR: Staged checksum changed before installation."
        return 1
    fi

    if [ -e "$TARGET" ]; then
        if ! cp -p "$TARGET" "$BACKUP"; then
            echo "ERROR: Could not create executable rollback backup."
            return 1
        fi
        EXECUTABLE_HAD_BACKUP=1
        echo "Executable backup: $BACKUP"
    fi
    if [ -e "$HELPER_TARGET" ]; then
        if ! cp -p "$HELPER_TARGET" "$HELPER_BACKUP"; then
            echo "ERROR: Could not create Wi-Fi helper backup."
            return 1
        fi
        HELPER_HAD_BACKUP=1
        echo "Wi-Fi helper backup: $HELPER_BACKUP"
    fi
    if [ -e "$CONFIG_TARGET" ]; then
        if ! cp -p "$CONFIG_TARGET" "$CONFIG_BACKUP"; then
            echo "ERROR: Could not back up existing MPV config."
            return 1
        fi
        CONFIG_HAD_BACKUP=1
        echo "MPV config backup: $CONFIG_BACKUP"
    fi

    if [ -e "$GB_TARGET" ]; then
        if ! cp -p "$GB_TARGET" "$GB_BACKUP"; then
            echo "ERROR: Could not back up existing GB profile."
            return 1
        fi
        GB_HAD_BACKUP=1
        echo "GB profile backup: $GB_BACKUP"
    fi
    if [ -e "$GBC_TARGET" ]; then
        if ! cp -p "$GBC_TARGET" "$GBC_BACKUP"; then
            echo "ERROR: Could not back up existing GBC profile."
            return 1
        fi
        GBC_HAD_BACKUP=1
        echo "GBC profile backup: $GBC_BACKUP"
    fi
    if [ -e "$JOY_TARGET" ]; then
        if ! cp -p "$JOY_TARGET" "$JOY_BACKUP"; then
            echo "ERROR: Could not back up existing controller profile."
            return 1
        fi
        JOY_HAD_BACKUP=1
        echo "Controller profile backup: $JOY_BACKUP"
    fi

    if ! systemctl stop "$UNIT.service"; then
        echo "ERROR: Could not stop existing KiddOS service."
        return 1
    fi
    if systemctl is-active --quiet "$UNIT.service"; then
        echo "ERROR: Existing KiddOS service is still active."
        return 1
    fi

    if ! mv "$STAGED" "$TARGET"; then
        echo "ERROR: Could not install staged executable."
        restore_previous
        return 1
    fi
    if ! chmod 755 "$TARGET"; then
        echo "ERROR: Could not set executable permissions."
        restore_previous
        return 1
    fi
    if ! mv "$HELPER_STAGED" "$HELPER_TARGET"; then
        echo "ERROR: Could not install Wi-Fi helper."
        restore_previous
        return 1
    fi
    if ! chmod 755 "$HELPER_TARGET"; then
        echo "ERROR: Could not set Wi-Fi helper permissions."
        restore_previous
        return 1
    fi
    if ! mv "$CONFIG_STAGED" "$CONFIG_TARGET"; then
        echo "ERROR: Could not install MPV input config."
        restore_previous
        return 1
    fi
    chmod 600 "$CONFIG_TARGET" || {
        echo "ERROR: Could not set MPV config permissions."
        restore_previous
        return 1
    }

    if ! mkdir -p "$PROFILE_DIR"; then
        echo "ERROR: Could not create RetroArch profile directory."
        restore_previous
        return 1
    fi
    if ! mv "$GB_STAGED" "$GB_TARGET"; then
        echo "ERROR: Could not install GB profile."
        restore_previous
        return 1
    fi
    if ! mv "$GBC_STAGED" "$GBC_TARGET"; then
        echo "ERROR: Could not install GBC profile."
        restore_previous
        return 1
    fi
    if ! chmod 600 "$GB_TARGET" "$GBC_TARGET"; then
        echo "ERROR: Could not set RetroArch profile permissions."
        restore_previous
        return 1
    fi

    if ! mv "$JOY_STAGED" "$JOY_TARGET"; then
        echo "ERROR: Could not install controller profile."
        restore_previous
        return 1
    fi
    if ! chmod 600 "$JOY_TARGET"; then
        echo "ERROR: Could not set controller profile permissions."
        restore_previous
        return 1
    fi

    if launch_kiddos; then
        sleep 4
        if systemctl is-active --quiet "$UNIT.service"; then
            echo "SUCCESS: New KiddOS is running."
            sha256sum "$TARGET"
            if [ "$EXECUTABLE_HAD_BACKUP" -eq 1 ]; then
                echo "Executable backup: $BACKUP"
            else
                echo "Executable backup: - (first install)"
            fi
            if [ "$HELPER_HAD_BACKUP" -eq 1 ]; then
                echo "Wi-Fi helper backup: $HELPER_BACKUP"
            else
                echo "Wi-Fi helper backup: -"
            fi
            if [ "$CONFIG_HAD_BACKUP" -eq 1 ]; then
                echo "MPV config backup: $CONFIG_BACKUP"
            else
                echo "MPV config backup: -"
            fi
            if [ "$GB_HAD_BACKUP" -eq 1 ]; then
                echo "GB profile backup: $GB_BACKUP"
            else
                echo "GB profile backup: -"
            fi
            if [ "$GBC_HAD_BACKUP" -eq 1 ]; then
                echo "GBC profile backup: $GBC_BACKUP"
            else
                echo "GBC profile backup: -"
            fi
            if [ "$JOY_HAD_BACKUP" -eq 1 ]; then
                echo "Controller profile backup: $JOY_BACKUP"
            else
                echo "Controller profile backup: -"
            fi
            return 0
        fi
    fi

    echo "ERROR: New KiddOS failed to remain active."
    restore_previous
    return 1
}

deploy_main
STATUS=$?
[ "$STATUS" -eq 0 ]
REMOTE_SCRIPT
    STATUS=$?

    rm -rf "$WORK"

    if [[ "$STATUS" -eq 0 ]]; then
        echo
        echo "DEPLOYMENT SUCCESS."
        if [ -e "$BACKUP" ]; then
            echo "Rollback backup: $BACKUP"
        else
            echo "Rollback backup: - (first install)"
        fi
        if [ -e "$HELPER_BACKUP" ]; then
            echo "Wi-Fi helper backup: $HELPER_BACKUP"
        else
            echo "Wi-Fi helper backup: -"
        fi
        return 0
    fi

    echo
    echo "DEPLOYMENT FAILED. Check the device output above."
    return 1
}

rollback_kiddos() {
    local BACKUP="${1:-}"
    local CONFIG_BACKUP="${2:-}"
    local GB_PROFILE_BACKUP="${3:-}"
    local GBC_PROFILE_BACKUP="${4:-}"
    local JOY_PROFILE_BACKUP="${5:-}"
    local HELPER_BACKUP="${6:-__UNCHANGED__}"
    local REMOTE TARGET HELPER_TARGET CONFIG_TARGET PROFILE_DIR RESTORE_PROFILES=0

    REMOTE="${KIDDOS_DEVICE:-root@192.168.0.89}"
    TARGET="/storage/roms/.kiddos/kiddos-ui"
    HELPER_TARGET="/storage/roms/.kiddos/wifi_helper.py"
    CONFIG_TARGET="/storage/.config/kiddos/mpv-input.conf"
    PROFILE_DIR="/storage/.config/kiddos/retroarch"

    if [[ -z "$BACKUP" || -z "$CONFIG_BACKUP" ]]; then
        echo "Usage: $0 rollback EXECUTABLE_BACKUP MPV_CONFIG_BACKUP|- [GB_PROFILE_BACKUP|- GBC_PROFILE_BACKUP|- CONTROLLER_PROFILE_BACKUP|- [WIFI_HELPER_BACKUP|-]]"
        return 2
    fi

    if [[ -n "$GB_PROFILE_BACKUP" || -n "$GBC_PROFILE_BACKUP" || -n "$JOY_PROFILE_BACKUP" ]]; then
        if [[ -z "$GB_PROFILE_BACKUP" || -z "$GBC_PROFILE_BACKUP" || -z "$JOY_PROFILE_BACKUP" ]]; then
            echo "ERROR: Supply GB, GBC, and controller profile backup arguments together, or omit all three."
            return 2
        fi
        RESTORE_PROFILES=1
    fi

    if [[ "$HELPER_BACKUP" == "-" ]]; then
        echo "Wi-Fi helper: restore previous absent state"
    elif [[ "$HELPER_BACKUP" != "__UNCHANGED__" ]]; then
        echo "Wi-Fi helper backup: $HELPER_BACKUP"
    else
        echo "Wi-Fi helper: unchanged (legacy rollback command)"
    fi

    echo "===== RESTORING EXECUTABLE AND CONFIGURATION ====="
    if [[ "$RESTORE_PROFILES" -eq 1 ]]; then
        echo "GB profile backup: $GB_PROFILE_BACKUP"
        echo "GBC profile backup: $GBC_PROFILE_BACKUP"
        echo "Controller profile backup: $JOY_PROFILE_BACKUP"
    else
        echo "Legacy rollback mode: leaving RetroArch profiles unchanged."
    fi

    ssh "$REMOTE" sh -s -- \
        "$TARGET" "$BACKUP" "$CONFIG_TARGET" "$CONFIG_BACKUP" \
        "$PROFILE_DIR" "$GB_PROFILE_BACKUP" "$GBC_PROFILE_BACKUP" "$JOY_PROFILE_BACKUP" "$RESTORE_PROFILES" "$HELPER_BACKUP" <<'REMOTE_SCRIPT'
TARGET="$1"
BACKUP="$2"
CONFIG_TARGET="$3"
CONFIG_BACKUP="$4"
PROFILE_DIR="$5"
GB_PROFILE_BACKUP="$6"
GBC_PROFILE_BACKUP="$7"
JOY_PROFILE_BACKUP="$8"
RESTORE_PROFILES="$9"
HELPER_BACKUP="${10}"
HELPER_TARGET="/storage/roms/.kiddos/wifi_helper.py"

GB_TARGET="$PROFILE_DIR/gb.cfg"
GBC_TARGET="$PROFILE_DIR/gbc.cfg"
JOY_TARGET="/storage/joypads/GO-Super Gamepad.cfg"

if [ ! -f "$BACKUP" ]; then
    echo "ERROR: Executable backup not found: $BACKUP"
    exit 1
fi
if [ "$CONFIG_BACKUP" != "-" ] && [ ! -f "$CONFIG_BACKUP" ]; then
    echo "ERROR: MPV config backup not found: $CONFIG_BACKUP"
    exit 1
fi
if [ "$HELPER_BACKUP" != "__UNCHANGED__" ] &&
   [ "$HELPER_BACKUP" != "-" ] && [ ! -f "$HELPER_BACKUP" ]; then
    echo "ERROR: Wi-Fi helper backup not found: $HELPER_BACKUP"
    exit 1
fi
if [ "$RESTORE_PROFILES" -eq 1 ]; then
    if [ "$GB_PROFILE_BACKUP" != "-" ] && [ ! -f "$GB_PROFILE_BACKUP" ]; then
        echo "ERROR: GB profile backup not found: $GB_PROFILE_BACKUP"
        exit 1
    fi
    if [ "$GBC_PROFILE_BACKUP" != "-" ] && [ ! -f "$GBC_PROFILE_BACKUP" ]; then
        echo "ERROR: GBC profile backup not found: $GBC_PROFILE_BACKUP"
        exit 1
    fi
    if [ "$JOY_PROFILE_BACKUP" != "-" ] && [ ! -f "$JOY_PROFILE_BACKUP" ]; then
        echo "ERROR: Controller profile backup not found: $JOY_PROFILE_BACKUP"
        exit 1
    fi
fi

if ! systemctl stop kiddos-ui.service; then
    echo "ERROR: Could not stop KiddOS service."
    exit 1
fi
if systemctl is-active --quiet kiddos-ui.service; then
    echo "ERROR: KiddOS is still active; refusing to overwrite files."
    exit 1
fi

if ! cp -p "$BACKUP" "$TARGET"; then
    echo "ERROR: Could not restore executable backup."
    exit 1
fi
if ! chmod 755 "$TARGET"; then
    echo "ERROR: Could not set executable permissions on restored file."
    exit 1
fi

if [ "$HELPER_BACKUP" = "-" ]; then
    if ! rm -f "$HELPER_TARGET"; then
        echo "ERROR: Could not restore absent Wi-Fi helper state."
        exit 1
    fi
elif [ "$HELPER_BACKUP" != "__UNCHANGED__" ]; then
    if ! cp -p "$HELPER_BACKUP" "$HELPER_TARGET"; then
        echo "ERROR: Could not restore Wi-Fi helper backup."
        exit 1
    fi
fi

if [ "$CONFIG_BACKUP" = "-" ]; then
    if ! rm -f "$CONFIG_TARGET"; then
        echo "ERROR: Could not restore absent MPV config state."
        exit 1
    fi
elif ! cp -p "$CONFIG_BACKUP" "$CONFIG_TARGET"; then
    echo "ERROR: Could not restore MPV config backup."
    exit 1
fi

if [ "$RESTORE_PROFILES" -eq 1 ]; then
    if ! mkdir -p "$PROFILE_DIR"; then
        echo "ERROR: Could not create RetroArch profile directory."
        exit 1
    fi

    if [ "$GB_PROFILE_BACKUP" = "-" ]; then
        if ! rm -f "$GB_TARGET"; then
            echo "ERROR: Could not restore absent GB profile state."
            exit 1
        fi
    elif ! cp -p "$GB_PROFILE_BACKUP" "$GB_TARGET"; then
        echo "ERROR: Could not restore GB profile backup."
        exit 1
    fi

    if [ "$GBC_PROFILE_BACKUP" = "-" ]; then
        if ! rm -f "$GBC_TARGET"; then
            echo "ERROR: Could not restore absent GBC profile state."
            exit 1
        fi
    elif ! cp -p "$GBC_PROFILE_BACKUP" "$GBC_TARGET"; then
        echo "ERROR: Could not restore GBC profile backup."
        exit 1
    fi

    if [ "$JOY_PROFILE_BACKUP" = "-" ]; then
        if ! rm -f "$JOY_TARGET"; then
            echo "ERROR: Could not restore absent controller profile state."
            exit 1
        fi
    elif ! cp -p "$JOY_PROFILE_BACKUP" "$JOY_TARGET"; then
        echo "ERROR: Could not restore controller profile backup."
        exit 1
    fi
fi

echo "Files restored. Launching KiddOS..."
if ! systemd-run --quiet --no-block --service-type=exec \
    --unit=kiddos-ui \
    --property=Restart=on-failure \
    --property=RestartSec=2 \
    --setenv=HOME=/storage "$TARGET"; then
    echo "ERROR: Could not launch restored KiddOS."
    exit 1
fi

sleep 4
if systemctl is-active --quiet kiddos-ui.service; then
    echo "ROLLBACK SUCCESS: KiddOS is running."
    sha256sum "$TARGET"
    if [ -f "$CONFIG_TARGET" ]; then
        sha256sum "$CONFIG_TARGET"
    else
        echo "MPV config: absent (restored previous state)"
    fi
    if [ "$HELPER_BACKUP" != "__UNCHANGED__" ]; then
        if [ -f "$HELPER_TARGET" ]; then
            sha256sum "$HELPER_TARGET"
        else
            echo "Wi-Fi helper: absent (restored previous state)"
        fi
    fi
    if [ "$RESTORE_PROFILES" -eq 1 ]; then
        for PROFILE in "$GB_TARGET" "$GBC_TARGET" "$JOY_TARGET"; do
            if [ -f "$PROFILE" ]; then
                sha256sum "$PROFILE"
            else
                echo "$PROFILE: absent (restored previous state)"
            fi
        done
    fi
    exit 0
fi

echo "ERROR: Restored KiddOS is not active."
exit 1
REMOTE_SCRIPT
    return $?
}

case "${1:-}" in
    deploy)
        deploy_kiddos
        ;;
    rollback)
        rollback_kiddos "${2:-}" "${3:-}" "${4:-}" "${5:-}" "${6:-}"
        ;;
    *)
        echo "Usage: $0 [deploy | rollback EXECUTABLE_BACKUP MPV_CONFIG_BACKUP|- [GB_PROFILE_BACKUP|- GBC_PROFILE_BACKUP|- CONTROLLER_PROFILE_BACKUP|-]]"
        false
        ;;
esac
