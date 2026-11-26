// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace KiddosLibrary {
namespace fs = std::filesystem;

enum class Category { Games = 0, Movies = 1, Music = 2, TVShows = 3 };

struct Item {
    std::string title;
    std::string path;       // Absolute path resolved from the SD-card root.
    std::string artwork;
    std::vector<std::string> files; // Tracks or episodes, in playback order.
    std::string platform; // Populated for games (gb or gbc).
};

using Slots = std::array<std::array<std::string, 4>, 4>;

std::vector<Item> discover(const fs::path& romRoot, Category category);
bool loadSlots(const fs::path& configPath, Slots& slots);
bool saveSlots(const fs::path& configPath, const Slots& slots);
bool assign(Slots& slots, Category category, std::size_t slot,
            const std::string& relativePath);
bool clear(Slots& slots, Category category, std::size_t slot);

} // namespace KiddosLibrary
