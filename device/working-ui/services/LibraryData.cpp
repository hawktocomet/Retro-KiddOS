// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "LibraryData.hpp"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <system_error>

namespace KiddosLibrary {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool hidden(const fs::path& path) {
    for (const auto& part : path)
        if (!part.empty() && part.string().front() == '.') return true;
    return false;
}

bool inside(const fs::path& root, const fs::path& candidate,
            fs::path& resolved) {
    std::error_code ec;
    resolved = fs::canonical(candidate, ec);
    if (ec) return false;

    auto r = root.begin();
    auto p = resolved.begin();
    for (; r != root.end(); ++r, ++p)
        if (p == resolved.end() || *r != *p) return false;
    return true;
}

bool regularInside(const fs::path& root, const fs::path& path,
                   fs::path& resolved) {
    std::error_code ec;
    return fs::is_regular_file(path, ec) && !ec &&
           inside(root, path, resolved);
}

bool directoryInside(const fs::path& root, const fs::path& path,
                     fs::path& resolved) {
    std::error_code ec;
    return fs::is_directory(path, ec) && !ec &&
           inside(root, path, resolved);
}

bool hasExtension(const fs::path& path,
                  const std::vector<std::string>& extensions) {
    const std::string ext = lower(path.extension().string());
    return std::find(extensions.begin(), extensions.end(), ext) !=
           extensions.end();
}

fs::path findArtwork(const fs::path& dir,
                     const std::vector<std::string>& preferredNames) {
    std::error_code ec;
    std::vector<std::pair<std::string, fs::path>> entries;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end;
         it.increment(ec)) {
        if (it->is_regular_file(ec) && !ec)
            entries.emplace_back(lower(it->path().filename().string()),
                                 it->path());
        ec.clear();
    }

    for (const auto& preferred : preferredNames) {
        const std::string wanted = lower(preferred);
        for (const auto& entry : entries)
            if (entry.first == wanted) return entry.second;
    }
    return {};
}

fs::path artworkForMovie(const fs::path& file) {
    return findArtwork(file.parent_path(),
        {file.stem().string() + ".png", file.stem().string() + ".jpg"});
}

fs::path artworkForGame(const fs::path& file) {
    return findArtwork(file.parent_path(),
        {file.stem().string() + ".png",
         file.stem().string() + ".jpg",
         file.stem().string() + ".webp"});
}

fs::path artworkForShow(const fs::path& dir) {
    return findArtwork(dir,
        {"poster.jpg", "poster.png", "cover.jpg", "cover.png"});
}

bool validRelativeReference(const std::string& value) {
    if (value.empty()) return false;
    const fs::path path(value);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
        return false;
    for (const auto& part : path)
        if (part == ".." || part == "." || part.empty()) return false;
    return true;
}

bool naturalLess(const std::string& a, const std::string& b) {
    return lower(a) < lower(b);
}

std::vector<fs::path> recursiveFiles(const fs::path& root,
                                     const fs::path& dir,
                                     const std::vector<std::string>& exts) {
    std::vector<fs::path> result;
    std::error_code ec;
    fs::recursive_directory_iterator it(
        dir, fs::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        if (hidden(it->path().lexically_relative(root))) {
            if (it->is_directory(ec)) it.disable_recursion_pending();
            ec.clear();
            continue;
        }
        fs::path resolved;
        if (regularInside(root, it->path(), resolved) &&
            hasExtension(resolved, exts))
            result.push_back(resolved);
        ec.clear();
    }
    std::sort(result.begin(), result.end(),
        [](const fs::path& a, const fs::path& b) {
            return lower(a.generic_string()) < lower(b.generic_string());
        });
    return result;
}


} // namespace

