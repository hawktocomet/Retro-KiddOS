#!/bin/sh

LOG=/storage/.config/kiddos-boot.log
exec >>"$LOG" 2>&1

echo "===== KiddOS early launcher: $(date) ====="

KIDDOS=/storage/roms/.kiddos/kiddos-ui
READY=0
ATTEMPT=0

while [ "$ATTEMPT" -lt 45 ]; do
    SERVER=0

    for PROC in /proc/[0-9]*; do
        [ -r "$PROC/comm" ] || continue
        IFS= read -r NAME < "$PROC/comm"
        if [ "$NAME" = "retropixel-serv" ]; then
            SERVER=1
            break
        fi
    done

    if [ "$SERVER" = 1 ] &&
       [ -e /dev/input/js0 ] &&
       [ -e /dev/dri/card0 ] &&
       [ -x "$KIDDOS" ]; then
        READY=1
        break
    fi

    ATTEMPT=$((ATTEMPT + 1))
    sleep 1
done

if [ "$READY" != 1 ]; then
    echo "Early launcher timed out; existing startup hook will handle launch."
    exit 0
fi

if systemctl is-active --quiet kiddos-ui.service; then
    echo "KiddOS is already running; skipping early duplicate."
    exit 0
fi

echo "Hardware ready; launching KiddOS early."

if ! systemctl mask --runtime emustation.service; then
    echo "Could not mask EmulationStation; leaving fallback to startup hook."
    exit 0
fi

if ! systemd-run --no-block --service-type=exec --unit=kiddos-ui \
    --property=Restart=on-failure --property=RestartSec=2 \
    --setenv=HOME=/storage "$KIDDOS"; then
    echo "Early KiddOS launch failed; restoring EmulationStation."
    systemctl unmask --runtime emustation.service
else
    echo "Early KiddOS launch requested successfully."
fi
