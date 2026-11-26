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

# Systems exposed by the initial KiddOS release.
# GAME_SYSTEMS remains the complete catalog for future expansion.
ENABLED_GAME_SYSTEMS = frozenset(("gb", "gbc"))

LEGACY_DIRECTORY = ".kiddos-ui-test-20261116"
INITIAL_SLOT_TITLES = (
    "Mario Tennis",
    "Pokemon Red",
    "Super Mario Bros. Deluxe",
    "Tetris",
)


def _game_id(relative_path):
    return hashlib.sha256(relative_path.encode("utf-8")).hexdigest()[:16]


MOVIE_EXTENSIONS = frozenset((".mp4", ".mkv", ".avi", ".mov", ".m4v"))
MUSIC_EXTENSIONS = frozenset(
    (".mp3", ".m4a", ".aac", ".flac", ".ogg", ".opus", ".wav", ".wma")
)
TV_EPISODE_EXTENSIONS = frozenset(
    (".mp4", ".mkv", ".avi", ".mov", ".m4v", ".webm")
)


def _media_id(category, relative_path):
    """Return a stable ID scoped to a media category and relative path."""
    value = f"{category}:{relative_path}"
    return hashlib.sha256(value.encode("utf-8")).hexdigest()[:16]


def _safe_media_file(path, rom_root):
    """Resolve a regular file only when it remains inside the ROM root."""
    try:
        resolved = path.resolve(strict=True)
        resolved.relative_to(Path(rom_root).resolve())
        return resolved if resolved.is_file() else None
    except (OSError, ValueError):
        return None


def discover_movies(rom_root):
    """Discover movies with matching adjacent JPG or PNG artwork."""
    root = Path(rom_root)
    directory = root / "movies"
    if not directory.is_dir():
        return []

    try:
        paths = sorted(directory.iterdir(), key=lambda item: item.name.casefold())
    except OSError:
        return []

    movies = []
    for path in paths:
        if path.name.startswith(".") or path.suffix.lower() not in MOVIE_EXTENSIONS:
            continue

        resolved = _safe_media_file(path, root)
        if resolved is None:
            continue

        try:
            artwork_candidates = [
                item for item in directory.iterdir()
                if item.stem.casefold() == path.stem.casefold()
                and item.suffix.lower() in (".png", ".jpg")
            ]
        except OSError:
            continue

        artwork_candidates.sort(key=lambda item: (
            0 if item.suffix.lower() == ".png" else 1,
            item.name.casefold(),
        ))

        artwork = next(
            (
                candidate for candidate in artwork_candidates
                if _safe_media_file(candidate, root) is not None
            ),
            None,
        )
        if artwork is None:
            continue

        resolved_artwork = _safe_media_file(artwork, root)
        relative = resolved.relative_to(root.resolve()).as_posix()

        movies.append({
            "id": _media_id("movie", relative),
            "title": path.stem,
            "path": str(resolved),
            "artwork": str(resolved_artwork),
        })

    return movies


def discover_music_playlists(rom_root):
    """Discover artwork-backed playlist folders, not individual tracks."""
    root = Path(rom_root)
    base = root / "music"
    if not base.is_dir():
        return []

    try:
        directories = sorted(
            (item for item in base.iterdir() if item.is_dir()),
            key=lambda item: item.name.casefold(),
        )
    except OSError:
        return []

    playlists = []
    for directory in directories:
        if directory.name.startswith("."):
            continue

        try:
            resolved_directory = directory.resolve(strict=True)
            relative = resolved_directory.relative_to(root.resolve()).as_posix()
        except (OSError, ValueError):
            continue

        artwork = next(
            (
                directory / name
                for name in ("cover.jpg", "cover.png")
                if (directory / name).is_file()
            ),
            None,
        )
        if artwork is None:
            continue

        resolved_artwork = _safe_media_file(artwork, root)
        if resolved_artwork is None:
            continue

        tracks = []
        try:
            candidates = sorted(
                directory.rglob("*"),
                key=lambda item: item.as_posix().casefold(),
            )
        except OSError:
            continue

        for candidate in candidates:
            if (
                candidate.name.startswith(".")
                or candidate.suffix.lower() not in MUSIC_EXTENSIONS
            ):
                continue
            resolved_track = _safe_media_file(candidate, root)
            if resolved_track is not None:
                tracks.append(str(resolved_track))

        if not tracks:
            continue

        playlists.append({
            "id": _media_id("music-playlist", relative),
            "title": directory.name,
            "path": str(resolved_directory),
            "artwork": str(resolved_artwork),
            "tracks": tracks,
            "track_count": len(tracks),
        })

    return playlists


def discover_tv_shows(rom_root):
    """Discover artwork-backed TV show folders and their episodes."""
    root = Path(rom_root)
    base = root / "tvshows"
    if not base.is_dir():
        return []

    try:
        directories = sorted(
            (item for item in base.iterdir() if item.is_dir()),
            key=lambda item: item.name.casefold(),
        )
    except OSError:
        return []

    shows = []
    for directory in directories:
        if directory.name.startswith("."):
            continue

        try:
            resolved_directory = directory.resolve(strict=True)
            relative = resolved_directory.relative_to(root.resolve()).as_posix()
        except (OSError, ValueError):
            continue

        artwork = next(
            (
                directory / name
                for name in ("poster.jpg", "poster.png", "cover.jpg", "cover.png")
                if (directory / name).is_file()
                and _safe_media_file(directory / name, root) is not None
            ),
            None,
        )
        if artwork is None:
            continue

        episodes = []
        try:
            candidates = sorted(
                directory.rglob("*"),
                key=lambda item: item.as_posix().casefold(),
            )
        except OSError:
            continue

        for candidate in candidates:
            if (
                candidate.name.startswith(".")
                or candidate.suffix.lower() not in TV_EPISODE_EXTENSIONS
            ):
                continue

            resolved_episode = _safe_media_file(candidate, root)
            if resolved_episode is not None:
                episodes.append(str(resolved_episode))

        if not episodes:
            continue

        shows.append({
            "id": _media_id("tv-show", relative),
            "title": directory.name,
            "path": str(resolved_directory),
            "artwork": str(_safe_media_file(artwork, root)),
            "episodes": episodes,
            "episode_count": len(episodes),
        })

    return shows



