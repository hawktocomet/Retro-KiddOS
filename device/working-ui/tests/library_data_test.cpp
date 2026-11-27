// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "services/LibraryData.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace KiddosLibrary;

int main() {
    char temp[] = "/tmp/kiddos-library-test-XXXXXX";
    if (!mkdtemp(temp)) return 1;

    const fs::path root(temp);
    int failures = 0;

    auto check = [&](bool ok, const char* message) {
        std::cout << (ok ? "PASS: " : "FAIL: ") << message << '\n';
        if (!ok) ++failures;
    };

    auto file = [](const fs::path& path) {
        fs::create_directories(path.parent_path());
        std::ofstream(path) << "test";
    };

    file(root / "movies/Film.mp4");
    file(root / "movies/Film.png");
    file(root / "movies/No-Artwork.mp4");
    file(root / "movies/Mixed.MP4");
    file(root / "movies/Mixed.PnG");

    file(root / "music/Playlist/cover.jpg");
    file(root / "music/Playlist/Disc 1/Track 01.MP3");
    file(root / "music/Playlist/Disc 1/Track 02.mp3");
    file(root / "music/No-Cover/song.mp3");
    file(root / "music/MixedCase/cover.JPG");
    file(root / "music/MixedCase/Track.Mp3");
    file(root / "music/.hidden/Hidden/cover.jpg");
    file(root / "music/.hidden/Hidden/hidden.mp3");

    file(root / "tvshows/Show/poster.jpg");
    file(root / "tvshows/Show/Season 1/Episode 01.mkv");
    file(root / "tvshows/No-Poster/Episode 01.mkv");
    file(root / "tvshows/MixedCase/poster.PNG");
    file(root / "tvshows/MixedCase/Episode.MKV");

    file(root / "gb/Tetris.gb");
    file(root / "gb/Tetris.jpg");
    file(root / "gb/Tetris.png");
    file(root / "gb/Tetris.webp");
    file(root / "gb/No-Artwork.gb");
    file(root / "gb/Archive.zip");
    file(root / "gb/Archive.png");
    file(root / "gbc/Pokemon Crystal.GBC");
    file(root / "gbc/Pokemon Crystal.WEBP");

    auto games = discover(root, Category::Games);
    auto movies = discover(root, Category::Movies);
    auto music = discover(root, Category::Music);
    auto shows = discover(root, Category::TVShows);

    check(games.size() == 2,
          "GB and GBC games require artwork; ZIP files are not slot-eligible");
    bool foundGb = false;
    bool foundGbc = false;
    for (const auto& item : games) {
        if (item.title == "Tetris")
            foundGb = item.platform == "gb" &&
                      item.artwork.find("Tetris.png") != std::string::npos;
        if (item.title == "Pokemon Crystal")
            foundGbc = item.platform == "gbc" &&
                       item.artwork.find("Pokemon Crystal.WEBP") != std::string::npos;
    }
    check(foundGb, "GB game discovery preserves platform and PNG artwork priority");
    check(foundGbc, "GBC game discovery accepts mixed-case extensions and WebP artwork");

    check(movies.size() == 2,
          "movies require artwork and accept mixed-case extensions");
    check(music.size() == 2,
          "music playlists require artwork and accept mixed-case extensions");
    check(shows.size() == 2,
          "TV shows require artwork and accept mixed-case extensions");

    bool foundPlaylist = false;
    for (const auto& item : music) {
        if (item.title == "Playlist")
            foundPlaylist = item.files.size() == 2 &&
                            item.artwork.find("cover.jpg") != std::string::npos;
    }
    check(foundPlaylist, "playlist is one item containing its tracks");

    bool hiddenFound = false;
    for (const auto& item : music)
        if (item.title == "Hidden") hiddenFound = true;
    check(!hiddenFound, "hidden directories are excluded");

    Slots slots{};
    check(assign(slots, Category::Music, 0, "music/Playlist"),
          "playlist can be assigned by relative path");
    check(assign(slots, Category::Music, 1, "music/Playlist"),
          "reassignment moves an item to its new slot");
    check(slots[2][0].empty() && slots[2][1] == "music/Playlist",
          "an item cannot occupy two slots in one category");

    check(!assign(slots, Category::Music, 2, "../outside"),
          "parent traversal is rejected");
    check(!assign(slots, Category::Music, 2, "/absolute/path"),
          "absolute slot paths are rejected");
    check(!assign(slots, Category::Music, 2, "music/./Playlist"),
          "dot path components are rejected");
    check(!assign(slots, Category::Music, 4, "music/Playlist"),
          "out-of-range slot indexes are rejected");

    Slots unsafe{};
    unsafe[2][0] = "../outside";
    check(!saveSlots(root / "settings/unsafe.conf", unsafe),
          "unsafe references cannot be saved");

    const fs::path corrupt = root / "settings/corrupt.conf";
    fs::create_directories(corrupt.parent_path());
    {
        std::ofstream out(corrupt);
        out << "KIDDOS_SLOTS_V1\n";
        out << "\"../outside\"\n";
        for (int i = 1; i < 16; ++i) out << "\"\"\n";
    }
    Slots rejected{};
    check(!loadSlots(corrupt, rejected),
          "unsafe references in an existing slot file are rejected");

    const fs::path config = root / "settings/slots.conf";
    check(saveSlots(config, slots), "slot assignments save atomically");

    Slots restored{};
    check(loadSlots(config, restored) && restored == slots,
          "slot assignments reload without a media catalog");

    check(clear(restored, Category::Music, 1) &&
          restored[2][1].empty(),
          "clearing a slot only clears its reference");


    // Shared slot resolution covers all four categories.
    auto reference = [&](const Item& item) {
        return fs::path(item.path).lexically_relative(root).generic_string();
    };

    Slots resolvedSlots{};
    resolvedSlots[0][0] = reference(games[0]);
    resolvedSlots[0][2] = "gb/Missing.gb";
    resolvedSlots[0][3] = reference(games[1]);
    const auto resolvedGames =
        resolveSlots(root, resolvedSlots, Category::Games, games);
    check(resolvedGames[0] == &games[0] &&
          resolvedGames[1] == nullptr &&
          resolvedGames[2] == nullptr &&
          resolvedGames[3] == &games[1],
          "game resolution preserves empty and missing slots");

    resolvedSlots = {};
    resolvedSlots[1][0] = reference(movies[0]);
    check(resolveSlots(root, resolvedSlots, Category::Movies, movies)[0] ==
              &movies[0], "movie slots use shared resolution");

    resolvedSlots = {};
    resolvedSlots[2][0] = reference(music[0]);
    check(resolveSlots(root, resolvedSlots, Category::Music, music)[0] ==
              &music[0], "music slots use shared resolution");

    resolvedSlots = {};
    resolvedSlots[3][0] = reference(shows[0]);
    check(resolveSlots(root, resolvedSlots, Category::TVShows, shows)[0] ==
              &shows[0], "TV show slots use shared resolution");

    fs::remove_all(root);
    return failures == 0 ? 0 : 1;
}
