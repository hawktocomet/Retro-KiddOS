#include "MusicCatalog.hpp"
#include "LibraryData.hpp"
#include "../models/Models.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace KiddosServices {
namespace {
constexpr const char* kRoot = "/storage/roms";
constexpr const char* kSlots = "/storage/.config/kiddos/slots.conf";
std::vector<KiddosMusicPlaylist> playlistItems;
}

const std::vector<KiddosMusicPlaylist>& musicPlaylists() { return playlistItems; }

bool loadMusic() {
    KiddosLibrary::Slots slots{};
    if (!KiddosLibrary::loadSlots(kSlots, slots)) {
        std::fprintf(stderr, "KiddOS: unable to load native music slots\n");
        return false;
    }

    const auto discovered =
        KiddosLibrary::discover(kRoot, KiddosLibrary::Category::Music);

    const auto resolved = KiddosLibrary::resolveSlots(
        kRoot, slots, KiddosLibrary::Category::Music, discovered);
    std::vector<KiddosMusicPlaylist> nextPlaylists;

    for (const auto* item : resolved) {
        KiddosMusicPlaylist playlist;
        if (item) {
            playlist.title = item->title;
            playlist.path = item->path;
            playlist.artwork = item->artwork;
            for (const auto& path : item->files) {
                playlist.tracks.push_back({
                    std::filesystem::path(path).stem().string(), path
                });
            }
        }

        // Keep the playlist in its original slot, even when empty or missing.
        nextPlaylists.push_back(std::move(playlist));
    }

    playlistItems = std::move(nextPlaylists);
    std::printf("KiddOS: loaded %zu music playlist slots\n",
                playlistItems.size());
    return true;
}
}
