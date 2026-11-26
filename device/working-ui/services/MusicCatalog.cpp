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
std::vector<MusicTrack> musicItems;
std::vector<KiddosMusicPlaylist> playlistItems;
}

const std::vector<MusicTrack>& music() { return musicItems; }
const std::vector<KiddosMusicPlaylist>& musicPlaylists() { return playlistItems; }

bool loadMusic() {
    KiddosLibrary::Slots slots{};
    if (!KiddosLibrary::loadSlots(kSlots, slots)) {
        std::fprintf(stderr, "KiddOS: unable to load native music slots\n");
        return false;
    }

    const auto discovered =
        KiddosLibrary::discover(kRoot, KiddosLibrary::Category::Music);

    std::vector<KiddosMusicPlaylist> nextPlaylists;
    std::vector<MusicTrack> nextTracks;

    for (const auto& reference : slots[2]) {
        KiddosMusicPlaylist playlist;
        if (!reference.empty()) {
            for (const auto& item : discovered) {
                std::error_code ec;
                const auto relative = std::filesystem::relative(item.path, kRoot, ec);
                if (ec || relative.generic_string() != reference) continue;

                playlist.title = item.title;
                playlist.path = item.path;
                playlist.artwork = item.artwork;
                for (const auto& path : item.files) {
                    playlist.tracks.push_back({
                        std::filesystem::path(path).stem().string(), path
                    });
                }
                break;
            }
        }

        // Keep the playlist in its original slot, even when empty or missing.
        for (const auto& track : playlist.tracks)
            nextTracks.push_back(track);
        nextPlaylists.push_back(std::move(playlist));
    }

    playlistItems = std::move(nextPlaylists);
    musicItems = std::move(nextTracks);
    std::printf("KiddOS: loaded %zu music playlist slots\n",
                playlistItems.size());
    return true;
}
}
