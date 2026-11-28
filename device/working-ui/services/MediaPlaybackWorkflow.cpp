// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "MediaPlaybackWorkflow.hpp"

#include "MovieCatalog.hpp"
#include "MovieLauncher.hpp"
#include "TVShowCatalog.hpp"
#include "TVShowLauncher.hpp"

#include <algorithm>
#include <cstddef>

namespace KiddosServices {
namespace {

int moveSelection(int selectedIndex, int delta, std::size_t itemCount) {
    const int count = std::min(4, static_cast<int>(itemCount));
    if (count <= 0)
        return 0;

    return std::clamp(selectedIndex + delta, 0, count - 1);
}

} // namespace

int MediaPlaybackWorkflow::moveMovieSelection(
    int selectedIndex, int delta) const {
    return moveSelection(selectedIndex, delta, movies().size());
}

int MediaPlaybackWorkflow::moveTVSelection(
    int selectedIndex, int delta) const {
    return moveSelection(selectedIndex, delta, tvShows().size());
}

bool MediaPlaybackWorkflow::hasMovie(int selectedIndex) const {
    return selectedIndex >= 0 &&
        selectedIndex < static_cast<int>(movies().size());
}

bool MediaPlaybackWorkflow::hasTVShow(int selectedIndex) const {
    return selectedIndex >= 0 &&
        selectedIndex < std::min(4, static_cast<int>(tvShows().size()));
}

bool MediaPlaybackWorkflow::launchMovie(int selectedIndex) const {
    if (!hasMovie(selectedIndex))
        return false;

    return KiddosServices::launchMovie(
        movies()[static_cast<std::size_t>(selectedIndex)]);
}

bool MediaPlaybackWorkflow::launchTVShow(int selectedIndex) const {
    if (!hasTVShow(selectedIndex))
        return false;

    return KiddosServices::launchTVShow(
        tvShows()[static_cast<std::size_t>(selectedIndex)]);
}

} // namespace KiddosServices
