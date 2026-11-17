#!/bin/sh

. /etc/profile

case "${1}" in
  before)
    LOG=/storage/.config/kiddos-boot.log
    exec >>"$LOG" 2>&1

    echo "===== KiddOS boot hook: $(date) ====="

    KIDDOS=/storage/roms/.kiddos-ui-test-20261116/kiddos-ui

    if [ ! -x "$KIDDOS" ]; then
      echo "KiddOS executable missing; leaving normal startup unchanged."
      exit 0
    fi

    if ! systemctl mask --runtime emustation.service; then
      echo "Could not mask EmulationStation; leaving normal startup unchanged."
      exit 0
    fi

    if ! systemd-run \
      --no-block \
      --service-type=exec \
      --unit=kiddos-ui \
      --property=Restart=on-failure \
      --property=RestartSec=2 \
      --setenv=HOME=/storage \
      "$KIDDOS"; then
      echo "KiddOS launch failed; restoring EmulationStation."
      systemctl unmask --runtime emustation.service
    else
      echo "KiddOS transient service launched."
    fi
    ;;
esac

exit 0
