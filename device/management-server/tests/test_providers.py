# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
from urllib.request import Request

SERVER_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SERVER_DIR))

import providers


class FakeResponse:
    def __init__(self, payload):
        self.payload = json.dumps(payload).encode("utf-8")

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def read(self):
        return self.payload


class ProviderTests(unittest.TestCase):
    @patch("providers.urlopen")
    def test_music_search_normalizes_release_group(self, mocked_urlopen):
        mocked_urlopen.return_value = FakeResponse({
            "release-groups": [{
                "id": "release-id",
                "title": "Album Name",
                "first-release-date": "1999",
                "primary-type": "Album",
                "secondary-types": ["Soundtrack"],
                "artist-credit": [
                    {"artist": {"name": "Artist One"}},
                    " & ",
                    {"artist": {"name": "Artist Two"}},
                ],
            }]
        })
        result = providers.search("music", "Album Name")
        item = result["results"][0]

        self.assertEqual(result["provider"], "MusicBrainz")
        self.assertEqual(item["artist"], "Artist One & Artist Two")
        self.assertEqual(
            item["artwork_url"],
            "https://coverartarchive.org/release-group/release-id/front",
        )
        request = mocked_urlopen.call_args.args[0]
        self.assertIsInstance(request, Request)
        self.assertIn("RetroKiddOS", request.get_header("User-agent"))
        self.assertEqual(mocked_urlopen.call_args.kwargs["timeout"], 8)

    @patch("providers.urlopen")
    def test_tv_search_strips_summary_html(self, mocked_urlopen):
        mocked_urlopen.return_value = FakeResponse([{
            "show": {
                "id": 42,
                "name": "Example Show",
                "summary": "<p>A <b>great</b> show.</p>",
                "premiered": "2001-01-01",
                "genres": ["Family"],
                "image": {"medium": "https://images.example/medium.jpg"},
                "url": "https://www.tvmaze.com/shows/42/example-show",
            }
        }])
        item = providers.search("tv", "Example Show")["results"][0]

        self.assertEqual(item["summary"], "A great show.")
        self.assertEqual(item["artwork_url"], "https://images.example/medium.jpg")
        self.assertIn("CC BY-SA", item["license"])

    def test_rejects_invalid_category_and_query(self):
        for category, query in (("movies", "A film"), ("music", "  ")):
            with self.subTest(category=category):
                with self.assertRaises(ValueError):
                    providers.search(category, query)

    def test_rejects_long_query_and_invalid_limit(self):
        with self.assertRaises(ValueError):
            providers.search("tv", "x" * 161)
        with self.assertRaises(ValueError):
            providers.search("music", "album", limit=11)

    @patch("providers.urlopen")
    def test_provider_failure_is_reported(self, mocked_urlopen):
        from urllib.error import URLError
        mocked_urlopen.side_effect = URLError("offline")
        with self.assertRaises(providers.ProviderError):
            providers.search("tv", "Example")


if __name__ == "__main__":
    unittest.main(verbosity=2)
