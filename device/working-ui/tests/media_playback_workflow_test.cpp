// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "../services/MediaPlaybackWorkflow.hpp"
#include "../services/MovieCatalog.hpp"
#include "../services/MovieLauncher.hpp"
#include "../services/TVShowCatalog.hpp"
#include "../services/TVShowLauncher.hpp"

#include <cstdio>
#include <vector>

namespace {
std::vector<Movie> movieItems;
std::vector<TVShow> showItems;
int movieLaunches = 0;
int showLaunches = 0;
bool movieLaunchSucceeds = true;
bool showLaunchSucceeds = true;
int failures = 0;

void check(bool condition, const char* description) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++failures;
}

void setupCatalogs() {
    movieItems = {
        {"Movie 1", "/movies/1.mp4", ""},
        {"Movie 2", "/movies/2.mp4", ""},
        {"Movie 3", "/movies/3.mp4", ""},
        {"Movie 4", "/movies/4.mp4", ""},
        {"Movie 5", "/movies/5.mp4", ""}
    };

    showItems = {
        {"Show 1", "/shows/1", "", {}},
        {"Show 2", "/shows/2", "", {}},
        {"Show 3", "/shows/3", "", {}},
        {"Show 4", "/shows/4", "", {}},
        {"Show 5", "/shows/5", "", {}}
    };
}
}

namespace KiddosServices {
bool loadMovies() { return true; }
const std::vector<Movie>& movies() { return movieItems; }
bool launchMovie(const Movie&) {
    ++movieLaunches;
    return movieLaunchSucceeds;
}

bool loadTVShows() { return true; }
const std::vector<TVShow>& tvShows() { return showItems; }
bool launchTVShow(const TVShow&) {
    ++showLaunches;
    return showLaunchSucceeds;
}
}

int main() {
    using KiddosServices::MediaPlaybackWorkflow;

    setupCatalogs();
    MediaPlaybackWorkflow workflow;

    check(workflow.moveMovieSelection(3, -2) == 1,
          "movie Up movement changes selection by two positions once");
    check(workflow.moveMovieSelection(0, -2) == 0,
          "movie selection clamps at first item");
    check(workflow.moveMovieSelection(1, 2) == 3,
          "movie Down movement changes selection by two positions");
    check(workflow.moveMovieSelection(3, 2) == 3,
          "movie selection clamps at fourth visible slot");

    check(workflow.moveTVSelection(3, -2) == 1,
          "TV Up movement changes selection by two positions");
    check(workflow.moveTVSelection(0, -2) == 0,
          "TV selection clamps at first item");
    check(workflow.moveTVSelection(1, 2) == 3,
          "TV Down movement changes selection by two positions");
    check(workflow.moveTVSelection(3, 2) == 3,
          "TV selection clamps at fourth visible slot");

    check(workflow.hasMovie(4),
          "movie validation retains full catalog range");
    check(!workflow.hasMovie(-1) && !workflow.hasMovie(5),
          "movie validation rejects out-of-range indexes");
    check(workflow.hasTVShow(3),
          "TV validation accepts fourth visible slot");
    check(!workflow.hasTVShow(4) && !workflow.hasTVShow(-1),
          "TV validation rejects entries beyond four visible slots");

    movieLaunches = 0;
    check(workflow.launchMovie(0) && movieLaunches == 1,
          "valid movie selection launches through existing launcher");
    check(!workflow.launchMovie(5) && movieLaunches == 1,
          "invalid movie selection does not launch");

    showLaunches = 0;
    check(workflow.launchTVShow(3) && showLaunches == 1,
          "valid TV selection launches through existing launcher");
    check(!workflow.launchTVShow(4) && showLaunches == 1,
          "TV selection beyond four slots does not launch");

    movieLaunchSucceeds = false;
    showLaunchSucceeds = false;
    check(!workflow.launchMovie(0),
          "movie launcher failure is reported");
    check(!workflow.launchTVShow(0),
          "TV launcher failure is reported");

    movieItems.clear();
    showItems.clear();
    check(workflow.moveMovieSelection(2, -2) == 0 &&
          workflow.moveTVSelection(2, -2) == 0,
          "empty catalogs reset selection to zero");

    return failures == 0 ? 0 : 1;
}
