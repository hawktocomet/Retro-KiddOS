#!/usr/bin/env bash
set -Eeuo pipefail
cd /home/hawk/Projects/Retro-Pixel-Pocket/KiddOS
ROOT="$PWD"
SRC="$ROOT/device/working-ui"
WORKSPACE="${KIDDOS_BUILD_WORKSPACE:-$ROOT/../Build-Workspace}"
BIN="$WORKSPACE/build/arm64/kiddos-ui"
DEVICE="root@192.168.0.89"
STAMP="20261122-191411"
LOG="$ROOT/music-one-shot-$STAMP.log"
STATUS="$ROOT/music-one-shot-$STAMP.status"
PATCH="$ROOT/.music-one-shot-$STAMP.py"
fail(){ echo "FAILED: $*" | tee "$STATUS"; exit 1; }

echo "RUNNING" > "$STATUS"
python3 "$PATCH" || fail "Source patch failed. No deployment attempted."

echo "===== SOURCE VALIDATION =====" | tee -a "$LOG"
grep -q 'screen == Music' "$SRC/Application.cpp" || fail "Music navigation missing"
grep -q 'drawMusic(ui)' "$SRC/Application.cpp" || fail "Music renderer not wired"
grep -q 'MusicCatalog.cpp' "$SRC/CMakeLists.txt" || fail "CMake source list missing Music"
grep -q 'MusicLauncher.cpp' "$SRC/build-kiddos-ui.sh" || fail "ARM64 build source list missing Music"
grep -q -- '--no-video' "$SRC/services/MusicLauncher.cpp" || fail "Audio-only MPV option missing"
grep -q 'music' "$SRC/services/MusicCatalog.cpp" || fail "Music catalog missing"
echo "Source checks passed." | tee -a "$LOG"

echo "===== DEVICE PREFLIGHT =====" | tee -a "$LOG"
ssh -o BatchMode=yes -o ConnectTimeout=8 "$DEVICE" '
  test -x /usr/bin/mpv &&
  test -f /storage/.config/kiddos/mpv-input.conf &&
  test -x /usr/bin/python3 -o -x /usr/bin/python
' || fail "Device MPV, controller config, or Python preflight failed."
ssh "$DEVICE" 'mkdir -p /storage/roms/music; echo "Music directory ready"; ls -la /storage/roms/music | head -12' | tee -a "$LOG"

echo "===== ARM64 BUILD =====" | tee -a "$LOG"
KIDDOS_BUILD_WORKSPACE="$WORKSPACE" bash "$SRC/build-kiddos-ui.sh" 2>&1 | tee -a "$LOG" || fail "ARM64 build failed."
test -x "$BIN" || fail "ARM64 output missing."
file "$BIN" | tee -a "$LOG"
readelf -h "$BIN" | grep -q AArch64 || fail "Build output is not AArch64."
LOCAL_HASH="$(sha256sum "$BIN" | awk '{print $1}')"
echo "Build SHA256: $LOCAL_HASH" | tee -a "$LOG"

echo "===== SAFE DEVICE DEPLOYMENT =====" | tee -a "$LOG"
REMOTE_DIR="$(ssh "$DEVICE" '
  pid="$(systemctl show kiddos-ui -p MainPID --value)"
  test -n "$pid" && test "$pid" != 0
  exe="$(readlink -f "/proc/$pid/exe")"
  case "$exe" in /storage/roms/*/kiddos-ui) printf "%s\n" "$exe";; *) echo "Unexpected running executable: $exe" >&2; exit 1;; esac
')"
REMOTE_DIR="${REMOTE_DIR%/kiddos-ui}"
REMOTE_STAGE="/storage/roms/.kiddos-music-stage-$STAMP"
REMOTE_BACKUP="/storage/roms/.kiddos-music-rollback-$STAMP"
ssh "$DEVICE" "mkdir -p '$REMOTE_STAGE' '$REMOTE_BACKUP'"
scp "$BIN" "$DEVICE:$REMOTE_STAGE/kiddos-ui.new"
REMOTE_HASH="$(ssh "$DEVICE" "sha256sum '$REMOTE_STAGE/kiddos-ui.new' | awk '{print \\\$1}'")"
test "$LOCAL_HASH" = "$REMOTE_HASH" || fail "Transferred binary checksum mismatch."

ssh "$DEVICE" "
set -Eeuo pipefail
ACTIVE='$REMOTE_DIR/kiddos-ui'
NEW='$REMOTE_STAGE/kiddos-ui.new'
BACKUP='$REMOTE_BACKUP'
test -x \"\$ACTIVE\"
cp -a \"\$ACTIVE\" \"\$BACKUP/kiddos-ui.previous\"
systemctl stop kiddos-ui
install -m 755 \"\$NEW\" \"\$ACTIVE.next\"
mv -f \"\$ACTIVE.next\" \"\$ACTIVE\"
systemctl start kiddos-ui
sleep 4
if ! systemctl is-active --quiet kiddos-ui; then
  echo 'New service failed; restoring previous executable.' >&2
  systemctl stop kiddos-ui || true
  cp -a \"\$BACKUP/kiddos-ui.previous\" \"\$ACTIVE\"
  systemctl start kiddos-ui
  exit 1
fi
echo 'SERVICE_ACTIVE'
echo 'BACKUP_PATH='\"\$BACKUP\"
"
ssh "$DEVICE" "journalctl -u kiddos-ui -n 100 --no-pager" | tee -a "$LOG"
ssh "$DEVICE" "systemctl is-active kiddos-ui" | grep -qx active || fail "Service is not active."
echo "DEPLOYED executable=$REMOTE_DIR/kiddos-ui backup=$REMOTE_BACKUP hash=$LOCAL_HASH" | tee -a "$LOG"
echo "SUCCESS" > "$STATUS"
echo
echo "Deployment succeeded. Visually verify Music on RPPBLUE before committing."
echo "Music directory: /storage/roms/music"
echo "Backup retained: $REMOTE_BACKUP"
