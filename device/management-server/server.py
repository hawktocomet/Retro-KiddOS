#!/usr/bin/env python3

import argparse
import json
import platform
import re
import socket
import subprocess
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

VERSION = "0.3.0"
WEB_ROOT = Path(__file__).resolve().parent / "web"
ROM_ROOT = Path("/storage/roms")

GAME_SYSTEMS = {
    "gb": ("Game Boy", {".gb", ".zip"}),
    "gbc": ("Game Boy Color", {".gbc", ".zip"}),
    "gba": ("Game Boy Advance", {".gba", ".zip"}),
    "arcade": ("Arcade", {".zip", ".7z"}),
    "snes": ("Super Nintendo", {".sfc", ".smc", ".zip"}),
    "genesis": ("Genesis / Mega Drive", {".gen", ".md", ".bin", ".smd", ".zip"}),
    "megadrive": ("Genesis / Mega Drive", {".gen", ".md", ".bin", ".smd", ".zip"}),
    "mastersystem": ("Master System", {".sms", ".zip"}),
    "nes": ("Nintendo Entertainment System", {".nes", ".zip"}),
}

MOVIE_EXTENSIONS = {".mp4", ".mkv", ".avi", ".mov", ".m4v"}


def read_text(path):
    try:
        return Path(path).read_text().strip()
    except (OSError, UnicodeError):
        return None


def run_read_only(command):
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=3,
            check=False,
        )
        if result.returncode == 0:
            return result.stdout
    except (OSError, subprocess.TimeoutExpired):
        pass
    return ""


def network_status():
    link = run_read_only(["iw", "dev", "wlan0", "link"])
    addresses = run_read_only(["ip", "-4", "-o", "addr", "show", "dev", "wlan0"])

    connected = "Connected to " in link
    ssid_match = re.search(r"^\s*SSID:\s*(.+)$", link, re.MULTILINE)
    signal_match = re.search(r"^\s*signal:\s*(-?\d+)\s*dBm", link, re.MULTILINE)
    ip_match = re.search(r"\binet\s+([0-9.]+)", addresses)

    return {
        "connected": connected,
        "ssid": ssid_match.group(1).strip() if ssid_match else None,
        "ip_address": ip_match.group(1) if ip_match else None,
        "signal_dbm": int(signal_match.group(1)) if signal_match else None,
    }


def battery_status():
    root = Path("/sys/class/power_supply")
    battery = root / "battery"

    capacity = read_text(battery / "capacity")
    status = read_text(battery / "status")

    external_power = False
    for name in ("ac", "usb"):
        if read_text(root / name / "online") == "1":
            external_power = True

    percentage = None
    if capacity is not None:
        try:
            percentage = max(0, min(100, int(capacity)))
        except ValueError:
            pass

    return {
        "percentage": percentage,
        "status": status,
        "external_power": external_power,
        "charging": status is not None and status.lower() == "charging",
    }


def metadata_names(directory):
    """Read optional EmulationStation names; ignore malformed metadata."""
    result = {}
    metadata = directory / "gamelist.xml"

    try:
        root = ET.parse(metadata).getroot()
        for game in root.findall(".//game"):
            path_node = game.find("path")
            name_node = game.find("name")
            if path_node is None or name_node is None:
                continue
            if not path_node.text or not name_node.text:
                continue

            candidate = (directory / path_node.text.strip()).resolve()
            base = directory.resolve()

            try:
                candidate.relative_to(base)
            except ValueError:
                continue

            result[candidate] = name_node.text.strip()
    except (OSError, ET.ParseError, ValueError):
        pass

    return result


