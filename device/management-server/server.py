#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import argparse
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import tempfile
import threading
import time
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

import providers

from catalog import (
    ENABLED_GAME_SYSTEMS,
    GAME_SYSTEMS as CATALOG_GAME_SYSTEMS,
    assign_game,
    delete_game,
    discover_games,
    game_catalog,
    unassign_game,
)

VERSION = "0.3.0"
WEB_ROOT = Path(__file__).resolve().parent / "web"
ROM_ROOT = Path("/storage/roms")
STATE_ROOT = Path("/storage/.kiddos-management")
KIDDOS_UI = Path("/storage/roms/.kiddos-ui-test-20261116/kiddos-ui")

MOVIE_EXTENSIONS = {".mp4", ".mkv", ".avi", ".mov", ".m4v"}

MAX_ARTWORK_BYTES = 20 * 1024 * 1024
IDLE_TIMEOUT_SECONDS = 15 * 60

# Optional local artwork lookup configuration. These are deliberately not
# inferred from desktop paths; configure them explicitly at server startup.



class ManagementActivity:
    def __init__(self):
        self._lock = threading.Lock()
        self._last_activity = time.monotonic()
        self._active_requests = 0

    def begin_request(self):
        with self._lock:
            self._active_requests += 1

    def finish_request(self, successful):
        with self._lock:
            if successful:
                self._last_activity = time.monotonic()
            self._active_requests = max(0, self._active_requests - 1)

    def idle_expired(self, timeout=IDLE_TIMEOUT_SECONDS):
        with self._lock:
            return (
                self._active_requests == 0
                and time.monotonic() - self._last_activity >= timeout
            )


ACTIVITY = ManagementActivity()

class GameCatalogCache:
    def __init__(self, ttl=1.0):
        self._lock = threading.Lock()
        self._ttl = ttl
        self._key = None
        self._expires_at = 0.0
        self._catalog = None

    def get(self):
        key = (str(ROM_ROOT), str(STATE_ROOT))
        with self._lock:
            now = time.monotonic()
            if (self._catalog is None or self._key != key
                    or now >= self._expires_at):
                self._catalog = game_catalog(ROM_ROOT, STATE_ROOT)
                self._key = key
                self._expires_at = time.monotonic() + self._ttl
            return self._catalog

    def invalidate(self):
        with self._lock:
            self._catalog = None
            self._key = None
            self._expires_at = 0.0


GAME_CATALOG_CACHE = GameCatalogCache()


def get_game_catalog():
    return GAME_CATALOG_CACHE.get()


def invalidate_game_catalog():
    GAME_CATALOG_CACHE.invalidate()








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
        entry for entry in CATALOG_GAME_SYSTEMS
        if entry[0] in ENABLED_GAME_SYSTEMS
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


def storage_status():
    try:
        usage = shutil.disk_usage(ROM_ROOT)
    except OSError:
        return {
            "total_bytes": None,
            "used_bytes": None,
            "available_bytes": None,
            "used_percent": None,
        }

    return {
        "total_bytes": usage.total,
        "used_bytes": usage.used,
        "available_bytes": usage.free,
        "used_percent": round(usage.used * 100 / usage.total)
            if usage.total else None,
    }


def kiddos_status():
    version = run_read_only([str(KIDDOS_UI), "--version"]).strip()
    prefix = "KiddOS "
    if not version.startswith(prefix):
        return {"installed": False, "version": None}
    return {"installed": True, "version": version[len(prefix):]}


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
        "storage": storage_status(),
        "kiddos": kiddos_status(),
    }


