// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include <algorithm>

namespace KiddosUI {

// Selection behavior for the existing two-column, four-slot library grid.
// This models navigation only; game/music/movie data remains category-owned.
class LibraryGrid {
public:
    explicit LibraryGrid(int itemCount = 4) : itemCount_(std::max(0, itemCount)) {}

    int selected() const { return selected_; }
    int& selectedRef() { return selected_; }
    int itemCount() const { return itemCount_; }

    void setItemCount(int count) {
        itemCount_ = std::max(0, count);
        selected_ = itemCount_ == 0 ? 0 : std::min(selected_, itemCount_ - 1);
    }

    void select(int index) {
        if (itemCount_ == 0) {
            selected_ = 0;
            return;
        }
        selected_ = std::max(0, std::min(index, itemCount_ - 1));
    }

    void moveUp() { if (selected_ >= 2) selected_ -= 2; }
    void moveDown() { if (selected_ + 2 < itemCount_) selected_ += 2; }
    void moveLeft() { if (selected_ % 2 != 0) --selected_; }
    void moveRight() { if (selected_ % 2 == 0 && selected_ + 1 < itemCount_) ++selected_; }

private:
    int itemCount_ = 4;
    int selected_ = 0;
};

} // namespace KiddosUI
