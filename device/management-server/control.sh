#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

SERVICE="kiddos-management.service"
SERVER="/storage/.kiddos-management/server.py"

case "${1:-}" in
  status)
    if systemctl is-active --quiet "$SERVICE"; then
      printf 'running\n'
    else
      printf 'stopped\n'
    fi
    ;;

  ip)
    ip -4 -o addr show dev wlan0 2>/dev/null |
      awk '{ for (i = 1; i <= NF; i++) if ($i == "inet") {
        split($(i + 1), address, "/")
        print address[1]
        exit
      } }'
    ;;

  start)
    if [ ! -r "$SERVER" ]; then
      echo "KiddOS management server is missing: $SERVER" >&2
      exit 1
    fi

    if systemctl is-active --quiet "$SERVICE"; then
      printf 'running\n'
      exit 0
    fi

    # Clear a stale failed state for this service only.
    systemctl reset-failed "$SERVICE" >/dev/null 2>&1

    if systemd-run --quiet --no-block \
      --unit=kiddos-management \
      --property=Restart=on-failure \
      --property=RestartSec=2 \
      --setenv=HOME=/storage \
      /usr/bin/python3 "$SERVER" --host 0.0.0.0 --port 8081; then
      printf 'starting\n'
    else
      echo "Could not start KiddOS management service." >&2
      exit 1
    fi
    ;;

  stop)
    if systemctl stop "$SERVICE"; then
      printf 'stopped\n'
    else
      echo "Could not stop KiddOS management service." >&2
      exit 1
    fi
    ;;

  *)
    echo "Usage: $0 {status|start|stop}" >&2
    exit 2
    ;;
esac
