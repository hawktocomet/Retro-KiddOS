#include "TVShowCatalog.hpp"
#include "LibraryData.hpp"
#include "../models/Models.hpp"

#include <cstdio>
#include <filesystem>
#include <utility>
#include <vector>

namespace KiddosServices {
namespace {
constexpr const char* kRoot = "/storage/roms";
constexpr const char* kSlots = "/storage/.config/kiddos/slots.conf";
std::vector<TVShow> showItems;
}

const std::vector<TVShow>& tvShows() { return showItems; }

bool loadTVShows() {
    KiddosLibrary::Slots slots{};
    if (!KiddosLibrary::loadSlots(kSlots, slots)) {
        std::fprintf(stderr, "KiddOS: unable to load native TV slots\n");
        return false;
    }

    const auto discovered =
        KiddosLibrary::discover(kRoot, KiddosLibrary::Category::TVShows);
    std::vector<TVShow> next;

    for (const auto& reference : slots[3]) {
        TVShow show;
        if (!reference.empty()) {
            for (const auto& item : discovered) {
                std::error_code ec;
                const auto relative = std::filesystem::relative(item.path, kRoot, ec);
                if (ec || relative.generic_string() != reference) continue;

                show.title = item.title;
                show.directory = item.path;
                show.artwork = item.artwork;
                show.episodes = item.files;
                break;
            }
        }
        next.push_back(std::move(show));
    }

    showItems = std::move(next);
    std::printf("KiddOS: loaded four TV show slot positions\n");
    return true;
}
}
