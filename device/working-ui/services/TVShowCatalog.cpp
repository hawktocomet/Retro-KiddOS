// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "TVShowCatalog.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace KiddosServices {
namespace fs = std::filesystem;
namespace {

std::vector<TVShow> catalog;

bool isVideo(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".mp4" || ext == ".mkv" || ext == ".avi" ||
           ext == ".mov" || ext == ".m4v" || ext == ".webm";
}

bool naturalLess(const std::string& a, const std::string& b) {
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[j]);
        if (std::isdigit(ca) && std::isdigit(cb)) {
            std::size_t ia = i, jb = j;
            while (ia < a.size() && a[ia] == '0') ++ia;
            while (jb < b.size() && b[jb] == '0') ++jb;
            std::size_t ea = ia, eb = jb;
            while (ea < a.size() && std::isdigit(
                       static_cast<unsigned char>(a[ea]))) ++ea;
            while (eb < b.size() && std::isdigit(
                       static_cast<unsigned char>(b[eb]))) ++eb;
            const std::size_t la = ea - ia, lb = eb - jb;
            if (la != lb) return la < lb;
            const int cmp = a.compare(ia, la, b, jb, lb);
            if (cmp != 0) return cmp < 0;
            i = ea;
            j = eb;
            continue;
        }
        const int xa = std::tolower(ca);
        const int xb = std::tolower(cb);
        if (xa != xb) return xa < xb;
        ++i;
        ++j;
    }
    return a.size() < b.size();
}

std::string findArtwork(const fs::path& dir) {
    const char* names[] = {
        "poster.jpg", "poster.png", "cover.jpg", "cover.png"
    };
    std::error_code ec;
    for (const char* name : names) {
        const fs::path candidate = dir / name;
        if (fs::is_regular_file(candidate, ec) && !ec)
            return candidate.string();
        ec.clear();
    }
    return {};
}

bool scanShow(const fs::path& dir, TVShow& show) {
    show.title = dir.filename().string();
    show.directory = dir.string();
    show.artwork = findArtwork(dir);

    std::error_code ec;
    fs::recursive_directory_iterator it(
        dir, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    if (ec) return false;

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code typeError;
        if (it->is_regular_file(typeError) && !typeError &&
            isVideo(it->path()))
            show.episodes.push_back(it->path().string());
    }

    std::sort(show.episodes.begin(), show.episodes.end(), naturalLess);
    return true;
}

} // namespace

bool loadTVShows() {
    const fs::path root("/storage/roms/tvshows");
    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec) {
        catalog.clear();
        std::fprintf(stderr, "KiddOS: TV show directory unavailable: %s\n",
                     root.c_str());
        return false;
    }

    std::vector<TVShow> next;
    fs::directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    if (ec) return false;

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        std::error_code typeError;
        if (!it->is_directory(typeError) || typeError)
            continue;

        TVShow show;
        if (scanShow(it->path(), show) && !show.episodes.empty())
            next.push_back(std::move(show));
    }

    std::sort(next.begin(), next.end(),
        [](const TVShow& a, const TVShow& b) {
            return naturalLess(a.title, b.title);
        });
    catalog = std::move(next);
    std::printf("KiddOS: loaded %zu TV show(s)\n", catalog.size());
    return true;
}

const std::vector<TVShow>& tvShows() {
    return catalog;
}

} // namespace KiddosServices
