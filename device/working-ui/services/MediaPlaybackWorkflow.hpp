// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#pragma once

namespace KiddosServices {

class MediaPlaybackWorkflow {
public:
    int moveMovieSelection(int selectedIndex, int delta) const;
    int moveTVSelection(int selectedIndex, int delta) const;

    bool hasMovie(int selectedIndex) const;
    bool hasTVShow(int selectedIndex) const;

    bool launchMovie(int selectedIndex) const;
    bool launchTVShow(int selectedIndex) const;
};

} // namespace KiddosServices
