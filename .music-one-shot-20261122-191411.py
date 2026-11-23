from pathlib import Path
import sys

root = Path.cwd()
src = root / "device/working-ui"

def edit(rel, replacements):
    p = src / rel
    s = p.read_text()
    for old, new in replacements:
        if old not in s:
            raise SystemExit(f"PATCH ABORTED: expected text not found in {rel}: {old[:100]!r}")
        s = s.replace(old, new, 1)
    p.write_text(s)
    print(f"Updated {rel}")

# Music model.
edit("models/Models.hpp", [
    ("struct Movie {\n    std::string title;\n    std::string path;\n};",
     "struct Movie {\n    std::string title;\n    std::string path;\n};\n\nstruct MusicTrack {\n    std::string title;\n    std::string path;\n};")
])

# Music browser state.
edit("screens/ScreenState.hpp", [
    ("struct MovieBrowserState {\n    int selectedRow = 0;\n    int scrollOffset = 0;\n};",
     "struct MovieBrowserState {\n    int selectedRow = 0;\n    int scrollOffset = 0;\n};\n\nstruct MusicBrowserState {\n    int selectedRow = 0;\n    int scrollOffset = 0;\n};")
])

# Catalog: use the same robust fork/pipe/NUL-delimited pattern as MovieCatalog.
(src / "services/MusicCatalog.hpp").write_text("""// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once
#include "../models/Models.hpp"
#include <vector>
namespace KiddosServices {
bool loadMusic();
const std::vector<MusicTrack>& music();
}
""")
(src / "services/MusicCatalog.cpp").write_text(r'''// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "MusicCatalog.hpp"
#include <cerrno>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace KiddosServices {
namespace {
std::vector<MusicTrack> trackList;
constexpr const char* kScript = R"PY(
from pathlib import Path
import sys
directory = Path("/storage/roms/music")
extensions = {".mp3", ".m4a", ".aac", ".flac", ".ogg", ".opus", ".wav", ".wma"}
paths = sorted((p for p in directory.iterdir()
                if p.is_file() and p.suffix.lower() in extensions),
               key=lambda p: p.name.casefold()) if directory.is_dir() else []
out = sys.stdout.buffer
for path in paths[:4]:
    out.write(path.stem.encode("utf-8") + b"\0")
    out.write(str(path).encode("utf-8") + b"\0")
)PY";
}
bool loadMusic() {
    int fds[2];
    if (pipe(fds) != 0) { std::perror("KiddOS: music pipe"); return false; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0) _exit(126);
        close(fds[1]);
        execlp("python3", "python3", "-c", kScript, static_cast<char*>(nullptr));
        _exit(127);
    }
    close(fds[1]);
    if (pid < 0) { close(fds[0]); std::perror("KiddOS: music fork"); return false; }
    std::vector<char> output;
    char buffer[1024];
    bool ok = true;
    for (;;) {
        ssize_t n = read(fds[0], buffer, sizeof(buffer));
        if (n > 0) output.insert(output.end(), buffer, buffer+n);
        else if (n == 0) break;
        else if (errno != EINTR) { ok = false; break; }
    }
    close(fds[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        return false;
    }
    if (!ok || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return false;
    std::vector<std::string> fields;
    std::string field;
    for (char c : output) {
        if (c == '\0') { fields.push_back(field); field.clear(); }
        else field.push_back(c);
    }
    if (!field.empty() || fields.size() % 2) return false;
    std::vector<MusicTrack> next;
    for (std::size_t i=0; i<fields.size(); i+=2) {
        if (fields[i].empty() || fields[i+1].empty()) return false;
        next.push_back({std::move(fields[i]), std::move(fields[i+1])});
    }
    trackList = std::move(next);
    std::printf("KiddOS: loaded %zu music tracks\n", trackList.size());
    std::fflush(stdout);
    return true;
}
const std::vector<MusicTrack>& music() { return trackList; }
}
''')

