// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "ParentCenter.hpp"

#include <algorithm>
#include <system_error>

ParentCenterController::ParentCenterController() = default;

bool ParentCenterController::initialize() {
    std::error_code error;
    if (std::filesystem::exists(slotsPath_, error)) {
        ready_ = !error && KiddosLibrary::loadSlots(slotsPath_, slots_);
    } else if (!error) {
        ready_ = KiddosLibrary::saveSlots(slotsPath_, slots_);
    } else {
        ready_ = false;
    }
    return ready_;
}

void ParentCenterController::openCategory(KiddosLibrary::Category category) {
    category_ = category;
    slotSelection_ = 0;
    items_ = KiddosLibrary::discover(romRoot_, category_);
    librarySelection_ = 0;
    libraryScroll_ = 0;
}

void ParentCenterController::openLibrary() {
    librarySelection_ = 0;
    libraryScroll_ = 0;
}

void ParentCenterController::moveSlot(int delta) {
    slotSelection_ = (slotSelection_ + delta + 4) % 4;
}

void ParentCenterController::moveLibrary(int delta) {
    const int total = static_cast<int>(items_.size()) + 1;
    librarySelection_ = std::clamp(librarySelection_ + delta, 0, total - 1);

    if (librarySelection_ < libraryScroll_)
        libraryScroll_ = librarySelection_;
    if (librarySelection_ >= libraryScroll_ + 8)
        libraryScroll_ = librarySelection_ - 7;
}

bool ParentCenterController::assignSelected() {
    if (!ready_ || librarySelection_ <= 0)
        return false;

    const auto index = static_cast<std::size_t>(librarySelection_ - 1);
    if (index >= items_.size())
        return false;

    return KiddosLibrary::assign(
        slots_, category_, static_cast<std::size_t>(slotSelection_),
        itemReference(items_[index])) && persistSlots();
}

bool ParentCenterController::clearSelectedSlot() {
    if (!ready_)
        return false;

    return KiddosLibrary::clear(
        slots_, category_, static_cast<std::size_t>(slotSelection_)) &&
        persistSlots();
}

const KiddosLibrary::Slots& ParentCenterController::slots() const { return slots_; }
const std::vector<KiddosLibrary::Item>& ParentCenterController::items() const { return items_; }
KiddosLibrary::Category ParentCenterController::category() const { return category_; }
int ParentCenterController::slotSelection() const { return slotSelection_; }
int ParentCenterController::librarySelection() const { return librarySelection_; }
int ParentCenterController::libraryScroll() const { return libraryScroll_; }
bool ParentCenterController::ready() const { return ready_; }

const char* ParentCenterController::categoryName() const {
    switch (category_) {
        case KiddosLibrary::Category::Games: return "GAMES";
        case KiddosLibrary::Category::Movies: return "MOVIES";
        case KiddosLibrary::Category::Music: return "MUSIC PLAYLISTS";
        case KiddosLibrary::Category::TVShows: return "TV SHOWS";
    }
    return "LIBRARY";
}

std::string ParentCenterController::itemReference(
    const KiddosLibrary::Item& item) const {
    return std::filesystem::path(item.path)
        .lexically_relative(romRoot_).generic_string();
}

bool ParentCenterController::persistSlots() {
    return ready_ && KiddosLibrary::saveSlots(slotsPath_, slots_);
}
