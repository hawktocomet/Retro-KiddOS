// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet
#include "../ui/UI.hpp"
#include "../services/MusicCatalog.hpp"
#include "../services/MusicLauncher.hpp"
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
    const auto& playlists = KiddosServices::musicPlaylists();
    const int count = static_cast<int>(playlists.size());
    const SDL_Color muted = {155,177,215,255};
    for (int i=0; i<4; ++i) {
        SDL_Rect tile = tiles[i];
        SDL_SetRenderDrawColor(r,28,38,68,255);
        SDL_RenderFillRect(r,&tile);
        const bool occupied = i < count &&
            !playlists[static_cast<std::size_t>(i)].tracks.empty();
        if (occupied && ui.musicImages[i]) {
            int iw = 0, ih = 0;
            if (SDL_QueryTexture(ui.musicImages[i], nullptr, nullptr,
                                 &iw, &ih) == 0 && iw > 0 && ih > 0) {
                const float scale = std::min(112.0f / iw, 112.0f / ih);
                SDL_Rect image = {
                    tile.x + (tile.w - static_cast<int>(iw * scale)) / 2,
                    tile.y + 8,
                    static_cast<int>(iw * scale),
                    static_cast<int>(ih * scale)
                };
                SDL_RenderCopy(r, ui.musicImages[i], nullptr, &image);
            }
        }
        std::string label = occupied
            ? playlists[static_cast<std::size_t>(i)].title : "EMPTY SLOT";
        if (label.size() > 25) label = label.substr(0,22) + "...";
        drawWifiLine(r, ui.labelFont, label, tile.x+18, tile.y+145, muted);
        if (i == ui.mediaBrowser->music.selectedRow && occupied) {
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


void drawMusicPlaylist(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawText(r, ui.musicSubtitle, 360, 119);
    drawBattery(ui);

    const auto& playlists = KiddosServices::musicPlaylists();
    const int playlistIndex = ui.mediaBrowser->musicPlaylistIndex;
    const std::vector<MusicTrack> emptyTracks;
    const auto& tracks = playlistIndex >= 0 &&
        playlistIndex < static_cast<int>(playlists.size())
        ? playlists[static_cast<std::size_t>(playlistIndex)].tracks
        : emptyTracks;
    const int first = std::max(0, ui.mediaBrowser->music.scrollOffset);
    constexpr int visibleRows = 8;
    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};

    for (int row = 0; row < visibleRows; ++row) {
        const int index = first + row;
        if (index >= static_cast<int>(tracks.size())) break;
        SDL_Rect item = {76, 158 + row * 54, 568, 46};
        if (index == ui.mediaBrowser->music.selectedRow) {
            SDL_SetRenderDrawColor(r, 35, 91, 139, 255);
            SDL_RenderFillRect(r, &item);
            SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
            SDL_RenderDrawRect(r, &item);
        }
        drawWifiLine(r, ui.labelFont,
                     tracks[static_cast<std::size_t>(index)].title,
                     94, item.y + 10,
                     index == ui.mediaBrowser->music.selectedRow ? white : muted);
    }

    drawText(r, ui.musicFooter, 360, 625);
    SDL_RenderPresent(r);
}


void drawNowPlaying(UI& ui) {
    SDL_Renderer* r = ui.renderer;
    drawSimplePanel(r);
    drawText(r, ui.heading, 360, 68);
    drawBattery(ui);

    const SDL_Color white = {245, 248, 255, 255};
    const SDL_Color muted = {155, 177, 215, 255};
    const SDL_Rect artwork = {225, 125, 270, 270};

    if (ui.musicArtwork) {
        SDL_RenderCopy(r, ui.musicArtwork, nullptr, &artwork);
    } else {
        SDL_Rect fallback = {225, 125, 270, 270};
        SDL_SetRenderDrawColor(r, 28, 38, 68, 255);
        SDL_RenderFillRect(r, &fallback);
        SDL_SetRenderDrawColor(r, 80, 190, 255, 255);
        SDL_Rect stem = {340, 185, 12, 115};
        SDL_RenderFillRect(r, &stem);
        SDL_Rect flag = {350, 185, 62, 12};
        SDL_RenderFillRect(r, &flag);
        SDL_Rect note = {305, 280, 48, 30};
        SDL_RenderFillRect(r, &note);
        SDL_Rect note2 = {350, 280, 48, 30};
        SDL_RenderFillRect(r, &note2);
    }

    Text nowPlaying = makeText(r, ui.labelFont, "NOW PLAYING", white);
    const bool paused = KiddosServices::musicPaused();
    Text indicator = makeText(
        r, ui.footerFont, paused ? "Ⅱ  PAUSED" : "♪  PLAYING",
        paused ? SDL_Color{255, 210, 110, 255}
               : SDL_Color{110, 230, 160, 255});
    drawText(r, nowPlaying, 360, 420);
    drawText(r, ui.musicNowPlayingTitle, 360, 465);
    drawText(r, indicator, 360, 520);

    Text footer = makeText(r, ui.footerFont,
        "A  PAUSE/RESUME     B  BACK     L/R  CHANGE SONG", muted);
    drawText(r, footer, 360, 625);
    destroyText(nowPlaying);
    destroyText(indicator);
    destroyText(footer);
    SDL_RenderPresent(r);
}
