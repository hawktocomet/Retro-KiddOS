# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import json
import sys
import tempfile
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

SERVER_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SERVER_DIR))

import server


class ManagementApiTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = Path(self.temporary.name)
        self.roms = root / "roms"
        self.state = root / "state"
        legacy = self.roms / ".kiddos-ui-test-20261116"
        legacy.mkdir(parents=True)

        for name in (
            "Mario Tennis.gbc",
            "Pokemon Red Version.gb",
            "Super Mario Bros. Deluxe.gbc",
            "Tetris.gb",
        ):
            (legacy / name).write_bytes(b"test-rom")

        server.ROM_ROOT = self.roms
        server.STATE_ROOT = self.state

        self.httpd = ThreadingHTTPServer(("127.0.0.1", 0), server.Handler)
        self.thread = threading.Thread(
            target=self.httpd.serve_forever, daemon=True
        )
        self.thread.start()
        self.base_url = f"http://127.0.0.1:{self.httpd.server_port}"

    def tearDown(self):
        self.httpd.shutdown()
        self.httpd.server_close()
        self.thread.join(timeout=2)
        self.temporary.cleanup()

    def test_catalog_endpoint_returns_four_initial_slots(self):
        with urlopen(self.base_url + "/api/catalog") as response:
            self.assertEqual(response.status, 200)
            data = json.loads(response.read())

        self.assertEqual(
            [slot["game"]["title"] for slot in data["slots"]],
            [
                "Mario Tennis",
                "Pokemon Red",
                "Super Mario Bros. Deluxe",
                "Tetris",
            ],
        )
        self.assertEqual(data["summary"]["assigned_count"], 4)
        self.assertEqual(data["summary"]["installed_count"], 4)

    def test_catalog_endpoint_persists_assignments(self):
        urlopen(self.base_url + "/api/catalog").close()
        slots_file = self.state / "slots.json"
        data = json.loads(slots_file.read_text())
        data["slots"]["2"] = None
        slots_file.write_text(json.dumps(data))

        with urlopen(self.base_url + "/api/catalog") as response:
            result = json.loads(response.read())

        self.assertIsNone(result["slots"][1]["game"])
        self.assertEqual(result["summary"]["assigned_count"], 3)

    def test_existing_library_endpoint_remains_available(self):
        with urlopen(self.base_url + "/api/library") as response:
            self.assertEqual(response.status, 200)
            data = json.loads(response.read())

        self.assertIn("games", data)
        self.assertIn("systems", data)
        self.assertIn("movies", data)
        self.assertIn("summary", data)

    def test_post_remains_disabled(self):
        request = Request(
            self.base_url + "/api/catalog",
            data=b"{}",
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with self.assertRaises(HTTPError) as error:
            urlopen(request)

        self.assertEqual(error.exception.code, 405)
        error.exception.close()


    def post_json(self, path, data):
        request = Request(
            self.base_url + path,
            data=json.dumps(data).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urlopen(request) as response:
            return response.status, json.loads(response.read())

    def upload(self, system, filename, content=b"test-rom"):
        request = Request(
            self.base_url + (
                f"/api/upload?system={system}&filename={filename}"
            ),
            data=content,
            headers={"Content-Type": "application/octet-stream"},
            method="POST",
        )
        return urlopen(request)

    def test_upload_adds_rom_to_library(self):
        with self.upload("gb", "NewGame.gb", b"new-rom") as response:
            self.assertEqual(response.status, 201)
            data = json.loads(response.read())

        self.assertTrue(data["success"])
        self.assertEqual(data["filename"], "NewGame.gb")
        self.assertEqual(data["size_bytes"], 7)
        self.assertTrue((self.roms / "gb" / "NewGame.gb").is_file())
        self.assertIn(
            "NewGame",
            [game["title"] for game in data["catalog"]["available"]],
        )

    def test_upload_rejects_duplicate_filename(self):
        with self.upload("gb", "Duplicate.gb", b"first"):
            pass

        with self.assertRaises(HTTPError) as error:
            self.upload("gb", "Duplicate.gb", b"second")

        self.assertEqual(error.exception.code, 400)
        error.exception.close()
        self.assertEqual(
            (self.roms / "gb" / "Duplicate.gb").read_bytes(), b"first"
        )

    def test_upload_rejects_unsupported_extension(self):
        with self.assertRaises(HTTPError) as error:
            self.upload("gb", "WrongSystem.gba")

        self.assertEqual(error.exception.code, 400)
        error.exception.close()
        self.assertFalse((self.roms / "gb" / "WrongSystem.gba").exists())

    def test_upload_rejects_invalid_system_and_filename(self):
        for system, filename in (
            ("unknown", "Game.gb"),
            ("gb", "../Game.gb"),
            ("gb", r"..\\Game.gb"),
            ("gb", ".hidden.gb"),
        ):
            with self.subTest(system=system, filename=filename):
                with self.assertRaises(HTTPError) as error:
                    self.upload(system, filename)
                self.assertEqual(error.exception.code, 400)
                error.exception.close()

    def test_assign_replace_unassign_and_delete_endpoints(self):
        with self.upload("gb", "RemoteGame.gb", b"remote"):
            pass

        catalog = json.loads(
            urlopen(self.base_url + "/api/catalog").read()
        )
        uploaded = next(
            game for game in catalog["available"]
            if game["filename"] == "RemoteGame.gb"
        )

        status, assigned = self.post_json(
            "/api/slots/assign",
            {"slot": 1, "game_id": uploaded["id"], "replace": True},
        )
        self.assertEqual(status, 200)
        self.assertEqual(
            assigned["slots"][0]["game"]["filename"], "RemoteGame.gb"
        )
        self.assertIn(
            "Mario Tennis",
            [game["title"] for game in assigned["available"]],
        )

        status, unassigned = self.post_json(
            "/api/slots/unassign", {"slot": 1}
        )
        self.assertEqual(status, 200)
        self.assertIsNone(unassigned["slots"][0]["game"])
        self.assertIn(
            "RemoteGame",
            [game["title"] for game in unassigned["available"]],
        )

        status, deleted = self.post_json(
            "/api/games/delete", {"game_id": uploaded["id"]}
        )
        self.assertEqual(status, 200)
        self.assertFalse((self.roms / "gb" / "RemoteGame.gb").exists())
        self.assertNotIn(
            "RemoteGame",
            [game["title"] for game in deleted["games"]],
        )

    def test_assign_requires_explicit_replace(self):
        with self.upload("gb", "ReplacementCandidate.gb", b"candidate"):
            pass

        catalog = json.loads(
            urlopen(self.base_url + "/api/catalog").read()
        )
        game = next(
            item for item in catalog["available"]
            if item["filename"] == "ReplacementCandidate.gb"
        )
        request = Request(
            self.base_url + "/api/slots/assign",
            data=json.dumps({
                "slot": 1,
                "game_id": game["id"],
            }).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with self.assertRaises(HTTPError) as error:
            urlopen(request)

        self.assertEqual(error.exception.code, 400)
        error.exception.close()

        after = json.loads(
            urlopen(self.base_url + "/api/catalog").read()
        )
        self.assertNotEqual(
            after["slots"][0]["game"]["id"], game["id"]
        )
        self.assertIn(
            game["id"], [item["id"] for item in after["available"]]
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
