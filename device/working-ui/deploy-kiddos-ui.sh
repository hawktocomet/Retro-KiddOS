#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

deploy_kiddos() {
    local SCRIPT_DIR PROJECT_ROOT LOCAL
    local REMOTE TARGET STAGED REMOTE_DIR
    local CONFIG_LOCAL CONFIG_TARGET CONFIG_STAGED CONFIG_BACKUP
    local HELPER_LOCAL HELPER_TARGET HELPER_STAGED HELPER_BACKUP
    local GB_LOCAL GBC_LOCAL GB_STAGED GBC_STAGED JOY_LOCAL JOY_STAGED
    local EARLY_SCRIPT_LOCAL EARLY_SCRIPT_TARGET EARLY_SCRIPT_STAGED EARLY_SCRIPT_BACKUP EARLY_SCRIPT_EXPECTED
    local EARLY_UNIT_LOCAL EARLY_UNIT_TARGET EARLY_UNIT_STAGED EARLY_UNIT_BACKUP EARLY_UNIT_EXPECTED
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
    EARLY_SCRIPT_LOCAL="$PROJECT_ROOT/firmware/boot/kiddos-early-start.sh"
    EARLY_SCRIPT_TARGET="/storage/.config/kiddos-early-start.sh"
    EARLY_SCRIPT_STAGED="$REMOTE_DIR/kiddos-early-start.sh.new"
    EARLY_SCRIPT_BACKUP="${EARLY_SCRIPT_TARGET}.pre-deploy-${DEPLOY_ID}"
    EARLY_UNIT_LOCAL="$PROJECT_ROOT/firmware/boot/kiddos-early.service"
    EARLY_UNIT_TARGET="/storage/.config/system.d/kiddos-early.service"
    EARLY_UNIT_STAGED="$REMOTE_DIR/kiddos-early.service.new"
    EARLY_UNIT_BACKUP="${EARLY_UNIT_TARGET}.pre-deploy-${DEPLOY_ID}"

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
    if [[ ! -f "$EARLY_SCRIPT_LOCAL" || ! -f "$EARLY_UNIT_LOCAL" ]]; then
        echo "ERROR: Early-start script or systemd unit source is missing."
        return 1
    fi
    if ! sh -n "$EARLY_SCRIPT_LOCAL"; then
        echo "ERROR: Early-start script has invalid shell syntax."
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
    EARLY_SCRIPT_EXPECTED="$(sha256sum "$EARLY_SCRIPT_LOCAL" | cut -d ' ' -f 1)"
    EARLY_UNIT_EXPECTED="$(sha256sum "$EARLY_UNIT_LOCAL" | cut -d ' ' -f 1)"
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

    if ! ssh "$REMOTE" "mkdir -p '/storage/roms/.kiddos' '$REMOTE_DIR/parts' /storage/.config/kiddos /storage/.config/system.d /storage/joypads && rm -f '$REMOTE_DIR'/parts/chunk-* '$STAGED' '$HELPER_STAGED' '$CONFIG_STAGED' '$GB_STAGED' '$GBC_STAGED' '$JOY_STAGED' '$EARLY_SCRIPT_STAGED' '$EARLY_UNIT_STAGED'"; then
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
    if ! scp -q "$EARLY_SCRIPT_LOCAL" "$REMOTE:$EARLY_SCRIPT_STAGED"; then
        echo "ERROR: Early-start script transfer failed."
        rm -rf "$WORK"
        return 1
    fi
    if ! scp -q "$EARLY_UNIT_LOCAL" "$REMOTE:$EARLY_UNIT_STAGED"; then
        echo "ERROR: Early-start unit transfer failed."
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

    for PROFILE in "$EARLY_SCRIPT_STAGED:$EARLY_SCRIPT_EXPECTED" "$EARLY_UNIT_STAGED:$EARLY_UNIT_EXPECTED"; do
        REMOTE_FILE="${PROFILE%%:*}"
        EXPECTED_HASH="${PROFILE##*:}"
        ACTUAL_HASH="$(ssh "$REMOTE" "sha256sum '$REMOTE_FILE' | cut -d ' ' -f 1")"
        STATUS=$?
        if [[ "$STATUS" -ne 0 || "$ACTUAL_HASH" != "$EXPECTED_HASH" ]]; then
            echo "ERROR: Boot-file checksum verification failed: $REMOTE_FILE"
            rm -rf "$WORK"
            return 1
        fi
        echo "Verified: $REMOTE_FILE"
    done

    echo
    echo "===== INSTALLING AND LAUNCHING ====="
    ssh "$REMOTE" sh -s -- "$TARGET" "$STAGED" "$BACKUP" "$EXPECTED" "$HELPER_TARGET" "$HELPER_STAGED" "$HELPER_BACKUP" "$HELPER_EXPECTED" "$CONFIG_TARGET" "$CONFIG_STAGED" "$CONFIG_BACKUP" "$CONFIG_EXPECTED" "/storage/.config/kiddos/retroarch" "$GB_STAGED" "$GBC_STAGED" "$GB_EXPECTED" "$GBC_EXPECTED" "$JOY_STAGED" "$JOY_EXPECTED" "$EARLY_SCRIPT_TARGET" "$EARLY_SCRIPT_STAGED" "$EARLY_SCRIPT_BACKUP" "$EARLY_SCRIPT_EXPECTED" "$EARLY_UNIT_TARGET" "$EARLY_UNIT_STAGED" "$EARLY_UNIT_BACKUP" "$EARLY_UNIT_EXPECTED" <<'REMOTE_SCRIPT'
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
EARLY_SCRIPT_TARGET="${20}"
EARLY_SCRIPT_STAGED="${21}"
EARLY_SCRIPT_BACKUP="${22}"
EARLY_SCRIPT_EXPECTED="${23}"
EARLY_UNIT_TARGET="${24}"
EARLY_UNIT_STAGED="${25}"
EARLY_UNIT_BACKUP="${26}"
EARLY_UNIT_EXPECTED="${27}"
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
EARLY_SCRIPT_HAD_BACKUP=0
EARLY_UNIT_HAD_BACKUP=0
EARLY_FILES_INSTALL_STARTED=0
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
    if [ "$EARLY_FILES_INSTALL_STARTED" -eq 1 ]; then
        if [ "$EARLY_SCRIPT_HAD_BACKUP" -eq 1 ]; then
            if ! cp -p "$EARLY_SCRIPT_BACKUP" "$EARLY_SCRIPT_TARGET"; then
                echo "CRITICAL: Could not restore early-start script."
                return 1
            fi
        else
            if ! rm -f "$EARLY_SCRIPT_TARGET"; then
                echo "CRITICAL: Could not remove early-start script."
                return 1
            fi
        fi
        if [ "$EARLY_UNIT_HAD_BACKUP" -eq 1 ]; then
            if ! cp -p "$EARLY_UNIT_BACKUP" "$EARLY_UNIT_TARGET"; then
                echo "CRITICAL: Could not restore early-start unit."
                return 1
            fi
        else
            if ! rm -f "$EARLY_UNIT_TARGET"; then
                echo "CRITICAL: Could not remove early-start unit."
                return 1
            fi
        fi
        if ! systemctl daemon-reload; then
            echo "CRITICAL: Could not reload systemd after restoring boot files."
            return 1
        fi
    fi
    if [ "$EXECUTABLE_HAD_BACKUP" -eq 0 ]; then
        echo "FIRST-INSTALL ROLLBACK: New executable and helper removed."
        LEGACY_TARGET="/storage/roms/.kiddos-ui-test-20261116/kiddos-ui"
        if [ -x "$LEGACY_TARGET" ]; then
            echo "Relaunching preserved legacy KiddOS: $LEGACY_TARGET"
            if ! systemd-run --quiet --no-block --service-type=exec \
                --unit="$UNIT" \
                --property=Restart=on-failure \
                --property=RestartSec=2 \
                --setenv=HOME=/storage "$LEGACY_TARGET"; then
                echo "CRITICAL: Could not relaunch legacy KiddOS."
                return 1
            fi
            sleep 4
            if systemctl is-active --quiet "$UNIT.service"; then
                echo "ROLLBACK SUCCESS: Legacy KiddOS is running."
                sha256sum "$LEGACY_TARGET"
                return 0
            fi
            echo "CRITICAL: Legacy KiddOS did not become active."
            return 1
        fi
        echo "WARNING: No legacy executable exists to relaunch."
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
    if [ ! -f "$EARLY_SCRIPT_STAGED" ] || [ "$(sha256sum "$EARLY_SCRIPT_STAGED" | cut -d " " -f 1)" != "$EARLY_SCRIPT_EXPECTED" ] || ! sh -n "$EARLY_SCRIPT_STAGED"; then
        echo "ERROR: Staged early-start script failed checksum or syntax verification."
        return 1
    fi
    if [ ! -f "$EARLY_UNIT_STAGED" ] || [ "$(sha256sum "$EARLY_UNIT_STAGED" | cut -d " " -f 1)" != "$EARLY_UNIT_EXPECTED" ]; then
        echo "ERROR: Staged early-start unit checksum verification failed."
        return 1
    fi
    if [ ! -x "$STAGED" ]; then
        echo "ERROR: Verified staged executable missing."
        return 1
    fi
    if [ -e "$BACKUP" ] || [ -e "$HELPER_BACKUP" ] || [ -e "$CONFIG_BACKUP" ] ||
       [ -e "$GB_BACKUP" ] || [ -e "$GBC_BACKUP" ] || [ -e "$JOY_BACKUP" ] ||
       [ -e "$EARLY_SCRIPT_BACKUP" ] || [ -e "$EARLY_UNIT_BACKUP" ]; then
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
    if [ -e "$EARLY_SCRIPT_TARGET" ]; then
        if ! cp -p "$EARLY_SCRIPT_TARGET" "$EARLY_SCRIPT_BACKUP"; then
            echo "ERROR: Could not back up existing early-start script."
            return 1
        fi
        EARLY_SCRIPT_HAD_BACKUP=1
        echo "Early-start script backup: $EARLY_SCRIPT_BACKUP"
    fi
    if [ -e "$EARLY_UNIT_TARGET" ]; then
        if ! cp -p "$EARLY_UNIT_TARGET" "$EARLY_UNIT_BACKUP"; then
            echo "ERROR: Could not back up existing early-start unit."
            return 1
        fi
        EARLY_UNIT_HAD_BACKUP=1
        echo "Early-start unit backup: $EARLY_UNIT_BACKUP"
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
    EARLY_FILES_INSTALL_STARTED=1
    if ! mv "$EARLY_SCRIPT_STAGED" "$EARLY_SCRIPT_TARGET"; then
        echo "ERROR: Could not install early-start script."
        restore_previous
        return 1
    fi
    if ! chmod 755 "$EARLY_SCRIPT_TARGET"; then
        echo "ERROR: Could not set early-start script permissions."
        restore_previous
        return 1
    fi
    if ! mv "$EARLY_UNIT_STAGED" "$EARLY_UNIT_TARGET"; then
        echo "ERROR: Could not install early-start unit."
        restore_previous
        return 1
    fi
    if ! chmod 644 "$EARLY_UNIT_TARGET"; then
        echo "ERROR: Could not set early-start unit permissions."
        restore_previous
        return 1
    fi
    if ! systemctl daemon-reload; then
        echo "ERROR: systemd daemon-reload failed."
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
            if [ "$EARLY_SCRIPT_HAD_BACKUP" -eq 1 ]; then
                echo "Early-start script backup: $EARLY_SCRIPT_BACKUP"
            else
                echo "Early-start script backup: - (first install)"
            fi
            if [ "$EARLY_UNIT_HAD_BACKUP" -eq 1 ]; then
                echo "Early-start unit backup: $EARLY_UNIT_BACKUP"
            else
                echo "Early-start unit backup: - (first install)"
            fi
            echo "Early-start service enablement was not changed."
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
        if ssh "$REMOTE" "[ -e '$EARLY_SCRIPT_BACKUP' ]"; then
            echo "Early-start script backup: $EARLY_SCRIPT_BACKUP"
        else
            echo "Early-start script backup: -"
        fi
        if ssh "$REMOTE" "[ -e '$EARLY_UNIT_BACKUP' ]"; then
            echo "Early-start unit backup: $EARLY_UNIT_BACKUP"
        else
            echo "Early-start unit backup: -"
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
    local GB_PROFILE_BACKUP="${3:-__UNCHANGED__}"
    local GBC_PROFILE_BACKUP="${4:-__UNCHANGED__}"
    local JOY_PROFILE_BACKUP="${5:-__UNCHANGED__}"
    local HELPER_BACKUP="${6:-__UNCHANGED__}"
    local EARLY_SCRIPT_BACKUP="${7:-__UNCHANGED__}"
    local EARLY_UNIT_BACKUP="${8:-__UNCHANGED__}"
    local REMOTE TARGET HELPER_TARGET CONFIG_TARGET PROFILE_DIR RESTORE_PROFILES=0

    REMOTE="${KIDDOS_DEVICE:-root@192.168.0.89}"
    TARGET="/storage/roms/.kiddos/kiddos-ui"
    HELPER_TARGET="/storage/roms/.kiddos/wifi_helper.py"
    CONFIG_TARGET="/storage/.config/kiddos/mpv-input.conf"
    PROFILE_DIR="/storage/.config/kiddos/retroarch"

    if [[ -z "$BACKUP" || -z "$CONFIG_BACKUP" ]]; then
        echo "Usage: $0 rollback EXECUTABLE_BACKUP MPV_CONFIG_BACKUP|- [GB_PROFILE_BACKUP|__UNCHANGED__|- GBC_PROFILE_BACKUP|__UNCHANGED__|- CONTROLLER_PROFILE_BACKUP|__UNCHANGED__|- [WIFI_HELPER_BACKUP|- [EARLY_SCRIPT_BACKUP|- [EARLY_UNIT_BACKUP|-]]]]]"
        return 2
    fi

    if [[ "$GB_PROFILE_BACKUP" != "__UNCHANGED__" || "$GBC_PROFILE_BACKUP" != "__UNCHANGED__" || "$JOY_PROFILE_BACKUP" != "__UNCHANGED__" ]]; then
        if [[ "$GB_PROFILE_BACKUP" == "__UNCHANGED__" || "$GBC_PROFILE_BACKUP" == "__UNCHANGED__" || "$JOY_PROFILE_BACKUP" == "__UNCHANGED__" ]]; then
            echo "ERROR: Supply all three profile arguments, or use __UNCHANGED__ for all three."
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

    if [[ "$EARLY_SCRIPT_BACKUP" == "-" ]]; then
        echo "Early-start script: restore previous absent state"
    elif [[ "$EARLY_SCRIPT_BACKUP" != "__UNCHANGED__" ]]; then
        echo "Early-start script backup: $EARLY_SCRIPT_BACKUP"
    fi
    if [[ "$EARLY_UNIT_BACKUP" == "-" ]]; then
        echo "Early-start unit: restore previous absent state"
    elif [[ "$EARLY_UNIT_BACKUP" != "__UNCHANGED__" ]]; then
        echo "Early-start unit backup: $EARLY_UNIT_BACKUP"
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
        "$PROFILE_DIR" "$GB_PROFILE_BACKUP" "$GBC_PROFILE_BACKUP" "$JOY_PROFILE_BACKUP" "$RESTORE_PROFILES" "$HELPER_BACKUP" "$EARLY_SCRIPT_BACKUP" "$EARLY_UNIT_BACKUP" <<'REMOTE_SCRIPT'
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
EARLY_SCRIPT_BACKUP="${11}"
EARLY_UNIT_BACKUP="${12}"
HELPER_TARGET="/storage/roms/.kiddos/wifi_helper.py"
EARLY_SCRIPT_TARGET="/storage/.config/kiddos-early-start.sh"
EARLY_UNIT_TARGET="/storage/.config/system.d/kiddos-early.service"

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
if [ "$EARLY_SCRIPT_BACKUP" != "__UNCHANGED__" ] &&
   [ "$EARLY_SCRIPT_BACKUP" != "-" ] && [ ! -f "$EARLY_SCRIPT_BACKUP" ]; then
    echo "ERROR: Early-start script backup not found: $EARLY_SCRIPT_BACKUP"
    exit 1
fi
if [ "$EARLY_UNIT_BACKUP" != "__UNCHANGED__" ] &&
   [ "$EARLY_UNIT_BACKUP" != "-" ] && [ ! -f "$EARLY_UNIT_BACKUP" ]; then
    echo "ERROR: Early-start unit backup not found: $EARLY_UNIT_BACKUP"
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

if [ "$EARLY_SCRIPT_BACKUP" = "-" ]; then
    if ! rm -f "$EARLY_SCRIPT_TARGET"; then
        echo "ERROR: Could not restore absent early-start script state."
        exit 1
    fi
elif [ "$EARLY_SCRIPT_BACKUP" != "__UNCHANGED__" ]; then
    if ! cp -p "$EARLY_SCRIPT_BACKUP" "$EARLY_SCRIPT_TARGET"; then
        echo "ERROR: Could not restore early-start script backup."
        exit 1
    fi
fi

if [ "$EARLY_UNIT_BACKUP" = "-" ]; then
    if ! rm -f "$EARLY_UNIT_TARGET"; then
        echo "ERROR: Could not restore absent early-start unit state."
        exit 1
    fi
elif [ "$EARLY_UNIT_BACKUP" != "__UNCHANGED__" ]; then
    if ! cp -p "$EARLY_UNIT_BACKUP" "$EARLY_UNIT_TARGET"; then
        echo "ERROR: Could not restore early-start unit backup."
        exit 1
    fi
fi
if [ "$EARLY_UNIT_BACKUP" != "__UNCHANGED__" ]; then
    if ! systemctl daemon-reload; then
        echo "ERROR: systemd daemon-reload failed after restoring early-start unit."
        exit 1
    fi
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
    if [ "$EARLY_SCRIPT_BACKUP" != "__UNCHANGED__" ]; then
        if [ -f "$EARLY_SCRIPT_TARGET" ]; then
            sha256sum "$EARLY_SCRIPT_TARGET"
        else
            echo "Early-start script: absent (restored previous state)"
        fi
    fi
    if [ "$EARLY_UNIT_BACKUP" != "__UNCHANGED__" ]; then
        if [ -f "$EARLY_UNIT_TARGET" ]; then
            sha256sum "$EARLY_UNIT_TARGET"
        else
            echo "Early-start unit: absent (restored previous state)"
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


audio_deploy_kiddos() {
    local SCRIPT_DIR PROJECT_ROOT LOCAL CONFIG_LOCAL
    local REMOTE TARGET STAGED BACKUP CONFIG_TARGET CONFIG_STAGED CONFIG_BACKUP
    local EXPECTED CONFIG_EXPECTED DEPLOY_ID STATUS

    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)" || return 1
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)" || return 1

    REMOTE="${KIDDOS_DEVICE:-root@192.168.0.89}"
    TARGET="/storage/roms/.kiddos/kiddos-ui"
    STAGED="${TARGET}.audio.new"
    CONFIG_LOCAL="$SCRIPT_DIR/alsa-dmix.conf"
    CONFIG_TARGET="/storage/.config/kiddos/alsa-dmix.conf"
    CONFIG_STAGED="${CONFIG_TARGET}.new"
    DEPLOY_ID="$(date +%Y%m%d-%H%M%S)"
    BACKUP="${TARGET}.pre-audio-deploy-${DEPLOY_ID}"
    CONFIG_BACKUP="${CONFIG_TARGET}.pre-audio-deploy-${DEPLOY_ID}"
    LOCAL="$PROJECT_ROOT/../Build-Workspace/build/arm64/kiddos-ui"

    if [[ ! -f "$LOCAL" || ! -x "$LOCAL" ]]; then
        echo "ERROR: Built KiddOS executable is missing or not executable: $LOCAL"
        return 1
    fi
    if [[ ! -f "$CONFIG_LOCAL" ]]; then
        echo "ERROR: ALSA dmix config is missing: $CONFIG_LOCAL"
        return 1
    fi
    for command_name in ssh scp sha256sum file; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "ERROR: Required command not found: $command_name"
            return 1
        fi
    done

    EXPECTED="$(sha256sum "$LOCAL" | cut -d ' ' -f 1)"
    CONFIG_EXPECTED="$(sha256sum "$CONFIG_LOCAL" | cut -d ' ' -f 1)"

    echo "===== AUDIO-ONLY DEPLOYMENT ====="
    file "$LOCAL"
    echo "Executable SHA-256: $EXPECTED"
    echo "ALSA config SHA-256: $CONFIG_EXPECTED"
    echo "Device: $REMOTE"

    if ! ssh "$REMOTE" "mkdir -p /storage/roms/.kiddos /storage/.config/kiddos && rm -f '$STAGED' '$CONFIG_STAGED'"; then
        echo "ERROR: Could not prepare remote staging directories."
        return 1
    fi

    if ! scp -q "$LOCAL" "$REMOTE:$STAGED"; then
        echo "ERROR: Executable transfer failed."
        return 1
    fi
    if ! scp -q "$CONFIG_LOCAL" "$REMOTE:$CONFIG_STAGED"; then
        echo "ERROR: ALSA config transfer failed."
        return 1
    fi

    ssh "$REMOTE" sh -s -- \
        "$TARGET" "$STAGED" "$BACKUP" "$EXPECTED" \
        "$CONFIG_TARGET" "$CONFIG_STAGED" "$CONFIG_BACKUP" "$CONFIG_EXPECTED" <<'REMOTE_AUDIO_SCRIPT'
