// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

#include "../services/LibraryData.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class ParentCenterController {
public:
    ParentCenterController();

    bool initialize();

    void openCategory(KiddosLibrary::Category category);
    void openLibrary();

    void moveSlot(int delta);
    void moveLibrary(int delta);

    bool assignSelected();
    bool clearSelectedSlot();

    const KiddosLibrary::Slots& slots() const;
    const std::vector<KiddosLibrary::Item>& items() const;
    KiddosLibrary::Category category() const;
    int slotSelection() const;
    int librarySelection() const;
    int libraryScroll() const;
    bool ready() const;

    const char* categoryName() const;
    std::string itemReference(const KiddosLibrary::Item& item) const;

private:
    KiddosLibrary::Slots slots_{};
    const std::filesystem::path slotsPath_ =
        "/storage/.config/kiddos/slots.conf";
    const std::filesystem::path romRoot_ = "/storage/roms";

    bool ready_ = false;
    KiddosLibrary::Category category_ = KiddosLibrary::Category::Games;
    int slotSelection_ = 0;
    int librarySelection_ = 0;
    int libraryScroll_ = 0;
    std::vector<KiddosLibrary::Item> items_;

    bool persistSlots();
};