# Audio-only MPV launcher. The existing MPV input config provides A pause/B quit.
(src / "services/MusicLauncher.hpp").write_text("""// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#pragma once
#include "../models/Models.hpp"
namespace KiddosServices {
bool launchMusic(const MusicTrack& track);
}
""")
(src / "services/MusicLauncher.cpp").write_text(r'''// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "MusicLauncher.hpp"
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
namespace KiddosServices {
bool launchMusic(const MusicTrack& track) {
    if (track.path.empty() || access(track.path.c_str(), R_OK) != 0) {
        std::fprintf(stderr, "KiddOS: music file missing or unreadable: %s\n", track.path.c_str());
        return false;
    }
    std::printf("KiddOS: playing music %s\n", track.title.c_str());
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/mpv", "mpv", "--no-config",
              "--no-video", "--ao=alsa", "--audio-device=alsa/hw:0,0",
              "--audio-channels=stereo", "--input-gamepad=yes",
              "--input-conf=/storage/.config/kiddos/mpv-input.conf",
              track.path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    if (pid < 0) { std::perror("KiddOS: music fork"); return false; }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        std::perror("KiddOS: music wait");
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
}
'''.replace('#include <cstdio>', '#include <cstdio>\n#include <cerrno>'))

# Music screen: four simple title tiles; no artwork or extra management UI.
(src / "screens/MusicScreens.cpp").write_text(r'''// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "../UI.hpp"
#include "../services/MusicCatalog.hpp"
#include <algorithm>
#include <string>

void drawMusic(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.musicSubtitle, 360, 119);
    drawBattery(ui);
    const SDL_Rect tiles[4] = {
        {88,174,252,190}, {380,174,252,190},
        {88,396,252,190}, {380,396,252,190}
    };
    const auto& tracks = KiddosServices::music();
    const int count = std::min(4, static_cast<int>(tracks.size()));
    const SDL_Color muted = {155,177,215,255};
    for (int i=0; i<4; ++i) {
        SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r,28,38,68,255);
        SDL_RenderFillRect(r,&tile);
        std::string label = i < count ? tracks[static_cast<std::size_t>(i)].title : "EMPTY SLOT";
        if (label.size() > 25) label = label.substr(0,22) + "...";
        drawWifiLine(r, ui.labelFont, label, tile.x+18, tile.y+82, muted);
        if (i == ui.musicBrowser.selectedRow && i < count) {
            SDL_SetRenderDrawColor(r,80,190,255,255);
            for (int n=0; n<5; ++n) {
                SDL_Rect outline={tile.x-n,tile.y-n,tile.w+n*2,tile.h+n*2};
                SDL_RenderDrawRect(r,&outline);
            }
        }
    }
    drawText(r, ui.musicFooter, 360, 625);
    SDL_RenderPresent(r);
}
''')

# Navigation enum.
edit("Navigation.hpp", [
    ("    Games,\n    Movies,", "    Games,\n    Music,\n    Movies,")
])

# UI resource fields/prototypes.
edit("UI.hpp", [
    ("    Text homeSubtitle, movieSubtitle, movieFooter;",
     "    Text homeSubtitle, movieSubtitle, movieFooter;\n    Text musicSubtitle, musicFooter;"),
    ("    KiddosScreens::MovieBrowserState movieBrowser;",
     "    KiddosScreens::MovieBrowserState movieBrowser;\n    KiddosScreens::MusicBrowserState musicBrowser;"),
    ("void drawMovies(UI& ui);",
     "void drawMovies(UI& ui);\nvoid drawMusic(UI& ui);")
])

# UI resource lifetime, labels and Home status.
edit("UI.cpp", [
    ("    destroyText(ui.movieFooter);",
     "    destroyText(ui.movieFooter);\n    destroyText(ui.musicSubtitle);\n    destroyText(ui.musicFooter);"),
    ('    ui.movieFooter = makeText(ui.renderer, ui.footerFont,\n        "D-PAD MOVE   A PLAY   B BACK   START MENU", muted);',
     '    ui.movieFooter = makeText(ui.renderer, ui.footerFont,\n        "D-PAD MOVE   A PLAY   B BACK   START MENU", muted);\n    ui.musicSubtitle = makeText(ui.renderer, ui.footerFont, "CHOOSE A SONG", muted);\n    ui.musicFooter = makeText(ui.renderer, ui.footerFont,\n        "D-PAD MOVE   A PLAY   B BACK   START MENU", muted);'),
    ('    ui.homeComingSoon[0] = makeText(ui.renderer, ui.footerFont,\n        "COMING SOON", muted);',
     '    ui.homeComingSoon[0] = makeText(ui.renderer, ui.footerFont,\n        "", muted);')
])