def inventory_games():
    games = []
    systems = []

    groups = [
        ("gb", "Game Boy", (".gb", ".zip"), ("gb",)),
        ("gbc", "Game Boy Color", (".gbc", ".zip"), ("gbc",)),
        ("gba", "Game Boy Advance", (".gba", ".zip"), ("gba",)),
        ("arcade", "Arcade", (".zip", ".7z"), ("arcade",)),
        ("snes", "Super Nintendo", (".sfc", ".smc", ".zip"), ("snes",)),
        ("genesis-megadrive", "Genesis / Mega Drive",
         (".gen", ".md", ".bin", ".smd", ".zip"), ("genesis", "megadrive")),
        ("mastersystem", "Master System", (".sms", ".zip"), ("mastersystem",)),
        ("nes", "Nintendo Entertainment System", (".nes", ".zip"), ("nes",)),
    ]

    for system_id, display_name, extensions, folders in groups:
        existing = [ROM_ROOT / folder for folder in folders
                    if (ROM_ROOT / folder).is_dir()]
        if not existing:
            continue

        system_count = 0

        for directory in existing:
            names = metadata_names(directory)

            try:
                files = sorted(directory.iterdir(), key=lambda p: p.name.casefold())
            except OSError:
                files = []

            for item_path in files:
                if item_path.name.startswith(".") or not item_path.is_file():
                    continue
                if item_path.suffix.lower() not in extensions:
                    continue

                try:
                    resolved = item_path.resolve()
                    resolved.relative_to(directory.resolve())
                    size = item_path.stat().st_size
                except (OSError, ValueError):
                    continue

                games.append({
                    "title": names.get(resolved, item_path.stem),
                    "system": display_name,
                    "system_id": system_id,
                    "filename": item_path.name,
                    "path": str(item_path),
                    "size_bytes": size,
                })
                system_count += 1

        systems.append({
            "id": system_id,
            "name": display_name,
            "count": system_count,
        })

    return games, systems


def inventory_movies():
    directory = ROM_ROOT / "movies"
    movies = []

    if not directory.is_dir():
        return movies

    try:
        files = sorted(directory.iterdir(), key=lambda p: p.name.casefold())
    except OSError:
        files = []

    for path in files:
        if path.name.startswith(".") or not path.is_file():
            continue
        if path.suffix.lower() not in MOVIE_EXTENSIONS:
            continue

        try:
            resolved = path.resolve()
            resolved.relative_to(directory.resolve())
            size = path.stat().st_size
        except (OSError, ValueError):
            continue

        movies.append({
            "title": path.stem,
            "filename": path.name,
            "path": str(path),
            "size_bytes": size,
        })

    return movies


def library_status():
    games, systems = inventory_games()
    movies = inventory_movies()

    return {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "games": games,
        "systems": systems,
        "movies": movies,
        "summary": {
            "game_count": len(games),
            "movie_count": len(movies),
            "music_count": None,
        },
    }


def device_status():
    return {
        "application": "Retro KiddOS Management",
        "version": VERSION,
        "hostname": socket.gethostname(),
        "platform": f"{platform.system()} {platform.machine()}",
        "python": platform.python_version(),
        "server": "running",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "network": network_status(),
        "battery": battery_status(),
    }


class Handler(BaseHTTPRequestHandler):
    def send_bytes(self, status, content_type, body):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path

        if path == "/api/status":
            body = json.dumps(device_status(), indent=2).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return

        if path == "/api/library":
            body = json.dumps(library_status(), indent=2).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return

        if path == "/":
            page = WEB_ROOT / "index.html"
            try:
                body = page.read_bytes()
            except OSError:
                self.send_bytes(500, "text/plain; charset=utf-8",
                                b"Management page unavailable")
                return
            self.send_bytes(200, "text/html; charset=utf-8", body)
            return

        self.send_bytes(404, "text/plain; charset=utf-8", b"Not found")

    def do_POST(self):
        self.send_bytes(405, "text/plain; charset=utf-8",
                        b"Read-only prototype")

    def log_message(self, fmt, *args):
        print("%s - %s" % (self.address_string(), fmt % args))


def main():
    global ROM_ROOT
    parser = argparse.ArgumentParser(description="Retro KiddOS management server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8081)
    parser.add_argument("--rom-root", default=str(ROM_ROOT))
    args = parser.parse_args()
    ROM_ROOT = Path(args.rom_root)

    try:
        server = ThreadingHTTPServer((args.host, args.port), Handler)
    except OSError as exc:
        print(f"Could not start management server: {exc}")
        return

    print(f"Retro KiddOS Management {VERSION}")
    print(f"Listening on http://{args.host}:{args.port}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping management server.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