TARGET="$1"
STAGED="$2"
BACKUP="$3"
EXPECTED="$4"
CONFIG_TARGET="$5"
CONFIG_STAGED="$6"
CONFIG_BACKUP="$7"
CONFIG_EXPECTED="$8"
UNIT="kiddos-ui"
EXECUTABLE_HAD_BACKUP=0
CONFIG_HAD_BACKUP=0

launch_kiddos() {
    systemd-run --quiet --no-block --service-type=exec \
        --unit="$UNIT" \
        --property=Restart=on-failure \
        --property=RestartSec=2 \
        --setenv=HOME=/storage "$TARGET"
}

restore_audio_files() {
    echo "Attempting audio deployment rollback..."
    systemctl stop "$UNIT.service" || {
        echo "CRITICAL: Could not stop KiddOS for rollback."
        return 1
    }

    if [ "$EXECUTABLE_HAD_BACKUP" -eq 1 ]; then
        cp -p "$BACKUP" "$TARGET" || {
            echo "CRITICAL: Could not restore executable backup: $BACKUP"
            return 1
        }
    else
        rm -f "$TARGET" || return 1
    fi

    if [ "$CONFIG_HAD_BACKUP" -eq 1 ]; then
        cp -p "$CONFIG_BACKUP" "$CONFIG_TARGET" || {
            echo "CRITICAL: Could not restore ALSA config backup: $CONFIG_BACKUP"
            return 1
        }
    else
        rm -f "$CONFIG_TARGET" || return 1
    fi

    echo "Previous executable and ALSA config states restored."
    if launch_kiddos; then
        sleep 4
        if systemctl is-active --quiet "$UNIT.service"; then
            echo "ROLLBACK SUCCESS: Previous KiddOS is running."
            sha256sum "$TARGET"
            [ -f "$CONFIG_TARGET" ] && sha256sum "$CONFIG_TARGET" || true
            return 0
        fi
    fi

    echo "CRITICAL: Previous KiddOS could not be relaunched."
    return 1
}