std::vector<Item> discover(const fs::path& rootPath, Category category) {
    std::vector<Item> items;
    std::error_code ec;
    fs::path root = fs::canonical(rootPath, ec);
    if (ec || !fs::is_directory(root)) return items;

    if (category == Category::Games) {
        const struct {
            const char* folder;
            const char* extension;
        } platforms[] = {
            {"gb", ".gb"},
            {"gbc", ".gbc"},
        };

        for (const auto& platform : platforms) {
            fs::path safeBase;
            if (!directoryInside(root, root / platform.folder, safeBase))
                continue;

            fs::directory_iterator it(safeBase, ec), end;
            for (; !ec && it != end; it.increment(ec)) {
                const fs::path path = it->path();
                if (hidden(path.filename()) ||
                    lower(path.extension().string()) != platform.extension)
                    continue;

                fs::path resolved;
                if (!regularInside(root, path, resolved))
                    continue;

                fs::path art = artworkForGame(path);
                fs::path safeArt;
                if (art.empty() || !regularInside(root, art, safeArt))
                    continue;

                Item item;
                item.title = path.stem().string();
                item.path = resolved.string();
                item.artwork = safeArt.string();
                item.platform = platform.folder;
                items.push_back(std::move(item));
            }
            ec.clear();
        }

        std::sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) {
                return naturalLess(a.title, b.title);
            });
        return items;
    }

    const char* folder = nullptr;
    switch (category) {
        case Category::Movies: folder = "movies"; break;
        case Category::Music: folder = "music"; break;
        case Category::TVShows: folder = "tvshows"; break;
        case Category::Games: break;
    }

    fs::path base = root / folder;
    fs::path safeBase;
    if (!directoryInside(root, base, safeBase)) return items;

    if (category == Category::Movies) {
        const std::vector<std::string> exts =
            {".mp4", ".mkv", ".avi", ".mov", ".m4v"};
        fs::directory_iterator it(safeBase, ec), end;
        for (; !ec && it != end; it.increment(ec)) {
            const fs::path path = it->path();
            if (hidden(path.filename()) || !hasExtension(path, exts)) continue;
            fs::path resolved;
            if (!regularInside(root, path, resolved)) continue;
            fs::path art = artworkForMovie(path);
            fs::path safeArt;
            if (art.empty() || !regularInside(root, art, safeArt)) continue;
            items.push_back({path.stem().string(), resolved.string(),
                             safeArt.string(), {}, {}});
        }
    } else {
        fs::directory_iterator it(safeBase, ec), end;
        for (; !ec && it != end; it.increment(ec)) {
            const fs::path dir = it->path();
            if (hidden(dir.filename())) continue;
            fs::path safeDir;
            if (!directoryInside(root, dir, safeDir)) continue;

            Item item;
            item.title = dir.filename().string();
            item.path = safeDir.string();

            if (category == Category::Music) {
                fs::path art = findArtwork(dir, {"cover.jpg", "cover.png"});
                fs::path safeArt;
                if (art.empty() || !regularInside(root, art, safeArt)) continue;
                auto tracks = recursiveFiles(root, safeDir,
                    {".mp3", ".m4a", ".aac", ".flac", ".ogg",
                     ".opus", ".wav", ".wma"});
                if (tracks.empty()) continue;
                item.artwork = safeArt.string();
                for (const auto& track : tracks)
                    item.files.push_back(track.string());
            } else {
                fs::path art = artworkForShow(safeDir);
                fs::path safeArt;
                if (art.empty() || !regularInside(root, art, safeArt)) continue;
                auto episodes = recursiveFiles(root, safeDir,
                    {".mp4", ".mkv", ".avi", ".mov", ".m4v", ".webm"});
                if (episodes.empty()) continue;
                item.artwork = safeArt.string();
                for (const auto& episode : episodes)
                    item.files.push_back(episode.string());
            }
            items.push_back(std::move(item));
        }
    }

    std::sort(items.begin(), items.end(),
        [](const Item& a, const Item& b) {
            return naturalLess(a.title, b.title);
        });
    return items;
}

bool loadSlots(const fs::path& path, Slots& slots) {
    std::ifstream in(path);
    if (!in) return false;
    std::string magic;
    if (!std::getline(in, magic) || magic != "KIDDOS_SLOTS_V1") return false;

    Slots loaded{};
    for (auto& category : loaded) {
        for (auto& slot : category) {
            if (!(in >> std::quoted(slot))) return false;
            if (!slot.empty() && !validRelativeReference(slot)) return false;
        }
    }
    std::string extra;
    if (in >> extra) return false;
    slots = std::move(loaded);
    return true;
}

bool saveSlots(const fs::path& path, const Slots& slots) {
    for (const auto& category : slots)
        for (const auto& slot : category)
            if (!slot.empty() && !validRelativeReference(slot)) return false;

    std::error_code ec;
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path(), ec);
    if (ec) return false;

    fs::path temp = path;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) return false;
        out << "KIDDOS_SLOTS_V1\n";
        for (const auto& category : slots)
            for (const auto& slot : category)
                out << std::quoted(slot) << '\n';
        if (!out) {
            out.close();
            fs::remove(temp, ec);
            return false;
        }
    }
    fs::rename(temp, path, ec);
    if (ec) {
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

bool assign(Slots& slots, Category category, std::size_t slot,
            const std::string& relativePath) {
    const auto c = static_cast<std::size_t>(category);
    if (c >= slots.size() || slot >= 4 ||
        !validRelativeReference(relativePath)) return false;
    for (auto& existing : slots[c])
        if (existing == relativePath) existing.clear();
    slots[c][slot] = relativePath;
    return true;
}

bool clear(Slots& slots, Category category, std::size_t slot) {
    const auto c = static_cast<std::size_t>(category);
    if (c >= slots.size() || slot >= 4) return false;
    slots[c][slot].clear();
    return true;
}

std::array<const Item*, 4> resolveSlots(
    const fs::path& romRoot, const Slots& slots, Category category,
    const std::vector<Item>& items) {
    std::array<const Item*, 4> resolved{};
    const auto c = static_cast<std::size_t>(category);
    if (c >= slots.size()) return resolved;

    for (std::size_t slot = 0; slot < resolved.size(); ++slot) {
        if (slots[c][slot].empty()) continue;

        for (const auto& item : items) {
            std::error_code ec;
            const auto relative = fs::relative(item.path, romRoot, ec);
            if (!ec && !relative.empty() &&
                relative.generic_string() == slots[c][slot]) {
                resolved[slot] = &item;
                break;
            }
        }
    }
    return resolved;
}

} // namespace KiddosLibrary