class Handler(BaseHTTPRequestHandler):
    def send_bytes(self, status, content_type, body):
        self._response_status = status
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path
        tracked = path in (
            "/api/catalog",
            "/api/library",
            "/api/artwork",
            "/api/metadata/search",
        )
        if tracked:
            ACTIVITY.begin_request()
        self._response_status = 500
        try:
            self._dispatch_get()
        finally:
            if tracked:
                ACTIVITY.finish_request(self._response_status < 400)

    def _dispatch_get(self):
        path = urlparse(self.path).path

        if path == "/api/status":
            body = json.dumps(device_status(), indent=2).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return

        if path == "/api/library":
            body = json.dumps(library_status(), indent=2).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return

        if path == "/api/catalog":
            try:
                invalidate_game_catalog()
                data = get_game_catalog()
                body = json.dumps(data, indent=2).encode("utf-8")
            except (OSError, ValueError) as exc:
                message = json.dumps({"error": str(exc)}).encode("utf-8")
                self.send_bytes(500, "application/json; charset=utf-8", message)
                return
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return



        if path == "/api/artwork":
            self.handle_artwork(urlparse(self.path).query)
            return

        if path == "/api/metadata/search":
            self.handle_metadata_search(urlparse(self.path).query)
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

    def handle_metadata_search(self, query):
        try:
            params = parse_qs(query, strict_parsing=True)
            categories = params.get("category", [])
            queries = params.get("q", [])
            limits = params.get("limit", ["8"])
            if len(categories) != 1 or len(queries) != 1 or len(limits) != 1:
                raise ValueError("Provide one category and one search query")
            result = providers.search(categories[0], queries[0], limit=limits[0])
            body = json.dumps(result).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
        except ValueError as exc:
            self.send_api_error(400, exc)
        except providers.ProviderError as exc:
            self.send_api_error(502, exc)




    def handle_artwork(self, query):
        try:
            params = parse_qs(query, strict_parsing=True)
            game_ids = params.get("game_id", [])
            if len(game_ids) != 1 or not game_ids[0]:
                self.send_api_error(400, "A single game ID is required")
                return

            catalog = get_game_catalog()
            game = next(
                (item for item in catalog["games"]
                 if item["id"] == game_ids[0]),
                None,
            )
            if game is None or not game.get("artwork"):
                self.send_api_error(404, "Artwork not found")
                return

            artwork = Path(game["artwork"]).resolve(strict=True)
            root = Path(ROM_ROOT).resolve(strict=True)
            artwork.relative_to(root)

            content_types = {
                ".png": "image/png",
                ".jpg": "image/jpeg",
                ".webp": "image/webp",
            }
            content_type = content_types.get(artwork.suffix.lower())
            if not content_type or not artwork.is_file():
                self.send_api_error(404, "Artwork not found")
                return

            body = artwork.read_bytes()
            self.send_bytes(200, content_type, body)
        except (OSError, ValueError):
            self.send_api_error(404, "Artwork not found")

    def read_json(self):
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            raise ValueError("Invalid Content-Length")

        if length <= 0 or length > 65536:
            raise ValueError("Request body must be between 1 and 65536 bytes")

        try:
            data = json.loads(self.rfile.read(length))
        except (json.JSONDecodeError, UnicodeDecodeError):
            raise ValueError("Request body must be valid JSON")

        if not isinstance(data, dict):
            raise ValueError("Request body must be a JSON object")
        return data

    def send_catalog(self):
        data = get_game_catalog()
        body = json.dumps(data).encode("utf-8")
        self.send_bytes(200, "application/json; charset=utf-8", body)

    def send_api_error(self, status, message):
        body = json.dumps({"error": str(message)}).encode("utf-8")
        self.send_bytes(status, "application/json; charset=utf-8", body)

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path
        tracked = path in (
            "/api/activity",
            "/api/upload",
            "/api/artwork/upload",
            "/api/slots/assign",
            "/api/slots/unassign",
            "/api/games/delete",
        )
        if tracked:
            ACTIVITY.begin_request()
        self._response_status = 500
        try:
            self._dispatch_post(parsed, path)
        finally:
            if tracked:
                ACTIVITY.finish_request(self._response_status < 400)

    def _dispatch_post(self, parsed, path):
        if path == "/api/activity":
            body = json.dumps({"success": True}).encode("utf-8")
            self.send_bytes(200, "application/json; charset=utf-8", body)
            return

        if path == "/api/upload":
            self.handle_upload(parsed.query)
            return

        if path == "/api/artwork/upload":
            self.handle_artwork_upload(parsed.query)
            return

        if path in ("/api/slots/assign", "/api/slots/unassign",
                    "/api/games/delete"):
            try:
                data = self.read_json()

                if path == "/api/slots/assign":
                    result = assign_game(
                        ROM_ROOT, STATE_ROOT,
                        data.get("slot"), data.get("game_id"),
                        replace=data.get("replace") is True,
                    )
                elif path == "/api/slots/unassign":
                    result = unassign_game(
                        ROM_ROOT, STATE_ROOT, data.get("slot")
                    )
                else:
                    result = delete_game(
                        ROM_ROOT, STATE_ROOT, data.get("game_id")
                    )

                invalidate_game_catalog()
                body = json.dumps(result).encode("utf-8")
                self.send_bytes(200, "application/json; charset=utf-8", body)
            except ValueError as exc:
                self.send_api_error(400, exc)
            except OSError as exc:
                self.send_api_error(500, exc)
            return

        if path in ("/api/status", "/api/library", "/api/catalog"):
            self.send_bytes(405, "text/plain; charset=utf-8",
                            b"Method not allowed")
        else:
            self.send_bytes(404, "text/plain; charset=utf-8", b"Not found")

    def handle_upload(self, query):
        temporary = None
        try:
            params = parse_qs(query, strict_parsing=True)
            system_id = params.get("system", [""])[0]
            filename = params.get("filename", [""])[0]

            system = next(
                (entry for entry in CATALOG_GAME_SYSTEMS
                 if entry[0] == system_id
                 and entry[0] in ENABLED_GAME_SYSTEMS),
                None,
            )
            if system is None:
                raise ValueError("Unsupported game system")

            if (not filename or filename in (".", "..")
                    or "/" in filename or "\\" in filename
                    or filename.startswith(".")):
                raise ValueError("Invalid filename")

            extension = Path(filename).suffix.lower()
            if extension not in system[2]:
                allowed = ", ".join(sorted(system[2]))
                raise ValueError(f"Unsupported file type; allowed: {allowed}")

            try:
                length = int(self.headers.get("Content-Length", "0"))
            except ValueError:
                raise ValueError("Invalid Content-Length")

            max_upload = 512 * 1024 * 1024
            if length <= 0 or length > max_upload:
                raise ValueError("Upload must be between 1 byte and 512 MiB")

            destination_dir = ROM_ROOT / system[3][0]

            destination_dir.mkdir(parents=True, exist_ok=True)
            destination = destination_dir / filename

            if destination.exists():
                raise ValueError("A file with that name already exists")

            fd, temporary = tempfile.mkstemp(
                prefix=".kiddos-upload-", suffix=".tmp",
                dir=str(destination_dir),
            )
            received = 0
            with os.fdopen(fd, "wb") as output:
                while received < length:
                    chunk = self.rfile.read(min(1024 * 1024, length - received))
                    if not chunk:
                        raise ValueError("Upload ended before all bytes arrived")
                    output.write(chunk)
                    received += len(chunk)
                output.flush()
                os.fsync(output.fileno())

            if destination.exists():
                raise ValueError("A file with that name already exists")
            os.replace(temporary, destination)
            temporary = None
            invalidate_game_catalog()

            body = json.dumps({
                "success": True,
                "filename": filename,
                "system": system[1],
                "size_bytes": received,
                "catalog": get_game_catalog(),
            }).encode("utf-8")
            self.send_bytes(201, "application/json; charset=utf-8", body)

        except (ValueError, OSError) as exc:
            self.send_api_error(400 if isinstance(exc, ValueError) else 500, exc)
        finally:
            if temporary:
                try:
                    os.unlink(temporary)
                except OSError:
                    pass

    def handle_artwork_upload(self, query):
        temporary = None
        try:
            params = parse_qs(query, strict_parsing=True)
            game_id = params.get("game_id", [""])[0]
            filename = params.get("filename", [""])[0]

            if not game_id:
                raise ValueError("Missing game ID")
            if (not filename or "/" in filename or "\\" in filename
                    or filename.startswith(".")):
                raise ValueError("Invalid artwork filename")

            extension = Path(filename).suffix.lower()
            if extension not in (".png", ".jpg", ".webp"):
                raise ValueError("Artwork must be PNG, JPG, or WebP")

            game = next(
                (item for item in discover_games(ROM_ROOT)
                 if item["id"] == game_id),
                None,
            )
            if game is None:
                raise ValueError("Game not found")

            rom_path = Path(game["path"])
            destination = rom_path.with_suffix(extension)

            try:
                length = int(self.headers.get("Content-Length", "0"))
            except ValueError:
                raise ValueError("Invalid Content-Length")
            if length <= 0 or length > MAX_ARTWORK_BYTES:
                raise ValueError("Artwork must be between 1 byte and 20 MiB")

            fd, temporary = tempfile.mkstemp(
                prefix=".kiddos-artwork-", suffix=".tmp",
                dir=str(rom_path.parent),
            )
            received = 0
            with os.fdopen(fd, "wb") as output:
                while received < length:
                    chunk = self.rfile.read(min(1024 * 1024, length - received))
                    if not chunk:
                        raise ValueError("Upload ended before all bytes arrived")
                    output.write(chunk)
                    received += len(chunk)
                output.flush()
                os.fsync(output.fileno())

            with open(temporary, "rb") as image:
                header = image.read(12)
            valid = (
                (extension == ".png" and header.startswith(b"\x89PNG\r\n\x1a\n"))
                or (extension == ".jpg" and header.startswith(b"\xff\xd8\xff"))
                or (extension == ".webp" and
                    header.startswith(b"RIFF") and header[8:12] == b"WEBP")
            )
            if not valid:
                raise ValueError("File contents do not match the image type")

            os.replace(temporary, destination)
            temporary = None
            invalidate_game_catalog()

            body = json.dumps({
                "success": True,
                "filename": destination.name,
                "catalog": get_game_catalog(),
            }).encode("utf-8")
            self.send_bytes(201, "application/json; charset=utf-8", body)

        except (ValueError, OSError) as exc:
            self.send_api_error(400 if isinstance(exc, ValueError) else 500, exc)
        finally:
            if temporary:
                try:
                    os.unlink(temporary)
                except OSError:
                    pass

    def log_message(self, fmt, *args):
        print("%s - %s" % (self.address_string(), fmt % args))