if [ ! -f "$STAGED" ] ||
   [ "$(sha256sum "$STAGED" | cut -d ' ' -f 1)" != "$EXPECTED" ]; then
    echo "ERROR: Staged executable is missing or failed checksum verification."
    exit 1
fi
if ! chmod 755 "$STAGED"; then
    echo "ERROR: Could not set staged executable permissions."
    exit 1
fi
if [ ! -f "$CONFIG_STAGED" ] ||
   [ "$(sha256sum "$CONFIG_STAGED" | cut -d ' ' -f 1)" != "$CONFIG_EXPECTED" ]; then
    echo "ERROR: Staged ALSA config is missing or failed checksum verification."
    exit 1
fi
if [ -e "$BACKUP" ] || [ -e "$CONFIG_BACKUP" ]; then
    echo "ERROR: A timestamped backup path already exists; refusing deployment."
    exit 1
fi

echo "Both staged files passed checksum verification."

if [ -e "$TARGET" ]; then
    cp -p "$TARGET" "$BACKUP" || {
        echo "ERROR: Could not back up existing executable."
        exit 1
    }
    EXECUTABLE_HAD_BACKUP=1
    echo "Executable backup: $BACKUP"
else
    echo "Executable backup: - (no previous executable)"
fi

if [ -e "$CONFIG_TARGET" ]; then
    cp -p "$CONFIG_TARGET" "$CONFIG_BACKUP" || {
        echo "ERROR: Could not back up existing ALSA config."
        exit 1
    }
    CONFIG_HAD_BACKUP=1
    echo "ALSA config backup: $CONFIG_BACKUP"
