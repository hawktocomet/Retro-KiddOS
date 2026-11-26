# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from catalog import (
    assign_game,
    delete_game,
    discover_games,
    discover_movies,
    discover_music_playlists,
    discover_tv_shows,
    game_catalog,
    media_slots_catalog,
    assign_media,
    unassign_media,
    unassign_game,
)


class MediaDiscoveryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.roms = self.root / "roms"
        self.roms.mkdir()

    def tearDown(self):
        self.temporary.cleanup()

    def test_movies_require_matching_artwork(self):
        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Family Movie.mp4").write_bytes(b"movie")
        (movies / "Family Movie.jpg").write_bytes(b"jpg")
        (movies / "No Artwork.mkv").write_bytes(b"movie")
        (movies / "Unsupported.txt").write_text("ignore")

        discovered = discover_movies(self.roms)

        self.assertEqual([item["title"] for item in discovered], ["Family Movie"])
        self.assertTrue(discovered[0]["artwork"].endswith("Family Movie.jpg"))
        self.assertEqual(discovered[0]["path"], str((movies / "Family Movie.mp4").resolve()))

    def test_movies_prefer_png_artwork_and_have_stable_ids(self):
        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Film.mkv").write_bytes(b"movie")
        (movies / "Film.jpg").write_bytes(b"jpg")
        (movies / "Film.png").write_bytes(b"png")

        first = discover_movies(self.roms)
        second = discover_movies(self.roms)

        self.assertEqual(first[0]["artwork"], str((movies / "Film.png").resolve()))
        self.assertEqual(first[0]["id"], second[0]["id"])

    def test_music_discovers_artwork_backed_playlist_folders(self):
        music = self.roms / "music"
        playlist = music / "Favorites"
        playlist.mkdir(parents=True)
        (playlist / "cover.jpg").write_bytes(b"cover")
        (playlist / "Track One.mp3").write_bytes(b"track")
        (playlist / "nested").mkdir()
        (playlist / "nested" / "Track Two.flac").write_bytes(b"track")
        (music / "Loose Track.mp3").write_bytes(b"track")

        discovered = discover_music_playlists(self.roms)

        self.assertEqual(len(discovered), 1)
        self.assertEqual(discovered[0]["title"], "Favorites")
        self.assertEqual(discovered[0]["track_count"], 2)
        self.assertEqual(len(discovered[0]["tracks"]), 2)

    def test_music_ignores_playlists_without_artwork_or_tracks(self):
        music = self.roms / "music"
        no_art = music / "No Artwork"
        no_tracks = music / "No Tracks"
        no_art.mkdir(parents=True)
        no_tracks.mkdir()
        (no_art / "Track.mp3").write_bytes(b"track")
        (no_tracks / "cover.png").write_bytes(b"cover")

        self.assertEqual(discover_music_playlists(self.roms), [])

    def test_tv_shows_group_episodes_and_require_artwork(self):
        shows = self.roms / "tvshows"
        show = shows / "Example Show"
        (show / "Season 1").mkdir(parents=True)
        (show / "poster.png").write_bytes(b"poster")
        (show / "Season 1" / "Episode 1.mp4").write_bytes(b"episode")
        (show / "Season 1" / "Episode 2.mkv").write_bytes(b"episode")

        no_art = shows / "No Artwork"
        no_art.mkdir()
        (no_art / "Episode.mp4").write_bytes(b"episode")

        no_episodes = shows / "No Episodes"
        no_episodes.mkdir()
        (no_episodes / "cover.jpg").write_bytes(b"cover")

        discovered = discover_tv_shows(self.roms)

        self.assertEqual(len(discovered), 1)
        self.assertEqual(discovered[0]["title"], "Example Show")
        self.assertEqual(discovered[0]["episode_count"], 2)
        self.assertEqual(len(discovered[0]["episodes"]), 2)

    def test_media_discovery_ignores_symlinks_outside_rom_root(self):
        outside = self.root / "outside.mp4"
        outside.write_bytes(b"outside")

        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Outside.mp4").symlink_to(outside)
        (movies / "Outside.jpg").write_bytes(b"artwork")

        self.assertEqual(discover_movies(self.roms), [])



class MediaSlotCatalogTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.roms = self.root / "roms"
        self.state = self.root / "state"
        self.roms.mkdir()

        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Film One.mp4").write_bytes(b"movie")
        (movies / "Film One.jpg").write_bytes(b"art")
        (movies / "Film Two.mp4").write_bytes(b"movie")
        (movies / "Film Two.jpg").write_bytes(b"art")

        playlist = self.roms / "music" / "Favorites"
        playlist.mkdir(parents=True)
        (playlist / "cover.jpg").write_bytes(b"art")
        (playlist / "Track.mp3").write_bytes(b"audio")

        show = self.roms / "tvshows" / "Bluey"
        show.mkdir(parents=True)
        (show / "poster.png").write_bytes(b"art")
        (show / "Episode.mp4").write_bytes(b"episode")

    def tearDown(self):
        self.temporary.cleanup()

    def test_initializes_four_empty_slots_for_each_media_category(self):
        result = media_slots_catalog(self.roms, self.state)

        self.assertEqual(
            set(result),
            {"movies", "music_playlists", "tv_shows"},
        )
        for category in result.values():
            self.assertEqual(len(category["slots"]), 4)
            self.assertTrue(
                all(slot["item"] is None for slot in category["slots"])
            )
            self.assertEqual(category["summary"]["assigned_count"], 0)

        persisted = json.loads(
            (self.state / "media-slots.json").read_text()
        )
        self.assertEqual(persisted["version"], 1)

    def test_assign_move_replace_and_unassign_preserve_media(self):
        initial = media_slots_catalog(self.roms, self.state)
        film_one = initial["movies"]["available"][0]
        film_two = initial["movies"]["available"][1]

        result = assign_media(
            self.roms, self.state, "movies", 1, film_one["id"]
        )
        self.assertEqual(
            result["movies"]["slots"][0]["item"]["id"], film_one["id"]
        )

        with self.assertRaisesRegex(ValueError, "occupied"):
            assign_media(
                self.roms, self.state, "movies", 1, film_two["id"]
            )

        result = assign_media(
            self.roms, self.state, "movies", 1, film_two["id"],
            replace=True,
        )
        self.assertEqual(
            result["movies"]["slots"][0]["item"]["id"], film_two["id"]
        )
        self.assertIn(
            film_one["id"],
            {item["id"] for item in result["movies"]["available"]},
        )

        result = assign_media(
            self.roms, self.state, "movies", 2, film_two["id"]
        )
        self.assertIsNone(result["movies"]["slots"][0]["item"])
        self.assertEqual(
            result["movies"]["slots"][1]["item"]["id"], film_two["id"]
        )

        result = unassign_media(self.roms, self.state, "movies", 2)
        self.assertIsNone(result["movies"]["slots"][1]["item"])
        self.assertTrue(Path(film_two["path"]).is_file())

    def test_categories_have_independent_assignments(self):
        initial = media_slots_catalog(self.roms, self.state)
        movie = initial["movies"]["available"][0]
        playlist = initial["music_playlists"]["available"][0]
        show = initial["tv_shows"]["available"][0]

        for category, item in (
            ("movies", movie),
            ("music_playlists", playlist),
            ("tv_shows", show),
        ):
            result = assign_media(
                self.roms, self.state, category, 1, item["id"]
            )
            self.assertEqual(
                result[category]["slots"][0]["item"]["id"], item["id"]
            )

        result = media_slots_catalog(self.roms, self.state)
        for category in ("movies", "music_playlists", "tv_shows"):
            self.assertEqual(result[category]["summary"]["assigned_count"], 1)

    def test_invalid_category_and_unavailable_item_are_rejected(self):
        media_slots_catalog(self.roms, self.state)

        with self.assertRaises(ValueError):
            assign_media(self.roms, self.state, "games", 1, "anything")

        for invalid_category in ([], {}):
            with self.assertRaises(ValueError):
                assign_media(
                    self.roms, self.state, invalid_category, 1, "anything"
                )
            with self.assertRaises(ValueError):
                unassign_media(
                    self.roms, self.state, invalid_category, 1
                )

        with self.assertRaises(ValueError):
            assign_media(self.roms, self.state, "movies", 1, "not-an-item")

        with self.assertRaises(ValueError):
            unassign_media(self.roms, self.state, "unknown", 1)

        with self.assertRaises(ValueError):
            unassign_media(self.roms, self.state, "movies", 5)


class GameCatalogTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.roms = self.root / "roms"
        self.state = self.root / "state"
        (self.roms / ".kiddos-ui-test-20261116").mkdir(parents=True)

        legacy = self.roms / ".kiddos-ui-test-20261116"
        for name in (
            "Mario Tennis.gbc",
            "Pokemon Red Version.gb",
            "Super Mario Bros. Deluxe.gbc",
            "Tetris.gb",
        ):
            (legacy / name).write_bytes(b"test-rom")

        for artwork_name in (
            "Mario Tennis.png",
            "Pokemon Red Version.png",
            "Super Mario Bros. Deluxe.png",
            "Tetris.png",
        ):
            (legacy / artwork_name).write_bytes(b"artwork")
        (self.roms / "gb").mkdir()
        (self.roms / "gb" / "Donkey Kong.gb").write_bytes(b"another-rom")
        (self.roms / "gb" / "Donkey Kong.png").write_bytes(b"artwork")

    def tearDown(self):
        self.temporary.cleanup()

    def test_discovers_full_library_but_limits_slot_eligibility(self):
        (self.roms / "gb" / "Ignore.gba").write_bytes(b"not-supported")
        (self.roms / "gba").mkdir()
        (self.roms / "gba" / "Advance Game.gba").write_bytes(b"gba")
        games = discover_games(self.roms)
        self.assertEqual(
            {game["title"] for game in games},
            {
                "Mario Tennis",
                "Pokemon Red",
                "Super Mario Bros. Deluxe",
                "Tetris",
                "Donkey Kong",
                "Advance Game",
            },
        )
        by_title = {game["title"]: game for game in games}
        self.assertTrue(by_title["Donkey Kong"]["slottable"])
        self.assertFalse(by_title["Advance Game"]["slottable"])

    def test_initializes_existing_four_games_in_original_order(self):
        catalog = game_catalog(self.roms, self.state)
        self.assertEqual(
            [slot["game"]["title"] for slot in catalog["slots"]],
            [
                "Mario Tennis",
                "Pokemon Red",
                "Super Mario Bros. Deluxe",
                "Tetris",
            ],
        )
        self.assertEqual(
            [slot["number"] for slot in catalog["slots"]],
            [1, 2, 3, 4],
        )
        self.assertEqual(
            [game["title"] for game in catalog["available"]],
            ["Donkey Kong"],
        )

    def test_assignments_persist_across_catalog_refreshes(self):
        first = game_catalog(self.roms, self.state)
        first_game_id = first["slots"][0]["game"]["id"]

        slots_file = self.state / "slots.json"
        data = json.loads(slots_file.read_text())
        data["slots"]["1"] = None
        slots_file.write_text(json.dumps(data))

        second = game_catalog(self.roms, self.state)
        self.assertIsNone(second["slots"][0]["game"])
        self.assertIn(first_game_id, {
            game["id"] for game in second["available"]
        })

    def test_missing_rom_does_not_get_reassigned(self):
        first = game_catalog(self.roms, self.state)
        original_id = first["slots"][0]["game"]["id"]
        (self.roms / ".kiddos-ui-test-20261116" /
         "Mario Tennis.gbc").unlink()

        second = game_catalog(self.roms, self.state)
        self.assertIsNone(second["slots"][0]["game"])
        self.assertNotEqual(
            second["slots"][0]["game"],
            second["slots"][1]["game"],
        )
        self.assertNotIn(original_id, {
            game["id"] for game in second["available"]
        })

    def test_artwork_is_discovered_without_moving_files(self):
        catalog = game_catalog(self.roms, self.state)
        mario = catalog["slots"][0]["game"]
        self.assertTrue(mario["artwork"].endswith("Mario Tennis.png"))
        self.assertTrue(Path(mario["path"]).is_file())
        self.assertTrue(Path(mario["artwork"]).is_file())

    def test_uppercase_artwork_extension_is_discovered(self):
        rom = self.roms / "gb" / "Uppercase Art.gb"
        artwork = rom.with_suffix(".PNG")
        rom.write_bytes(b"rom")
        artwork.write_bytes(b"artwork")

        game = next(
            item for item in discover_games(self.roms)
            if item["filename"] == rom.name
        )
        self.assertEqual(Path(game["artwork"]), artwork)

    def test_artwork_format_preference_is_png_then_jpg_then_webp(self):
        rom = self.roms / "gb" / "Format Preference.gb"
        rom.write_bytes(b"rom")
        png = rom.with_suffix(".png")
        rom.with_suffix(".jpg").write_bytes(b"jpg")
        rom.with_suffix(".webp").write_bytes(b"webp")
        png.write_bytes(b"png")

        game = next(
            item for item in discover_games(self.roms)
            if item["filename"] == rom.name
        )
        self.assertEqual(Path(game["artwork"]), png)

    def test_games_without_artwork_cannot_be_assigned(self):
        missing = self.roms / "gb" / "Missing Art.gb"
        missing.write_bytes(b"rom")

        catalog = game_catalog(self.roms, self.state)
        game = next(item for item in catalog["games"]
                    if item["filename"] == "Missing Art.gb")

        self.assertIsNone(game["artwork"])
        self.assertFalse(game["slottable"])
        self.assertNotIn(game["id"], {
            item["id"] for item in catalog["available"]
        })

        with self.assertRaisesRegex(ValueError, "not available"):
            assign_game(
                self.roms, self.state, 1, game["id"], replace=True
            )

    def test_corrupt_slot_configuration_fails_safely(self):
        self.state.mkdir()
        (self.state / "slots.json").write_text("{broken")
        with self.assertRaises(ValueError):
            game_catalog(self.roms, self.state)



    def test_assign_game_to_empty_slot_and_move_existing_assignment(self):
        initial = game_catalog(self.roms, self.state)
        donkey_kong = next(
            game for game in initial["available"]
            if game["title"] == "Donkey Kong"
        )

        result = unassign_game(self.roms, self.state, 1)
        self.assertIsNone(result["slots"][0]["game"])

        result = assign_game(
            self.roms, self.state, 1, donkey_kong["id"]
        )
        self.assertEqual(result["slots"][0]["game"]["title"], "Donkey Kong")
        self.assertTrue(Path(donkey_kong["path"]).is_file())

        result = assign_game(
            self.roms, self.state, 2, donkey_kong["id"], replace=True
        )
        self.assertIsNone(result["slots"][0]["game"])
        self.assertEqual(result["slots"][1]["game"]["title"], "Donkey Kong")
        self.assertIn(
            "Mario Tennis", {game["title"] for game in result["available"]}
        )

    def test_occupied_slot_requires_explicit_replace(self):
        initial = game_catalog(self.roms, self.state)
        donkey_kong = next(
            game for game in initial["available"]
            if game["title"] == "Donkey Kong"
        )
        original = initial["slots"][0]["game"]["id"]

        with self.assertRaisesRegex(ValueError, "occupied"):
            assign_game(self.roms, self.state, 1, donkey_kong["id"])

        unchanged = game_catalog(self.roms, self.state)
        self.assertEqual(unchanged["slots"][0]["game"]["id"], original)

        replaced = assign_game(
            self.roms, self.state, 1, donkey_kong["id"], replace=True
        )
        self.assertEqual(replaced["slots"][0]["game"]["title"], "Donkey Kong")
        self.assertIn(original, {
            game["id"] for game in replaced["available"]
        })

    def test_unassigning_slot_keeps_game_in_library(self):
        initial = game_catalog(self.roms, self.state)
        original = initial["slots"][0]["game"]
        result = unassign_game(self.roms, self.state, 1)

        self.assertIsNone(result["slots"][0]["game"])
        self.assertIn(
            original["id"], {game["id"] for game in result["available"]}
        )
        self.assertTrue(Path(original["path"]).is_file())

    def test_delete_game_removes_rom_and_clears_its_slot(self):
        initial = game_catalog(self.roms, self.state)
        original = initial["slots"][0]["game"]
        rom_path = Path(original["path"])
        artwork_path = rom_path.with_suffix(".png")
        uppercase_artwork_path = rom_path.with_suffix(".JPG")
        artwork_path.write_bytes(b"png")
        uppercase_artwork_path.write_bytes(b"jpg")

        result = delete_game(self.roms, self.state, original["id"])

        self.assertFalse(rom_path.exists())
        self.assertFalse(artwork_path.exists())
        self.assertFalse(uppercase_artwork_path.exists())
        self.assertIsNone(result["slots"][0]["game"])
        self.assertNotIn(
            original["id"], {game["id"] for game in result["games"]}
        )

    def test_invalid_slot_and_non_slottable_game_are_rejected(self):
        game_catalog(self.roms, self.state)
        gba_dir = self.roms / "gba"
        gba_dir.mkdir()
        gba_path = gba_dir / "Advance Game.gba"
        gba_path.write_bytes(b"gba")
        gba = next(
            game for game in discover_games(self.roms)
            if game["title"] == "Advance Game"
        )

        with self.assertRaises(ValueError):
            unassign_game(self.roms, self.state, 5)

        with self.assertRaises(ValueError):
            assign_game(self.roms, self.state, 1, gba["id"])

        self.assertTrue(gba_path.is_file())


if __name__ == "__main__":
    unittest.main(verbosity=2)
