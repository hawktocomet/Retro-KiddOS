#include "MovieCatalog.hpp"
#include "LibraryData.hpp"
#include "../models/Models.hpp"

#include <cstdio>
#include <filesystem>
#include <vector>

namespace KiddosServices {
namespace {
constexpr const char* kRoot = "/storage/roms";
constexpr const char* kSlots = "/storage/.config/kiddos/slots.conf";
std::vector<Movie> movieItems;
}

const std::vector<Movie>& movies() { return movieItems; }

bool loadMovies() {
    KiddosLibrary::Slots slots{};
    if (!KiddosLibrary::loadSlots(kSlots, slots)) {
        std::fprintf(stderr, "KiddOS: unable to load native movie slots\n");
        return false;
    }

    const auto discovered =
        KiddosLibrary::discover(kRoot, KiddosLibrary::Category::Movies);
    std::vector<Movie> next;

    for (const auto& reference : slots[1]) {
        Movie movie;
        if (!reference.empty()) {
            for (const auto& item : discovered) {
                std::error_code ec;
                const auto relative = std::filesystem::relative(item.path, kRoot, ec);
                if (!ec && relative.generic_string() == reference) {
                    movie.title = item.title;
                    movie.path = item.path;
                    movie.artwork = item.artwork;
                    break;
                }
            }
        }
        next.push_back(std::move(movie));
    }

    movieItems = std::move(next);
    std::printf("KiddOS: loaded four movie slot positions\n");
    return true;
}
}