else
    echo "ALSA config backup: - (first install)"
fi

if ! systemctl stop "$UNIT.service"; then
    echo "ERROR: Could not stop existing KiddOS service."
    exit 1
fi
if systemctl is-active --quiet "$UNIT.service"; then
    echo "ERROR: KiddOS service is still active; refusing to replace files."
    exit 1
fi

if ! mv "$STAGED" "$TARGET" || ! chmod 755 "$TARGET"; then
    echo "ERROR: Could not install the new executable."
    restore_audio_files
    exit 1
fi

if ! mv "$CONFIG_STAGED" "$CONFIG_TARGET" || ! chmod 644 "$CONFIG_TARGET"; then
    echo "ERROR: Could not install the ALSA dmix config."
    restore_audio_files
    exit 1
fi

if launch_kiddos; then
    sleep 4
    if systemctl is-active --quiet "$UNIT.service"; then
        echo "AUDIO DEPLOY SUCCESS: KiddOS is running."
        sha256sum "$TARGET" "$CONFIG_TARGET"
        if [ "$EXECUTABLE_HAD_BACKUP" -eq 1 ]; then
            echo "Executable backup: $BACKUP"
        else
            echo "Executable backup: - (no previous executable)"
        fi
        if [ "$CONFIG_HAD_BACKUP" -eq 1 ]; then
            echo "ALSA config backup: $CONFIG_BACKUP"
        else
            echo "ALSA config backup: - (first install)"
        fi
        exit 0
    fi