# Application wiring.
edit("Application.cpp", [
    ('#include "services/MovieLauncher.hpp"',
     '#include "services/MovieLauncher.hpp"\n#include "services/MusicCatalog.hpp"\n#include "services/MusicLauncher.hpp"'),
    ('        if (!KiddosServices::loadMovies()) {',
     '        if (!KiddosServices::loadMusic()) {\n            std::fprintf(stderr, "KiddOS: music catalog unavailable; continuing without music.\\\\n");\n        }\n        if (!KiddosServices::loadMovies()) {'),
    ('        ui.movieBrowser = movieBrowser_;',
     '        ui.movieBrowser = movieBrowser_;\n        ui.musicBrowser = musicBrowser_;'),
    ('        auto moveMovieSelection = [&](int delta) {',
     '        auto moveMusicSelection = [&](int delta) {\n            const int count = std::min(4, static_cast<int>(KiddosServices::music().size()));\n            if (count == 0) { ui.musicBrowser.selectedRow = 0; return; }\n            ui.musicBrowser.selectedRow = std::clamp(ui.musicBrowser.selectedRow + delta, 0, count - 1);\n        };\n\n        auto moveMovieSelection = [&](int delta) {'),
    ('        bool launchMovie = false;\n        while (running && !launch && !launchMovie) {',
     '        bool launchMovie = false;\n        bool launchMusic = false;\n        while (running && !launch && !launchMovie && !launchMusic) {'),
    ('                            if (homeSelected == 0) screen = Games;\n                            else if (homeSelected == 2) screen = Movies;',
     '                            if (homeSelected == 0) screen = Games;\n                            else if (homeSelected == 1) screen = Music;\n                            else if (homeSelected == 2) screen = Movies;'),
    ('                    } else if (screen == Movies) {\n                        if (key == SDLK_UP)',
     '                    } else if (screen == Music) {\n                        if (key == SDLK_UP) moveMusicSelection(-2);\n                        else if (key == SDLK_DOWN) moveMusicSelection(2);\n                        else if (key == SDLK_LEFT) moveMusicSelection(-1);\n                        else if (key == SDLK_RIGHT) moveMusicSelection(1);\n                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {\n                            const auto& list = KiddosServices::music();\n                            if (ui.musicBrowser.selectedRow >= 0 && ui.musicBrowser.selectedRow < static_cast<int>(list.size())) launchMusic = true;\n                        } else if (key == SDLK_s) navigation.openSystemMenu();\n                    } else if (screen == Movies) {\n                        if (key == SDLK_UP)'),
    ('                        else if (screen == Movies || screen == Games)\n                            screen = Home;',
     '                        else if (screen == Movies || screen == Music || screen == Games)\n                            screen = Home;'),
    ('                            if (homeSelected == 0) screen = Games;\n                            else if (homeSelected == 2) screen = Movies;',
     '                            if (homeSelected == 0) screen = Games;\n                            else if (homeSelected == 1) screen = Music;\n                            else if (homeSelected == 2) screen = Movies;'),
    ('                        } else if (screen == Movies) {\n                            const auto& list = KiddosServices::movies();',
     '                        } else if (screen == Music) {\n                            const auto& list = KiddosServices::music();\n                            if (ui.musicBrowser.selectedRow >= 0 && ui.musicBrowser.selectedRow < static_cast<int>(list.size())) launchMusic = true;\n                        } else if (screen == Movies) {\n                            const auto& list = KiddosServices::movies();'),
    ('                        } else if (screen == Movies) {\n                            moveMovieSelection(-2);',
     '                        } else if (screen == Music) {\n                            moveMusicSelection(-2);\n                        } else if (screen == Movies) {\n                            moveMovieSelection(-2);'),
    ('                        } else if (screen == Movies) {\n                            moveMovieSelection(2);',
     '                        } else if (screen == Music) {\n                            moveMusicSelection(2);\n                        } else if (screen == Movies) {\n                            moveMovieSelection(2);'),
    ('                        } else if (screen == Movies) {\n                            moveMovieSelection(-1);',
     '                        } else if (screen == Music) {\n                            moveMusicSelection(-1);\n                        } else if (screen == Movies) {\n                            moveMovieSelection(-1);'),
    ('                        } else if (screen == Movies) {\n                            moveMovieSelection(1);',
     '                        } else if (screen == Music) {\n                            moveMusicSelection(1);\n                        } else if (screen == Movies) {\n                            moveMovieSelection(1);'),
    ('                                screen == Movies || screen == SelectedGame ||',
     '                                screen == Movies || screen == Music || screen == SelectedGame ||'),
    ('            if (running && !launch && !launchMovie) {',
     '            if (running && !launch && !launchMovie && !launchMusic) {'),
    ('                else if (screen == Movies) drawMovies(ui);',
     '                else if (screen == Movies) drawMovies(ui);\n                else if (screen == Music) drawMusic(ui);'),
    ('        movieBrowser_ = ui.movieBrowser;',
     '        movieBrowser_ = ui.movieBrowser;\n        musicBrowser_ = ui.musicBrowser;'),
    ('        if (running && launchMovie) {',
     '        if (running && launchMusic) {\n            const auto& list = KiddosServices::music();\n            if (musicBrowser_.selectedRow >= 0 && musicBrowser_.selectedRow < static_cast<int>(list.size())) {\n                if (!KiddosServices::launchMusic(list[static_cast<std::size_t>(musicBrowser_.selectedRow)])) SDL_Delay(1500);\n            }\n            screen = Music;\n        }\n        if (running && launchMovie) {')
])

