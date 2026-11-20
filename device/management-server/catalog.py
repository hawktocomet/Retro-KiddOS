# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

"""Persistent four-slot KiddOS game catalog."""

import hashlib
import json
import os
import tempfile
from pathlib import Path

GAME_SYSTEMS = (
    ("gb", "Game Boy", (".gb", ".zip"), ("gb",)),
    ("gbc", "Game Boy Color", (".gbc", ".zip"), ("gbc",)),
    ("gba", "Game Boy Advance", (".gba", ".zip"), ("gba",)),
    ("arcade", "Arcade", (".zip", ".7z"), ("arcade",)),
    ("snes", "Super Nintendo", (".sfc", ".smc", ".zip"), ("snes",)),
    ("genesis-megadrive", "Genesis / Mega Drive",
     (".gen", ".md", ".bin", ".smd", ".zip"), ("genesis", "megadrive")),
    ("mastersystem", "Master System", (".sms", ".zip"), ("mastersystem",)),
    ("nes", "Nintendo Entertainment System", (".nes", ".zip"), ("nes",)),
)

LEGACY_DIRECTORY = ".kiddos-ui-test-20261116"
INITIAL_SLOT_TITLES = (
    "Mario Tennis",
    "Pokemon Red",
    "Super Mario Bros. Deluxe",
    "Tetris",
)


def _game_id(relative_path):
    return hashlib.sha256(relative_path.encode("utf-8")).hexdigest()[:16]


def discover_games(rom_root):
    """Discover installed games and mark formats supported by KiddOS."""
    root = Path(rom_root)
    groups = [
        (system_id, display_name, extensions, folders)
        for system_id, display_name, extensions, folders in GAME_SYSTEMS
    ]
    groups.append((
        "legacy", "KiddOS Games", (".gb", ".gbc"),
        (LEGACY_DIRECTORY,),
    ))

    games = []

    for system_id, display_name, extensions, folders in groups:
        for folder in folders:
            directory = root / folder
            if not directory.is_dir():
                continue

            try:
                files = sorted(
                    directory.iterdir(),
                    key=lambda item: item.name.casefold(),
                )
            except OSError:
                continue

            artwork_by_stem = {}
            artwork_priority = {".png": 0, ".jpg": 1, ".webp": 2}
            artwork_files = sorted(
                files,
                key=lambda item: (
                    artwork_priority.get(item.suffix.lower(), 99),
                    item.name.casefold(),
                ),
            )
            for candidate in artwork_files:
                if (candidate.name.startswith(".")
                        or candidate.suffix.lower() not in
                        (".png", ".jpg", ".webp")
                        or not candidate.is_file()):
                    continue
                try:
                    candidate.resolve().relative_to(root.resolve())
                    artwork_by_stem.setdefault(
                        candidate.stem.casefold(), str(candidate)
                    )
                except (OSError, ValueError):
                    continue

            for path in files:
                if path.name.startswith(".") or not path.is_file():
                    continue
                if path.suffix.lower() not in extensions:
                    continue

                actual_system = system_id
                if system_id == "legacy":
                    actual_system = (
                        "gbc" if path.suffix.lower() == ".gbc" else "gb"
                    )

                try:
                    resolved = path.resolve()
                    resolved.relative_to(root.resolve())
                    size = path.stat().st_size
                except (OSError, ValueError):
                    continue

                relative = resolved.relative_to(root.resolve()).as_posix()
                title = path.stem
                if title == "Pokemon Red Version":
                    title = "Pokemon Red"

                artwork = artwork_by_stem.get(path.stem.casefold())

                slottable = (
                    (actual_system == "gb" and path.suffix.lower() == ".gb")
                    or (actual_system == "gbc" and path.suffix.lower() == ".gbc")
                )

                games.append({
                    "id": _game_id(relative),
                    "title": title,
                    "system": display_name,
                    "system_id": actual_system,
                    "filename": path.name,
                    "path": str(path),
                    "artwork": artwork,
                    "needs_artwork": artwork is None,
                    "size_bytes": size,
                    "slottable": slottable,
                })

    return games