fi

echo "ERROR: KiddOS failed to remain active after audio deployment."
restore_audio_files
exit 1
REMOTE_AUDIO_SCRIPT
    STATUS=$?

    if [[ "$STATUS" -eq 0 ]]; then
        echo
        echo "AUDIO DEPLOYMENT SUCCESS."
    else
        echo
        echo "AUDIO DEPLOYMENT FAILED (remote status $STATUS)."
    fi
    return "$STATUS"
}

case "${1:-}" in
    deploy)
        deploy_kiddos
        ;;
    audio-deploy)
        audio_deploy_kiddos
        ;;
    rollback)
        rollback_kiddos "${2:-}" "${3:-}" "${4:-}" "${5:-}" "${6:-}" "${7:-}" "${8:-}" "${9:-}"
        ;;
    *)
        echo "Usage: $0 [deploy | rollback EXECUTABLE_BACKUP MPV_CONFIG_BACKUP|- [GB_PROFILE_BACKUP|__UNCHANGED__|- GBC_PROFILE_BACKUP|__UNCHANGED__|- CONTROLLER_PROFILE_BACKUP|__UNCHANGED__|- [WIFI_HELPER_BACKUP|- [EARLY_SCRIPT_BACKUP|- [EARLY_UNIT_BACKUP|-]]]]]]"
        false
        ;;
esac