# Persist Music browser state.
edit("Application.hpp", [
    ("    KiddosScreens::MovieBrowserState movieBrowser_;",
     "    KiddosScreens::MovieBrowserState movieBrowser_;\n    KiddosScreens::MusicBrowserState musicBrowser_;")
])

# Enable Home tile and include Music files in both build systems.
edit("CMakeLists.txt", [
    ("    screens/MovieScreens.cpp", "    screens/MovieScreens.cpp\n    screens/MusicScreens.cpp"),
    ("    services/MovieLauncher.cpp", "    services/MovieLauncher.cpp\n    services/MusicCatalog.cpp\n    services/MusicLauncher.cpp")
])
edit("build-kiddos-ui.sh", [
    ('        "$SCRIPT_DIR/screens/MovieScreens.cpp"',
     '        "$SCRIPT_DIR/screens/MovieScreens.cpp"\n        "$SCRIPT_DIR/screens/MusicScreens.cpp"'),
    ('        "$SCRIPT_DIR/services/MovieLauncher.cpp"',
     '        "$SCRIPT_DIR/services/MovieLauncher.cpp"\n        "$SCRIPT_DIR/services/MusicCatalog.cpp"\n        "$SCRIPT_DIR/services/MusicLauncher.cpp"')
])

# Keep docs factual and update scope.
p = src / "README.md"
s = p.read_text()
s = s.replace("- Movie playback additionally requires `/usr/bin/mpv`, DRM video output, and the device ALSA audio device `alsa/hw:0,0`.",
              "- Movie playback requires `/usr/bin/mpv`, DRM video output, and ALSA audio. Music playback uses MPV audio-only output and the same ALSA device.")
s = s.replace("Games and Movies are implemented library sections.", "Games, Music, and Movies are implemented library sections.")
s = s.replace("Music remains a\nprepared navigation extension point.", "Music scans `/storage/roms/music` for supported audio files and presents up to four tracks.")
s += """
## Music carousel

The Music screen scans `/storage/roms/music` for MP3, M4A, AAC, FLAC,
OGG, OPUS, WAV, and WMA files. It displays up to four tracks in filename
order. D-pad navigation selects a track; A starts playback; B exits playback
to the Music carousel. MPV runs audio-only through the device ALSA output
and uses the existing A pause/B quit controller bindings.
"""
p.write_text(s)
print("Music source changes complete.")