def _write_slots(state_root, assignments):
    state_root = Path(state_root)
    state_root.mkdir(parents=True, exist_ok=True)
    destination = state_root / "slots.json"

    fd, temporary = tempfile.mkstemp(
        prefix=".slots-", suffix=".tmp", dir=str(state_root)
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump({"version": 1, "slots": assignments},
                      stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, destination)
    except Exception:
        try:
            os.unlink(temporary)
        except OSError:
            pass
        raise


def _load_or_initialize_slots(state_root, games):
    state_root = Path(state_root)
    state_root.mkdir(parents=True, exist_ok=True)
    slot_file = state_root / "slots.json"

    if slot_file.exists():
        try:
            data = json.loads(slot_file.read_text(encoding="utf-8"))
            slots = data["slots"]
            if data.get("version") != 1 or not isinstance(slots, dict):
                raise ValueError("Unsupported slot configuration")
            return {
                str(number): slots.get(str(number))
                for number in range(1, 5)
            }
        except (OSError, ValueError, KeyError, TypeError):
            raise ValueError(f"Invalid slot configuration: {slot_file}")

    by_title = {
        game["title"]: game["id"]
        for game in games if game["slottable"]
    }
    assignments = {
        str(number): by_title.get(title)
        for number, title in enumerate(INITIAL_SLOT_TITLES, start=1)
    }
    _write_slots(state_root, assignments)
    return assignments



def _save_assignments(state_root, assignments):
    """Persist validated slot assignments."""
    _write_slots(state_root, assignments)


def assign_game(rom_root, state_root, slot_number, game_id,
                replace=False):
    """Assign a discovered, launchable game to one of the four slots."""
    try:
        slot_number = int(slot_number)
    except (TypeError, ValueError):
        raise ValueError("Slot number must be between 1 and 4")

    if slot_number not in range(1, 5):
        raise ValueError("Slot number must be between 1 and 4")

    games = discover_games(rom_root)
    game = next((item for item in games if item["id"] == game_id), None)
    if game is None or not game["slottable"]:
        raise ValueError("Game is not available for a KiddOS slot")
    if not game["artwork"]:
        raise ValueError(
            "Game needs artwork before it can be assigned. "
            "Upload artwork or find matching cover art first."
        )

    assignments = _load_or_initialize_slots(state_root, games)
    key = str(slot_number)
    current = assignments[key]

    if current and current != game_id and not replace:
        raise ValueError("Slot is occupied; use replace to overwrite it")

    # A game can occupy only one slot. Assigning it elsewhere moves it.
    for number, assigned_id in assignments.items():
        if assigned_id == game_id:
            assignments[number] = None

    assignments[key] = game_id
    _save_assignments(state_root, assignments)
    return game_catalog(rom_root, state_root)


def unassign_game(rom_root, state_root, slot_number):
    """Clear a slot without deleting its game."""
    try:
        slot_number = int(slot_number)
    except (TypeError, ValueError):
        raise ValueError("Slot number must be between 1 and 4")

    if slot_number not in range(1, 5):
        raise ValueError("Slot number must be between 1 and 4")

    games = discover_games(rom_root)
    assignments = _load_or_initialize_slots(state_root, games)
    assignments[str(slot_number)] = None
    _save_assignments(state_root, assignments)
    return game_catalog(rom_root, state_root)


def delete_game(rom_root, state_root, game_id):
    """Permanently delete one discovered ROM and clear any slot using it."""
    root = Path(rom_root).resolve()
    games = discover_games(root)
    game = next((item for item in games if item["id"] == game_id), None)

    if game is None:
        raise ValueError("Game was not found in the library")

    target = Path(game["path"])
    try:
        resolved = target.resolve(strict=True)
        resolved.relative_to(root)
    except (OSError, ValueError):
        raise ValueError("Game path is outside the ROM library")

    if not resolved.is_file():
        raise ValueError("Game file no longer exists")

    assignments = _load_or_initialize_slots(state_root, games)
    for number, assigned_id in assignments.items():
        if assigned_id == game_id:
            assignments[number] = None

    # Persist the slot change before unlinking; never delete arbitrary paths.
    _save_assignments(state_root, assignments)
    resolved.unlink()

    # Remove any artwork sidecar belonging to the deleted ROM.
    try:
        artwork_files = list(resolved.parent.iterdir())
    except OSError:
        artwork_files = []

    for artwork in artwork_files:
        if (artwork.stem.casefold() != resolved.stem.casefold()
                or artwork.suffix.lower() not in (".png", ".jpg", ".webp")
                or not artwork.is_file()):
            continue
        try:
            artwork.resolve(strict=True).relative_to(root)
            artwork.unlink()
        except (OSError, ValueError):
            pass

    return game_catalog(root, state_root)

def game_catalog(rom_root, state_root):
    """Return four stable slots, eligible games, and full inventory."""
    games = discover_games(rom_root)
    assignments = _load_or_initialize_slots(state_root, games)
    by_id = {game["id"]: game for game in games}

    slots = []
    assigned_ids = set()

    for number in range(1, 5):
        game_id = assignments[str(number)]
        game = by_id.get(game_id) if game_id else None

        if game is not None and game["slottable"]:
            assigned_ids.add(game_id)
        else:
            game = None

        slots.append({"number": number, "game": game})

    available = [
        game for game in games
        if (game["slottable"] and game["artwork"]
            and game["id"] not in assigned_ids)
    ]

    return {
        "slots": slots,
        "available": available,
        "games": games,
        "summary": {
            "slot_count": 4,
            "assigned_count": len(assigned_ids),
            "available_count": len(available),
            "installed_count": len(games),
        },
    }
