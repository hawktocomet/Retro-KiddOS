#include "GameCatalog.hpp"

#include "LibraryData.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace KiddosServices {
namespace {
constexpr const char* kRoot = "/storage/roms";
constexpr const char* kSlots = "/storage/.config/kiddos/slots.conf";
std::array<Game, 4> gameItems{};

bool assignedItem(const std::vector<KiddosLibrary::Item>& items,
                  const std::string& reference,
                  KiddosLibrary::Item& result) {
    namespace fs = std::filesystem;
    for (const auto& item : items) {
        std::error_code ec;
        const auto relative = fs::relative(item.path, kRoot, ec);
        if (!ec && relative.generic_string() == reference) {
            result = item;
            return true;
        }
    }
    return false;
}
}

const std::array<Game, 4>& games() { return gameItems; }

bool loadGameSlots() {
    KiddosLibrary::Slots slots{};
    if (!KiddosLibrary::loadSlots(kSlots, slots)) {
        std::fprintf(stderr, "KiddOS: unable to load native game slots\n");
        return false;
    }

    const auto items = KiddosLibrary::discover(
        kRoot, KiddosLibrary::Category::Games);

    std::array<Game, 4> next{};
    for (std::size_t i = 0; i < next.size(); ++i) {
        KiddosLibrary::Item item;
        if (!slots[0][i].empty() &&
            assignedItem(items, slots[0][i], item)) {
            next[i].name = item.title;
            next[i].rom = item.path;
            next[i].platform = item.platform;
            next[i].image = item.artwork;
            next[i].available = true;
        }
        std::printf("KiddOS: game slot %zu: %s\n", i + 1,
                    next[i].available ? next[i].name.c_str() : "(empty)");
    }

    gameItems = std::move(next);
    return true;
}
}
