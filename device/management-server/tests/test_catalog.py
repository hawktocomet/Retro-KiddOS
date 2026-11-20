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
    game_catalog,
    unassign_game,
)


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

        (legacy / "Mario Tennis.png").write_bytes(b"artwork")
        (self.roms / "gb").mkdir()
        (self.roms / "gb" / "Donkey Kong.gb").write_bytes(b"another-rom")

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

        result = delete_game(self.roms, self.state, original["id"])

        self.assertFalse(rom_path.exists())
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