MEDIA_CATEGORIES = {
    "movies": discover_movies,
    "music_playlists": discover_music_playlists,
    "tv_shows": discover_tv_shows,
}


def _write_media_slots(state_root, categories):
    """Atomically persist independent four-slot media assignments."""
    state_root = Path(state_root)
    state_root.mkdir(parents=True, exist_ok=True)
    destination = state_root / "media-slots.json"

    fd, temporary = tempfile.mkstemp(
        prefix=".media-slots-", suffix=".tmp", dir=str(state_root)
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(
                {"version": 1, "categories": categories},
                stream,
                indent=2,
            )
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


def _load_or_initialize_media_slots(state_root):
    """Load media assignments or initialize all twelve slots empty."""
    state_root = Path(state_root)
    state_root.mkdir(parents=True, exist_ok=True)
    slot_file = state_root / "media-slots.json"

    if slot_file.exists():
        try:
            data = json.loads(slot_file.read_text(encoding="utf-8"))
            categories = data["categories"]
            if data.get("version") != 1 or not isinstance(categories, dict):
                raise ValueError("Unsupported media slot configuration")

            assignments = {}
            for category in MEDIA_CATEGORIES:
                slots = categories.get(category)
                if not isinstance(slots, dict):
                    raise ValueError("Invalid media category slots")
                assignments[category] = {
                    str(number): slots.get(str(number))
                    for number in range(1, 5)
                }
            return assignments
        except (OSError, ValueError, KeyError, TypeError):
            raise ValueError(f"Invalid media slot configuration: {slot_file}")

    assignments = {
        category: {str(number): None for number in range(1, 5)}
        for category in MEDIA_CATEGORIES
    }
    _write_media_slots(state_root, assignments)
    return assignments


def media_slots_catalog(rom_root, state_root):
    """Return four assignments and artwork-eligible inventory per category."""
    assignments = _load_or_initialize_media_slots(state_root)
    result = {}

    for category, discover in MEDIA_CATEGORIES.items():
        items = discover(rom_root)
        by_id = {item["id"]: item for item in items}
        assigned_ids = set()
        slots = []

        for number in range(1, 5):
            item_id = assignments[category][str(number)]
            item = by_id.get(item_id) if item_id else None
            if item is not None:
                assigned_ids.add(item_id)
            slots.append({"number": number, "item": item})

        available = [
            item for item in items if item["id"] not in assigned_ids
        ]
        result[category] = {
            "slots": slots,
            "available": available,
            "summary": {
                "slot_count": 4,
                "assigned_count": len(assigned_ids),
                "available_count": len(available),
                "installed_count": len(items),
            },
        }

    return result


def assign_media(rom_root, state_root, category, slot_number, item_id,
                 replace=False):
    """Assign an artwork-backed media item to one category's four slots."""
    if not isinstance(category, str) or category not in MEDIA_CATEGORIES:
        raise ValueError("Unknown media category")

    try:
        slot_number = int(slot_number)
    except (TypeError, ValueError):
        raise ValueError("Slot number must be between 1 and 4")
    if slot_number not in range(1, 5):
        raise ValueError("Slot number must be between 1 and 4")

    items = MEDIA_CATEGORIES[category](rom_root)
    if not isinstance(item_id, str):
        raise ValueError("Media item is not available")
    item = next((entry for entry in items if entry["id"] == item_id), None)
    if item is None:
        raise ValueError("Media item is not available")

    assignments = _load_or_initialize_media_slots(state_root)
    category_slots = assignments[category]
    key = str(slot_number)
    current = category_slots[key]

    if current and current != item_id and not replace:
        raise ValueError("Slot is occupied; use replace to overwrite it")

    # An item can occupy only one slot within its own category.
    for number, assigned_id in category_slots.items():
        if assigned_id == item_id:
            category_slots[number] = None

    category_slots[key] = item_id
    _write_media_slots(state_root, assignments)
    return media_slots_catalog(rom_root, state_root)


def unassign_media(rom_root, state_root, category, slot_number):
    """Clear a media slot without deleting its underlying files."""
    if not isinstance(category, str) or category not in MEDIA_CATEGORIES:
        raise ValueError("Unknown media category")

    try:
        slot_number = int(slot_number)
    except (TypeError, ValueError):
        raise ValueError("Slot number must be between 1 and 4")
    if slot_number not in range(1, 5):
        raise ValueError("Slot number must be between 1 and 4")

    assignments = _load_or_initialize_media_slots(state_root)
    assignments[category][str(slot_number)] = None
    _write_media_slots(state_root, assignments)
    return media_slots_catalog(rom_root, state_root)


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
                    artwork is not None
                    and (
                        (actual_system == "gb" and path.suffix.lower() == ".gb")
                        or (actual_system == "gbc" and path.suffix.lower() == ".gbc")
                    )
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
    """Return four stable slots and the enabled-system game inventory."""
    games = [
        game for game in discover_games(rom_root)
        if game["system_id"] in ENABLED_GAME_SYSTEMS
    ]
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
        if game["slottable"] and game["id"] not in assigned_ids
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
