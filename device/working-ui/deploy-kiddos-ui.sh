#!/usr/bin/env bash

deploy_kiddos() {
    local SCRIPT_DIR PROJECT_ROOT LOCAL
    local REMOTE TARGET STAGED REMOTE_DIR
    local EXPECTED ACTUAL BACKUP WORK CHUNK
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
    TARGET="/storage/roms/.kiddos-ui-test-20261116/kiddos-ui"
    STAGED="${TARGET}.new"
    REMOTE_DIR="/tmp/kiddos-deploy-stage"
    LOCAL="$PROJECT_ROOT/../Build-Workspace/build/arm64/kiddos-ui"

    if [[ ! -f "$LOCAL" ]]; then
        echo "ERROR: Build not found: $LOCAL"
        return 1
    fi
    for command_name in ssh scp split sha256sum file mktemp; do
        if ! command -v "$command_name" >/dev/null 2>&1; then
            echo "ERROR: Required command not found: $command_name"
            return 1
        fi
    done

    EXPECTED="$(sha256sum "$LOCAL" | cut -d ' ' -f 1)"
    BACKUP="${TARGET}.pre-deploy-$(date +%Y%m%d-%H%M%S)"
    WORK="$(mktemp -d)" || {
        echo "ERROR: Could not create temporary transfer directory."
        return 1
    }

    echo "===== LOCAL BUILD ====="
    file "$LOCAL"
    echo "SHA-256: $EXPECTED"
    echo "Device: $REMOTE"

    if ! ssh "$REMOTE" "mkdir -p '$REMOTE_DIR/parts' && rm -f '$REMOTE_DIR'/parts/chunk-* '$STAGED'"; then
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

    echo
    echo "===== INSTALLING AND LAUNCHING ====="
    ssh "$REMOTE" sh -s -- "$TARGET" "$STAGED" "$BACKUP" "$EXPECTED" <<'REMOTE_SCRIPT'
TARGET="$1"
STAGED="$2"
BACKUP="$3"
EXPECTED="$4"
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
    systemctl stop "$UNIT.service"
    if systemctl is-active --quiet "$UNIT.service"; then
        echo "CRITICAL: KiddOS is still active; refusing to overwrite executable."
        return 1
    fi
    if ! cp -p "$BACKUP" "$TARGET"; then
        echo "CRITICAL: Could not restore backup."
        return 1
    fi
    echo "Previous executable restored."
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
    if [ ! -x "$TARGET" ]; then
        echo "ERROR: Current executable missing or not executable."
        return 1
    fi
    if [ ! -x "$STAGED" ]; then
        echo "ERROR: Verified staged executable missing."
        return 1
    fi
    if [ -e "$BACKUP" ]; then
        echo "ERROR: Backup path already exists: $BACKUP"
        return 1
    fi

    ACTUAL="$(sha256sum "$STAGED" | cut -d " " -f 1)"
    if [ "$ACTUAL" != "$EXPECTED" ]; then
        echo "ERROR: Staged checksum changed before installation."
        return 1
    fi

    if ! cp -p "$TARGET" "$BACKUP"; then
        echo "ERROR: Could not create rollback backup."
        return 1
    fi
    echo "Rollback backup: $BACKUP"

    systemctl stop "$UNIT.service"
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

    if launch_kiddos; then
        sleep 4
        if systemctl is-active --quiet "$UNIT.service"; then
            echo "SUCCESS: New KiddOS is running."
            sha256sum "$TARGET"
            echo "Backup: $BACKUP"
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
        echo "Rollback backup: $BACKUP"
        return 0
    fi

    echo
    echo "DEPLOYMENT FAILED. Check the device output above."
    return 1
}

rollback_kiddos() {
    local BACKUP="${1:-}"
    local REMOTE TARGET

    REMOTE="${KIDDOS_DEVICE:-root@192.168.0.89}"
    TARGET="/storage/roms/.kiddos-ui-test-20261116/kiddos-ui"

    if [[ -z "$BACKUP" ]]; then
        echo "Usage: $0 rollback BACKUP_PATH"
        return 2
    fi

    echo "===== RESTORING BACKUP ====="
    ssh "$REMOTE" sh -s -- "$TARGET" "$BACKUP" <<'REMOTE_SCRIPT'
TARGET="$1"
BACKUP="$2"

if [ ! -f "$BACKUP" ]; then
    echo "ERROR: Backup not found: $BACKUP"
    false
else
    systemctl stop kiddos-ui.service
    if systemctl is-active --quiet kiddos-ui.service; then
        echo "ERROR: KiddOS is still active; refusing to overwrite executable."
        false
    elif cp -p "$BACKUP" "$TARGET"; then
        if ! chmod 755 "$TARGET"; then
            echo "ERROR: Could not set executable permissions on restored file."
            false
        else
            echo "Backup restored. Launching KiddOS..."
            systemd-run --quiet --no-block --service-type=exec \
                --unit=kiddos-ui \
                --property=Restart=on-failure \
                --property=RestartSec=2 \
                --setenv=HOME=/storage "$TARGET"
            sleep 4
            if systemctl is-active --quiet kiddos-ui.service; then
                echo "ROLLBACK SUCCESS: KiddOS is running."
                sha256sum "$TARGET"
            else
                echo "ERROR: Restored KiddOS is not active."
                false
            fi
        fi
    else
        echo "ERROR: Could not restore backup."
        false
    fi
fi
REMOTE_SCRIPT
}

case "${1:-}" in
    deploy)
        deploy_kiddos
        ;;
    rollback)
        rollback_kiddos "${2:-}"
        ;;
    *)
        echo "Usage: $0 [deploy | rollback BACKUP_PATH]"
        false
        ;;
esac
