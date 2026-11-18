#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import json
import sys

import dbus


def list_services():
    bus = dbus.SystemBus()
    manager = dbus.Interface(
        bus.get_object("net.connman", "/"),
        "net.connman.Manager",
    )

    services = []

    for path, props in manager.GetServices():
        if str(props.get("Type", "")) != "wifi":
            continue

        name = props.get("Name")
        security = props.get("Security", [])

        services.append({
            "path": str(path),
            "name": str(name) if name is not None else None,
            "strength": int(props.get("Strength", 0)),
            "security": [str(item) for item in security],
            "state": str(props.get("State", "unknown")),
            "favorite": bool(props.get("Favorite", False)),
            "autoconnect": bool(props.get("AutoConnect", False)),
        })

    services.sort(
        key=lambda item: (
            item["state"] == "online",
            item["strength"],
        ),
        reverse=True,
    )

    return services


def scan_wifi():
    bus = dbus.SystemBus()
    technology = dbus.Interface(
        bus.get_object(
            "net.connman",
            "/net/connman/technology/wifi",
        ),
        "net.connman.Technology",
    )
    technology.Scan()
    return list_services()


def print_ui_services(services):
    for item in services:
        name = item["name"] or "Hidden network"
        security = "open" if "none" in item["security"] else "secured"
        fields = (
            item["path"],
            name,
            str(item["strength"]),
            security,
            item["state"],
        )
        print(chr(9).join(fields))



AGENT_PATH = "/org/retro_kiddos/ConnManAgent"


def connect_service(service_path, password):
    import ctypes
    import dbus.service
    import dbus.mainloop.glib

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    manager = dbus.Interface(
        bus.get_object("net.connman", "/"),
        "net.connman.Manager",
    )

    glib = ctypes.CDLL("libglib-2.0.so.0")
    glib.g_main_loop_new.restype = ctypes.c_void_p
    glib.g_main_loop_new.argtypes = [ctypes.c_void_p, ctypes.c_int]
    glib.g_main_loop_run.argtypes = [ctypes.c_void_p]
    glib.g_main_loop_quit.argtypes = [ctypes.c_void_p]
    loop = glib.g_main_loop_new(None, False)

    result = {"done": False, "error": None}

    class Agent(dbus.service.Object):
        @dbus.service.method(
            "net.connman.Agent",
            in_signature="oa{sv}",
            out_signature="a{sv}",
        )
        def RequestInput(self, requested_path, fields):
            if str(requested_path) != service_path:
                raise dbus.DBusException("Unexpected service request")

            response = {}
            if "Passphrase" in fields:
                response["Passphrase"] = dbus.String(password)
            if "Identity" in fields or "Username" in fields:
                raise dbus.DBusException(
                    "This network requires unsupported credentials"
                )
            return dbus.Dictionary(response, signature="sv")

        @dbus.service.method(
            "net.connman.Agent", in_signature="os", out_signature=""
        )
        def ReportError(self, requested_path, error):
            result["error"] = "Network authentication failed: " + str(error)
            result["done"] = True
            glib.g_main_loop_quit(loop)

        @dbus.service.method(
            "net.connman.Agent", in_signature="os", out_signature=""
        )
        def RequestBrowser(self, requested_path, url):
            result["error"] = "Network requires browser authentication"
            result["done"] = True
            glib.g_main_loop_quit(loop)

        @dbus.service.method(
            "net.connman.Agent", in_signature="", out_signature=""
        )
        def Cancel(self):
            result["error"] = "Connection cancelled"
            result["done"] = True
            glib.g_main_loop_quit(loop)

        @dbus.service.method(
            "net.connman.Agent", in_signature="", out_signature=""
        )
        def Release(self):
            pass

    agent = Agent(bus, AGENT_PATH)
    registered = False

    def connected():
        result["done"] = True
        glib.g_main_loop_quit(loop)

    def failed(error):
        result["error"] = str(error)
        result["done"] = True
        glib.g_main_loop_quit(loop)

    try:
        manager.RegisterAgent(AGENT_PATH)
        registered = True

        service = dbus.Interface(
            bus.get_object("net.connman", service_path),
            "net.connman.Service",
        )
        service.Connect(
            reply_handler=connected,
            error_handler=failed,
        )
        glib.g_main_loop_run(loop)

        if result["error"]:
            raise RuntimeError(result["error"])

        return True
    finally:
        if registered:
            try:
                manager.UnregisterAgent(AGENT_PATH)
            except Exception:
                pass
        agent.remove_from_connection()

def main():
    if len(sys.argv) == 3 and sys.argv[1] == "connect":
        try:
            service_path = sys.argv[2]
            if not service_path.startswith("/net/connman/service/"):
                raise ValueError("Invalid ConnMan service path")
            password = sys.stdin.readline().rstrip("\r\n")
            connect_service(service_path, password)
            print("CONNECTED", flush=True)
            return 0
        except Exception as exc:
            print("CONNECTION FAILED: " + str(exc), file=sys.stderr, flush=True)
            return 1
    if len(sys.argv) != 2 or sys.argv[1] not in (
        "list", "scan", "ui-list", "ui-scan"
    ):
        print(
            "Usage: wifi_helper.py list|scan|ui-list|ui-scan|connect SERVICE_PATH",
            file=sys.stderr,
        )
        return 2

    try:
        do_scan = sys.argv[1] in ("scan", "ui-scan")
        services = scan_wifi() if do_scan else list_services()

        if sys.argv[1].startswith("ui-"):
            print_ui_services(services)
            return 0

        result = {
            "ok": True,
            "services": services,
        }
        print(json.dumps(result, separators=(",", ":")))
        return 0
    except Exception as exc:
        print(
            json.dumps({"ok": False, "error": str(exc)}),
            file=sys.stdout,
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
