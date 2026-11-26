// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "LibraryGrid.hpp"

namespace KiddosUI {

enum class LibrarySection {
    Games,
    Music,
    Movies
};

// Category navigation only. Each section will own its own content model.
// No shared or generalized media-item type is introduced here.
class LibraryNavigator {
public:
    explicit LibraryNavigator(int initialItemCount = 4)
        : grid_(initialItemCount) {}

    LibrarySection section() const { return section_; }
    LibraryGrid& grid() { return grid_; }
    const LibraryGrid& grid() const { return grid_; }

    void previousSection() {
        switch (section_) {
            case LibrarySection::Games: section_ = LibrarySection::Movies; break;
            case LibrarySection::Music: section_ = LibrarySection::Movies; break;
            case LibrarySection::Movies: section_ = LibrarySection::Games; break;
        }
        grid_.select(0);
    }

    void nextSection() {
        switch (section_) {
            case LibrarySection::Games: section_ = LibrarySection::Movies; break;
            case LibrarySection::Music: section_ = LibrarySection::Games; break;
            case LibrarySection::Movies: section_ = LibrarySection::Games; break;
        }
        grid_.select(0);
    }

private:
    LibrarySection section_ = LibrarySection::Games;
    LibraryGrid grid_;
};

} // namespace KiddosUI
