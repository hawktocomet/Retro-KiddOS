# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import json
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

SERVER_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SERVER_DIR))

import server


class ManagementActivityTests(unittest.TestCase):
    def test_active_request_prevents_idle_shutdown(self):
        tracker = server.ManagementActivity()
        with tracker._lock:
            tracker._last_activity -= 1000
        tracker.begin_request()
        self.assertFalse(tracker.idle_expired())
        tracker.finish_request(successful=False)
        self.assertTrue(tracker.idle_expired())

    def test_successful_request_resets_idle_deadline(self):
        tracker = server.ManagementActivity()
        with tracker._lock:
            tracker._last_activity -= 1000
        tracker.begin_request()
        tracker.finish_request(successful=True)
        self.assertFalse(tracker.idle_expired())


class ManagementWatchdogTests(unittest.TestCase):
    def test_watchdog_stops_serve_forever_cleanly(self):
        previous_activity = server.ACTIVITY
        tracker = server.ManagementActivity()
        with tracker._lock:
            tracker._last_activity -= 1000
        server.ACTIVITY = tracker

        httpd = ThreadingHTTPServer(("127.0.0.1", 0), server.Handler)
        stop_event = threading.Event()
        server_thread = threading.Thread(
            target=httpd.serve_forever, daemon=True
        )
        watchdog_thread = threading.Thread(
            target=server.idle_shutdown_watchdog,
            args=(httpd, stop_event),
            daemon=True,
        )

        try:
            server_thread.start()
            watchdog_thread.start()

            server_thread.join(timeout=3)
            watchdog_thread.join(timeout=3)

            self.assertFalse(
                server_thread.is_alive(),
                "Watchdog did not stop the HTTP server",
            )
            self.assertFalse(
                watchdog_thread.is_alive(),
                "Watchdog thread did not finish",
            )
        finally:
            stop_event.set()
            if server_thread.is_alive():
                httpd.shutdown()
                server_thread.join(timeout=2)
            httpd.server_close()
            server.ACTIVITY = previous_activity


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
            (legacy / (Path(name).stem + ".png")).write_bytes(b"artwork")

        server.ROM_ROOT = self.roms
        server.STATE_ROOT = self.state
        server.ACTIVITY = server.ManagementActivity()

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

    def test_media_catalog_endpoint_returns_artwork_eligible_media(self):
        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Movie One.mp4").write_bytes(b"movie")
        (movies / "Movie One.jpg").write_bytes(b"art")
        (movies / "No Artwork.mp4").write_bytes(b"movie")

        playlist = self.roms / "music" / "Bedtime"
        playlist.mkdir(parents=True)
        (playlist / "cover.jpg").write_bytes(b"art")
        (playlist / "01 - Song.mp3").write_bytes(b"audio")
        (playlist / "02 - Song.mp3").write_bytes(b"audio")

        no_cover = self.roms / "music" / "No Cover"
        no_cover.mkdir()
        (no_cover / "Song.mp3").write_bytes(b"audio")

        show = self.roms / "tvshows" / "Bluey"
        show.mkdir(parents=True)
        (show / "poster.png").write_bytes(b"art")
        (show / "S01E01.mkv").write_bytes(b"episode")

        with urlopen(self.base_url + "/api/media-catalog") as response:
            self.assertEqual(response.status, 200)
            data = json.loads(response.read())

        self.assertEqual(
            [item["title"] for item in data["movies"]],
            ["Movie One"],
        )
        self.assertEqual(
            [item["title"] for item in data["music_playlists"]],
            ["Bedtime"],
        )
        self.assertEqual(len(data["music_playlists"][0]["tracks"]), 2)
        self.assertEqual(
            [item["title"] for item in data["tv_shows"]],
            ["Bluey"],
        )
        self.assertEqual(len(data["tv_shows"][0]["episodes"]), 1)

    def test_media_slots_endpoint_assigns_and_unassigns_all_categories(self):
        movies = self.roms / "movies"
        movies.mkdir()
        (movies / "Movie One.mp4").write_bytes(b"movie")
        (movies / "Movie One.jpg").write_bytes(b"art")
        (movies / "Movie Two.mp4").write_bytes(b"movie")
        (movies / "Movie Two.jpg").write_bytes(b"art")

        playlist = self.roms / "music" / "Bedtime"
        playlist.mkdir(parents=True)
        (playlist / "cover.jpg").write_bytes(b"art")
        (playlist / "Song.mp3").write_bytes(b"audio")

        show = self.roms / "tvshows" / "Bluey"
        show.mkdir(parents=True)
        (show / "poster.png").write_bytes(b"art")
        (show / "Episode.mp4").write_bytes(b"episode")

        with urlopen(self.base_url + "/api/media-slots") as response:
            self.assertEqual(response.status, 200)
            initial = json.loads(response.read())

        categories = ("movies", "music_playlists", "tv_shows")
        for category in categories:
            self.assertEqual(len(initial[category]["slots"]), 4)
            self.assertTrue(
                all(slot["item"] is None
                    for slot in initial[category]["slots"])
            )

        assignments = {
            category: initial[category]["available"][0]
            for category in categories
        }
        for category, item in assignments.items():
            status, result = self.post_json(
                "/api/media-slots/assign",
                {
                    "category": category,
                    "slot": 1,
                    "item_id": item["id"],
                },
            )
            self.assertEqual(status, 200)
            self.assertEqual(
                result[category]["slots"][0]["item"]["id"], item["id"]
            )

        with urlopen(self.base_url + "/api/media-slots") as response:
            persisted = json.loads(response.read())

        for category, item in assignments.items():
            self.assertEqual(
                persisted[category]["slots"][0]["item"]["id"], item["id"]
            )

        status, result = self.post_json(
            "/api/media-slots/unassign",
            {"category": "movies", "slot": 1},
        )
        self.assertEqual(status, 200)
        self.assertIsNone(result["movies"]["slots"][0]["item"])
        self.assertTrue(Path(assignments["movies"]["path"]).is_file())

    def test_media_slots_require_explicit_replace_and_reject_invalid_items(self):
        movies = self.roms / "movies"
        movies.mkdir()
        for name in ("First", "Second"):
            (movies / f"{name}.mp4").write_bytes(b"movie")
            (movies / f"{name}.jpg").write_bytes(b"art")

        with urlopen(self.base_url + "/api/media-slots") as response:
            catalog = json.loads(response.read())
        first, second = catalog["movies"]["available"]

        status, _ = self.post_json(
            "/api/media-slots/assign",
            {"category": "movies", "slot": 1, "item_id": first["id"]},
        )
        self.assertEqual(status, 200)

        with self.assertRaises(HTTPError) as error:
            self.post_json(
                "/api/media-slots/assign",
                {"category": "movies", "slot": 1, "item_id": second["id"]},
            )
        self.assertEqual(error.exception.code, 400)
        error.exception.close()

        with self.assertRaises(HTTPError) as error:
            self.post_json(
                "/api/media-slots/assign",
                {"category": "movies", "slot": 2, "item_id": "missing"},
            )
        self.assertEqual(error.exception.code, 400)
        error.exception.close()

        for invalid_category in ([], {}):
            for endpoint, payload in (
                (
                    "/api/media-slots/assign",
                    {
                        "category": invalid_category,
                        "slot": 1,
                        "item_id": first["id"],
                    },
                ),
                (
                    "/api/media-slots/unassign",
                    {"category": invalid_category, "slot": 1},
                ),
            ):
                with self.assertRaises(HTTPError) as error:
                    self.post_json(endpoint, payload)
                self.assertEqual(error.exception.code, 400)
                error.exception.close()

    def test_media_slots_get_counts_as_activity(self):
        with server.ACTIVITY._lock:
            server.ACTIVITY._last_activity -= 1000
        with urlopen(self.base_url + "/api/media-slots") as response:
            self.assertEqual(response.status, 200)
        self.assertFalse(server.ACTIVITY.idle_expired())

    def test_media_catalog_get_counts_as_activity(self):
        with server.ACTIVITY._lock:
            server.ACTIVITY._last_activity -= 1000
        with urlopen(self.base_url + "/api/media-catalog") as response:
            self.assertEqual(response.status, 200)
        self.assertFalse(server.ACTIVITY.idle_expired())

    def test_artwork_endpoint_returns_image_for_game(self):
        rom = self.roms / ".kiddos-ui-test-20261116" / "Pokemon Red Version.gb"
        artwork = rom.with_suffix(".png")
        image = bytes.fromhex("89504e470d0a1a0a") + bytes(8)
        artwork.write_bytes(image)

        catalog = json.loads(urlopen(self.base_url + "/api/catalog").read())
        game = next(
            item for item in catalog["games"]
            if item["filename"] == rom.name
        )
        with urlopen(
            self.base_url + f"/api/artwork?game_id={game['id']}"
        ) as response:
            self.assertEqual(response.status, 200)
            self.assertEqual(response.headers["Content-Type"], "image/png")
            self.assertEqual(response.read(), image)

        with self.assertRaises(HTTPError) as error:
            urlopen(self.base_url + "/api/artwork?game_id=not-a-real-game")
        self.assertEqual(error.exception.code, 404)
        error.exception.close()

    def test_artwork_requests_reuse_cached_catalog(self):
        rom = self.roms / ".kiddos-ui-test-20261116" / "Pokemon Red Version.gb"
        rom.with_suffix(".png").write_bytes(
            bytes.fromhex("89504e470d0a1a0a") + bytes(8)
        )

        catalog = json.loads(urlopen(self.base_url + "/api/catalog").read())
        game = next(
            item for item in catalog["games"]
            if item["filename"] == rom.name
        )
        url = self.base_url + f"/api/artwork?game_id={game['id']}"

        original_game_catalog = server.game_catalog
        server.invalidate_game_catalog()

        with patch(
            "server.game_catalog", wraps=original_game_catalog
        ) as catalog_builder:
            for _ in range(2):
                with urlopen(url) as response:
                    self.assertEqual(response.status, 200)
                    self.assertTrue(response.read())

            catalog_builder.assert_called_once()

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

    def test_future_systems_stay_hidden_from_parent_apis(self):
        for system in ("gb", "gbc", "gba"):
            (self.roms / system).mkdir(exist_ok=True)

        advance = self.roms / "gba" / "AdvanceGame.gba"
        advance.write_bytes(b"future-system-rom")

        discovered = server.discover_games(self.roms)
        self.assertIn(
            "AdvanceGame",
            [game["title"] for game in discovered],
        )

        with urlopen(self.base_url + "/api/catalog") as response:
            catalog = json.loads(response.read())
        self.assertNotIn(
            "Advance Game",
            [game["title"] for game in catalog["games"]],
        )

        with urlopen(self.base_url + "/api/library") as response:
            library = json.loads(response.read())
        self.assertNotIn(
            "Advance Game",
            [game["title"] for game in library["games"]],
        )
        self.assertEqual(
            {system["id"] for system in library["systems"]},
            {"gb", "gbc"},
        )

    def test_status_get_does_not_reset_idle_timer(self):
        with server.ACTIVITY._lock:
            server.ACTIVITY._last_activity -= 1000
        with urlopen(self.base_url + "/api/status") as response:
            self.assertEqual(response.status, 200)
        self.assertTrue(server.ACTIVITY.idle_expired())

    def test_catalog_get_counts_as_activity(self):
        with server.ACTIVITY._lock:
            server.ACTIVITY._last_activity -= 1000
        with urlopen(self.base_url + "/api/catalog") as response:
            self.assertEqual(response.status, 200)
        self.assertFalse(server.ACTIVITY.idle_expired())

    def test_activity_endpoint_counts_as_activity(self):
        with server.ACTIVITY._lock:
            server.ACTIVITY._last_activity -= 1000
        status, result = self.post_json("/api/activity", {})
        self.assertEqual(status, 200)
        self.assertTrue(result["success"])
        self.assertFalse(server.ACTIVITY.idle_expired())

    def test_post_remains_disabled(self):
        for endpoint in ("/api/catalog", "/api/media-catalog"):
            request = Request(
                self.base_url + endpoint,
                data=b"{}",
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with self.subTest(endpoint=endpoint):
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

    def upload_artwork(self, game_id, filename="cover.png", content=None):
        if content is None:
            content = bytes.fromhex("89504e470d0a1a0a") + bytes(8)
        request = Request(
            self.base_url + (
                f"/api/artwork/upload?game_id={game_id}&filename={filename}"
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
        self.assertNotIn(
            "NewGame",
            [game["title"] for game in data["catalog"]["available"]],
        )

        game = next(
            item for item in data["catalog"]["games"]
            if item["filename"] == "NewGame.gb"
        )
        with self.upload_artwork(game["id"]) as response:
            self.assertEqual(response.status, 201)
            artwork_data = json.loads(response.read())

        self.assertTrue((self.roms / "gb" / "NewGame.png").is_file())
        self.assertIn(
            "NewGame",
            [item["title"] for item in artwork_data["catalog"]["available"]],
        )

    def test_artwork_upload_rejects_invalid_image_contents(self):
        with self.upload("gb", "NoArt.gb", b"rom"):
            pass
        catalog = json.loads(urlopen(self.base_url + "/api/catalog").read())
        game = next(
            item for item in catalog["games"]
            if item["filename"] == "NoArt.gb"
        )

        with self.assertRaises(HTTPError) as error:
            self.upload_artwork(game["id"], content=b"not-an-image")
        self.assertEqual(error.exception.code, 400)
        error.exception.close()
        self.assertFalse((self.roms / "gb" / "NoArt.png").exists())

    def test_artwork_upload_rejects_unknown_game(self):
        with self.assertRaises(HTTPError) as error:
            self.upload_artwork("not-a-real-game")
        self.assertEqual(error.exception.code, 400)
        error.exception.close()

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

    def test_upload_rejects_disabled_future_system(self):
        with self.assertRaises(HTTPError) as error:
            self.upload("gba", "AdvanceGame.gba")

        self.assertEqual(error.exception.code, 400)
        error.exception.close()
        self.assertFalse((self.roms / "gba" / "AdvanceGame.gba").exists())

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
            game for game in catalog["games"]
            if game["filename"] == "RemoteGame.gb"
        )
        with self.upload_artwork(uploaded["id"]):
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

        rom_path = self.roms / "gb" / "RemoteGame.gb"
        artwork_path = rom_path.with_suffix(".png")
        self.assertTrue(artwork_path.is_file())

        status, deleted = self.post_json(
            "/api/games/delete", {"game_id": uploaded["id"]}
        )
        self.assertEqual(status, 200)
        self.assertFalse(rom_path.exists())
        self.assertFalse(artwork_path.exists())
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
            item for item in catalog["games"]
            if item["filename"] == "ReplacementCandidate.gb"
        )
        with self.upload_artwork(game["id"]):
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


    def _first_game_id(self):
        catalog = json.loads(
            urlopen(self.base_url + "/api/catalog").read()
        )
        return catalog["games"][0]["id"]







    def test_device_side_artwork_find_endpoint_is_removed(self):
        request = Request(
            self.base_url + "/api/artwork/find",
            data=b"",
            method="POST",
        )
        with self.assertRaises(HTTPError) as error:
            urlopen(request)

        self.assertEqual(error.exception.code, 404)
        error.exception.close()






if __name__ == "__main__":
    unittest.main(verbosity=2)