def idle_shutdown_watchdog(httpd, stop_event):
    while not stop_event.wait(1):
        if ACTIVITY.idle_expired():
            print("Remote Management idle for 15 minutes; shutting down.")
            httpd.shutdown()
            return


def main():
    global ROM_ROOT, STATE_ROOT
    parser = argparse.ArgumentParser(description="Retro KiddOS management server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8081)
    parser.add_argument("--rom-root", default=str(ROM_ROOT))
    parser.add_argument("--state-root", default=str(STATE_ROOT))
    args = parser.parse_args()
    ROM_ROOT = Path(args.rom_root)
    STATE_ROOT = Path(args.state_root)

    try:
        server = ThreadingHTTPServer((args.host, args.port), Handler)
    except OSError as exc:
        print(f"Could not start management server: {exc}")
        return

    print(f"Retro KiddOS Management {VERSION}")
    global ACTIVITY
    ACTIVITY = ManagementActivity()
    stop_watchdog = threading.Event()
    watchdog = threading.Thread(
        target=idle_shutdown_watchdog,
        args=(server, stop_watchdog),
        daemon=True,
    )

    print(f"Listening on http://{args.host}:{args.port}/")
    watchdog.start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping management server.")
    finally:
        stop_watchdog.set()
        watchdog.join(timeout=2)
        server.server_close()


if __name__ == "__main__":
    main()
