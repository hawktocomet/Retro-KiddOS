# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 hawktocomet

"""Small, credential-free metadata search providers for Retro KiddOS."""

import json
from html.parser import HTMLParser
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

USER_AGENT = "RetroKiddOS/0.1 (metadata lookup)"
REQUEST_TIMEOUT = 8
MAX_RESULTS = 10


class ProviderError(RuntimeError):
    """A metadata provider could not complete a search."""


class _TextExtractor(HTMLParser):
    def __init__(self):
        super().__init__()
        self.parts = []

    def handle_data(self, data):
        value = data.strip()
        if value:
            self.parts.append(value)


def _plain_text(value):
    if not value:
        return ""
    parser = _TextExtractor()
    parser.feed(value)
    return " ".join(parser.parts)


def _get_json(url):
    request = Request(url, headers={
        "Accept": "application/json",
        "User-Agent": USER_AGENT,
    })
    try:
        with urlopen(request, timeout=REQUEST_TIMEOUT) as response:
            return json.loads(response.read().decode("utf-8"))
    except (HTTPError, URLError, TimeoutError, OSError) as exc:
        raise ProviderError(f"Metadata provider request failed: {exc}") from exc
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ProviderError("Metadata provider returned invalid JSON") from exc


def _musicbrainz_artist(credits):
    names = []
    for credit in credits or []:
        if isinstance(credit, dict):
            artist = credit.get("artist") or {}
            name = artist.get("name") or credit.get("name")
            if name:
                names.append(str(name))
        elif isinstance(credit, str):
            names.append(credit)
    return "".join(names) if names else "Unknown artist"


def search_music(query, limit=8):
    """Search MusicBrainz release groups (album-level metadata)."""
    params = urlencode({"query": query, "fmt": "json", "limit": limit})
    data = _get_json(f"https://musicbrainz.org/ws/2/release-group/?{params}")
    results = []

    for item in data.get("release-groups", []):
        mbid = item.get("id")
        title = item.get("title")
        if not mbid or not title:
            continue
        results.append({
            "id": mbid,
            "title": title,
            "artist": _musicbrainz_artist(item.get("artist-credit")),
            "release_date": item.get("first-release-date"),
            "type": item.get("primary-type"),
            "secondary_types": item.get("secondary-types") or [],
            "source": "MusicBrainz",
            "source_url": f"https://musicbrainz.org/release-group/{mbid}",
            "license": "MusicBrainz data: CC0; supplementary data may have other terms",
            "artwork_url": f"https://coverartarchive.org/release-group/{mbid}/front",
        })
    return results


def search_tv(query, limit=8):
    """Search TVmaze shows and return metadata plus optional image URLs."""
    params = urlencode({"q": query})
    data = _get_json(f"https://api.tvmaze.com/search/shows?{params}")
    results = []

    for entry in data[:limit]:
        show = entry.get("show") or {}
        show_id = show.get("id")
        title = show.get("name")
        if show_id is None or not title:
            continue
        image = show.get("image") or {}
        results.append({
            "id": str(show_id),
            "title": title,
            "summary": _plain_text(show.get("summary")),
            "premiered": show.get("premiered"),
            "ended": show.get("ended"),
            "genres": show.get("genres") or [],
            "network": (show.get("network") or {}).get("name"),
            "status": show.get("status"),
            "source": "TVmaze",
            "source_url": show.get("url") or f"https://www.tvmaze.com/shows/{show_id}",
            "license": "TVmaze data: CC BY-SA; attribution and ShareAlike apply",
            "artwork_url": image.get("original") or image.get("medium"),
        })
    return results


_SEARCHERS = {
    "music": ("MusicBrainz", search_music),
    "tv": ("TVmaze", search_tv),
}


def search(category, query, limit=8):
    """Search one supported category and return a stable response envelope."""
    category = str(category or "").strip().lower()
    query = str(query or "").strip()

    if category not in _SEARCHERS:
        raise ValueError("Supported metadata categories are music and tv")
    if not query:
        raise ValueError("A search query is required")
    if len(query) > 160:
        raise ValueError("Search query is too long")
    try:
        limit = int(limit)
    except (TypeError, ValueError):
        raise ValueError("Result limit must be a number")
    if not 1 <= limit <= MAX_RESULTS:
        raise ValueError(f"Result limit must be between 1 and {MAX_RESULTS}")

    provider_name, searcher = _SEARCHERS[category]
    return {
        "category": category,
        "query": query,
        "provider": provider_name,
        "results": searcher(query, limit),
    }
