// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "Application.hpp"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>

#include "../ui/UI.hpp"
#include "../security/PinWorkflow.hpp"
#include "../services/GameCatalog.hpp"
#include "../services/GameLauncher.hpp"
#include "../services/MovieCatalog.hpp"
#include "../services/TVShowCatalog.hpp"
#include "../services/TVShowLauncher.hpp"
#include "../services/MovieLauncher.hpp"
#include "../services/MusicCatalog.hpp"
#include "../services/MusicLauncher.hpp"
#include "../services/PowerService.hpp"
#include "../services/LibraryData.hpp"
#include <filesystem>
#include <vector>

Application::Application()
    : pinWorkflow_(std::make_unique<KiddosPin::PinWorkflow>(
          "/storage/.config/kiddos")) {}

Application::~Application() = default;

int Application::run() {
    using namespace KiddosNavigation;

    KiddosUI::LibraryGrid& gameGrid = library_.grid();
    KiddosUI::LibraryGrid& homeGrid = homeGrid_;
    int& homeSelected = homeGrid.selectedRef();
    int& selected = gameGrid.selectedRef();
    int& action = action_;
    int& menuSelected = menuSelected_;
    Navigation& navigation = navigation_;
    Screen& screen = navigation.currentRef();
    KiddosPin::PinWorkflow& pinWorkflow = *pinWorkflow_;
    int& parentSelected = parentSelected_;
    KiddosLibrary::Slots parentSlots{};
    const std::filesystem::path parentSlotsPath = "/storage/.config/kiddos/slots.conf";
    const std::filesystem::path romRoot = "/storage/roms";
    bool parentSlotsReady = false;
    KiddosLibrary::Category parentCategory = KiddosLibrary::Category::Games;
    int parentSlotSelected = 0;
    int parentLibrarySelected = 0;
    int parentLibraryScroll = 0;
    std::vector<KiddosLibrary::Item> parentLibrary;
    bool& running = running_;
    int currentMusicTrack = -1;
    Screen nowPlayingReturnScreen = Home;

    std::error_code slotConfigError;
    if (std::filesystem::exists(parentSlotsPath, slotConfigError)) {
        parentSlotsReady = !slotConfigError &&
            KiddosLibrary::loadSlots(parentSlotsPath, parentSlots);
    } else if (!slotConfigError) {
        parentSlotsReady = KiddosLibrary::saveSlots(parentSlotsPath, parentSlots);
    }
    if (!parentSlotsReady)
        std::fprintf(stderr, "KiddOS: native Parent Center slots unavailable or invalid.\\n");

    if (!KiddosPin::initializePinStore("/storage/.config/kiddos")) {
        std::fprintf(stderr,
                     "KiddOS: PIN store initialization failed; refusing to start.\n");
        return 1;
    }

    while (running) {
        if (!KiddosServices::loadGameSlots()) {
            std::fprintf(stderr,
                         "KiddOS: game catalog load failed; refusing to open carousel.\n");
            return 1;
        }
        if (!KiddosServices::loadMusic()) {
            std::fprintf(stderr, "KiddOS: music catalog unavailable; continuing without music.\\n");
        }
        if (!KiddosServices::loadMovies()) {
            std::fprintf(stderr,
                         "KiddOS: movie catalog unavailable; continuing without movies.\n");
        }
        if (!KiddosServices::loadTVShows()) {
            std::fprintf(stderr,
                         "KiddOS: TV catalog unavailable; continuing without TV shows.\\n");
        }
        UI ui;
        if (!openUI(ui)) return 1;
        ui.mediaBrowser = &mediaBrowser_;
        updateSelectedGameText(ui, selected);

        // Parent Center atomically replaces the native slot configuration.
        constexpr const char* slotsPath =
            "/storage/.config/kiddos/slots.conf";
        struct stat lastSlotsStat {};
        bool haveSlotsStat = stat(slotsPath, &lastSlotsStat) == 0;
        Uint32 lastCatalogPoll = SDL_GetTicks();

        const SDL_Color pinWhite = {245, 248, 255, 255};
        const SDL_Color pinRed = {255, 115, 115, 255};

        auto setPinPrompt = [&](const char* prompt, const char* error) {
            destroyText(ui.pinPrompt);
            destroyText(ui.pinError);
            ui.pinPrompt = makeText(ui.renderer, ui.labelFont,
                                    prompt, pinWhite);
            ui.pinError = makeText(ui.renderer, ui.footerFont,
                                   error, pinRed);
        };

        auto refreshPinPrompt = [&]() {
            setPinPrompt(pinWorkflow.prompt(), pinWorkflow.feedback());
        };

        auto moveMusicSelection = [&](int delta) {
            const int count = screen == Music
                ? static_cast<int>(KiddosServices::musicPlaylists().size())
                : static_cast<int>(KiddosServices::music().size());
            if (count == 0) {
                ui.mediaBrowser->music.selectedRow = 0;
                ui.mediaBrowser->music.scrollOffset = 0;
                return;
            }
            ui.mediaBrowser->music.selectedRow = std::clamp(
                ui.mediaBrowser->music.selectedRow + delta, 0, count - 1);
            constexpr int visibleRows = 8;
            if (ui.mediaBrowser->music.selectedRow < ui.mediaBrowser->music.scrollOffset)
                ui.mediaBrowser->music.scrollOffset = ui.mediaBrowser->music.selectedRow;
            if (ui.mediaBrowser->music.selectedRow >= ui.mediaBrowser->music.scrollOffset + visibleRows)
                ui.mediaBrowser->music.scrollOffset = ui.mediaBrowser->music.selectedRow - visibleRows + 1;
            ui.mediaBrowser->music.scrollOffset = std::clamp(
                ui.mediaBrowser->music.scrollOffset, 0, std::max(0, count - visibleRows));
        };

        auto playMusicTrack = [&](int index) {
            const auto& list = KiddosServices::music();
            if (index < 0 || index >= static_cast<int>(list.size()))
                return false;
            const auto& track = list[static_cast<std::size_t>(index)];
            const bool resuming =
                index == currentMusicTrack && KiddosServices::musicPaused();
            const bool started = resuming
                ? KiddosServices::resumeMusic()
                : KiddosServices::launchMusic(track);
            if (!started) return false;

            currentMusicTrack = index;
            if (!resuming) loadMusicArtwork(ui, track.path);
            destroyText(ui.musicNowPlayingTitle);
            ui.musicNowPlayingTitle = makeText(
                ui.renderer, ui.labelFont, track.title.c_str(),
                {245, 248, 255, 255});
            return true;
        };

        auto moveMovieSelection = [&](int delta) {
            const int count = std::min(
                4, static_cast<int>(KiddosServices::movies().size()));
            if (count == 0) {
                ui.mediaBrowser->movies.selectedRow = 0;
                ui.mediaBrowser->movies.scrollOffset = 0;
                return;
            }
            ui.mediaBrowser->movies.selectedRow = std::clamp(
                ui.mediaBrowser->movies.selectedRow + delta, 0, count - 1);
            ui.mediaBrowser->movies.scrollOffset = 0;
        };

        auto moveTVSelection = [&](int delta) {
            const int count = std::min(
                4, static_cast<int>(KiddosServices::tvShows().size()));
            if (count <= 0) {
                ui.mediaBrowser->tvShows.selectedRow = 0;
                ui.mediaBrowser->tvShows.scrollOffset = 0;
                return;
            }
            ui.mediaBrowser->tvShows.selectedRow = std::clamp(
                ui.mediaBrowser->tvShows.selectedRow + delta, 0, count - 1);
            ui.mediaBrowser->tvShows.scrollOffset = 0;
        };

        bool launchTVShow = false;
        auto confirmTVSelection = [&]() {
            const int count = static_cast<int>(KiddosServices::tvShows().size());
            if (ui.mediaBrowser->tvShows.selectedRow >= 0 &&
                ui.mediaBrowser->tvShows.selectedRow < std::min(4, count))
                launchTVShow = true;
        };

        auto categoryName = [&]() -> const char* {
            switch (parentCategory) {
                case KiddosLibrary::Category::Games: return "GAMES";
                case KiddosLibrary::Category::Movies: return "MOVIES";
                case KiddosLibrary::Category::Music: return "MUSIC PLAYLISTS";
                case KiddosLibrary::Category::TVShows: return "TV SHOWS";
            }
            return "LIBRARY";
        };
        auto itemReference = [&](const KiddosLibrary::Item& item) {
            return std::filesystem::path(item.path).lexically_relative(romRoot).generic_string();
        };
        auto discoverParentItems = [&]() {
            parentLibrary = KiddosLibrary::discover(romRoot, parentCategory);
            parentLibrarySelected = 0;
            parentLibraryScroll = 0;
        };
        auto persistParentSlots = [&]() {
            return parentSlotsReady && KiddosLibrary::saveSlots(parentSlotsPath, parentSlots);
        };

        auto beginPinChange = [&]() {
            pinWorkflow.beginChange();
            refreshPinPrompt();
            screen = PinEntry;
        };

        auto cancelPinChange = [&]() {
            pinWorkflow.cancelChange();
            screen = ParentCenter;
        };

        auto submitPin = [&]() {
            const KiddosPin::WorkflowResult result = pinWorkflow.submit();
            refreshPinPrompt();
            if (result == KiddosPin::WorkflowResult::Unlocked) {
                screen = ParentCenter;
                parentSelected = 0;
            } else if (result == KiddosPin::WorkflowResult::Changed) {
                screen = ParentCenter;
                parentSelected = 5;
            }
        };

        bool launch = false;
        bool launchMovie = false;
        while (running && !launch && !launchMovie && !launchTVShow) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    continue;
                }

                if (event.type == SDL_JOYBUTTONDOWN) {
                    const KiddosInput::Action inputAction =
                        KiddosInput::fromJoystickButton(event.jbutton.button);

                    if (inputAction == KiddosInput::Action::OpenNowPlaying) {
                        if (KiddosServices::musicPlaying()) {
                            if (screen != NowPlaying) nowPlayingReturnScreen = screen;
                            screen = NowPlaying;
                        }
                    } else if (inputAction == KiddosInput::Action::PreviousSection ||
                               inputAction == KiddosInput::Action::NextSection) {
                        const auto& list = KiddosServices::music();
                        if (!list.empty() && currentMusicTrack >= 0) {
                            const int direction =
                                inputAction == KiddosInput::Action::PreviousSection ? -1 : 1;
                            const int count = static_cast<int>(list.size());
                            const int next = (currentMusicTrack + direction + count) % count;
                            if (!playMusicTrack(next)) SDL_Delay(1200);
                        }
                    } else if (inputAction == KiddosInput::Action::Back) {
                        if (screen == WifiPassword) {
                            if (!ui.wifi.password.empty())
                                ui.wifi.password.pop_back();
                            else {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                            ui.wifi.passwordCursor =
                                static_cast<int>(ui.wifi.password.size());
                        } else if (screen == WifiSettings) {
                            screen = ParentCenter;
                        } else if (screen == PinEntry && pinWorkflow.isChanging())
                            cancelPinChange();
                        else if (screen == ParentLibrary)
                            screen = ParentSlots;
                        else if (screen == ParentSlots)
                            screen = ParentCenter;
                        else if (screen == ConfirmShutdown ||
                                 screen == PinEntry || screen == ParentCenter)
                            screen = SystemMenu;
                        else if (screen == SystemMenu)
                            navigation.closeSystemMenu();
                        else if (screen == GameInfo)
                            screen = SelectedGame;
                        else if (screen == SelectedGame)
                            screen = Games;
                        else if (screen == NowPlaying)
                            screen = nowPlayingReturnScreen;
                        else if (screen == MusicPlaylist)
                            screen = Music;
                        else if (screen == TVShows)
                            screen = Home;
                        else if (screen == Movies || screen == Music || screen == Games)
                            screen = Home;
                    } else if (inputAction == KiddosInput::Action::Confirm) {
                        if (screen == NowPlaying) {
                            if (KiddosServices::musicPaused()) {
                                KiddosServices::resumeMusic();
                            } else if (KiddosServices::pauseMusic()) {
                                if (currentMusicTrack >= 0)
                                    ui.mediaBrowser->music.selectedRow = currentMusicTrack;

                            }
                        } else if (screen == WifiPassword) {
                            if (activateWifiKeyboardKey(ui)) {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                        } else if (screen == Home) {
                            if (homeSelected == 0) screen = Games;
                            else if (homeSelected == 1) screen = Music;
                            else if (homeSelected == 2) screen = Movies;
                            else if (homeSelected == 3) {
                                ui.mediaBrowser->tvShows.selectedRow = 0;
                                ui.mediaBrowser->tvShows.scrollOffset = 0;
                                screen = TVShows;
                            }
                        } else if (screen == Music) {
                            const auto& playlists = KiddosServices::musicPlaylists();
                            const int index = ui.mediaBrowser->music.selectedRow;
                            if (index >= 0 && index < static_cast<int>(playlists.size()) &&
                                !playlists[static_cast<std::size_t>(index)].tracks.empty()) {
                                ui.mediaBrowser->musicPlaylistIndex = index;
                                ui.mediaBrowser->music.selectedRow = 0;
                                ui.mediaBrowser->music.scrollOffset = 0;
                                currentMusicTrack = -1;
                                screen = MusicPlaylist;
                            }
                        } else if (screen == MusicPlaylist) {
                            nowPlayingReturnScreen = MusicPlaylist;
                            if (playMusicTrack(ui.mediaBrowser->music.selectedRow))
                                screen = NowPlaying;
                            else
                                SDL_Delay(1200);
                        } else if (screen == Movies) {
                            const auto& list = KiddosServices::movies();
                            if (ui.mediaBrowser->movies.selectedRow >= 0 &&
                                ui.mediaBrowser->movies.selectedRow < static_cast<int>(list.size()))
                                launchMovie = true;
                        } else if (screen == TVShows) {
                            confirmTVSelection();
                        } else if (screen == Games) {
                            if (KiddosServices::games()[selected].available) {
                                screen = SelectedGame;
                                action = 0;
                            }
                        } else if (screen == SelectedGame) {
                            if (!KiddosServices::games()[selected].available) {
                                screen = Games;
                            } else if (action == 0) {
                                launch = true;
                            } else if (action == 1) {
                                screen = GameInfo;
                            }
                        } else if (screen == GameInfo) {
                            screen = SelectedGame;
                        } else if (screen == SystemMenu) {
                            if (menuSelected == 1) {
                                screen = ConfirmShutdown;
                            } else {
                                pinWorkflow.beginUnlock();
                                refreshPinPrompt();
                                screen = PinEntry;
                            }
                        } else if (screen == PinEntry) {
                            submitPin();
                        } else if (screen == ParentCenter) {
                            if (parentSelected >= 0 && parentSelected <= 3) {
                                parentCategory = static_cast<KiddosLibrary::Category>(parentSelected);
                                parentSlotSelected = 0;
                                discoverParentItems();
                                screen = ParentSlots;
                            } else if (parentSelected == 4) {
                                updateWifiStatus(ui);
                                ui.wifi.commandError = false;
                                refreshWifiMenuText(ui);
                                loadWifiNetworks(ui, true);
                                screen = WifiSettings;
                            } else {
                                beginPinChange();
                            }
                        } else if (screen == ParentSlots) {
                            parentLibrarySelected = 0;
                            parentLibraryScroll = 0;
                            screen = ParentLibrary;
                        } else if (screen == ParentLibrary) {
                            if (!parentSlotsReady) {
                                SDL_Delay(800);
                            } else if (parentLibrarySelected == 0) {
                                KiddosLibrary::clear(parentSlots, parentCategory,
                                    static_cast<std::size_t>(parentSlotSelected));
                                if (persistParentSlots()) screen = ParentSlots;
                            } else {
                                const auto index = static_cast<std::size_t>(parentLibrarySelected - 1);
                                if (index < parentLibrary.size() &&
                                    KiddosLibrary::assign(parentSlots, parentCategory,
                                        static_cast<std::size_t>(parentSlotSelected),
                                        itemReference(parentLibrary[index])) && persistParentSlots())
                                    screen = ParentSlots;
                            }
                        } else if (screen == WifiSettings) {
                            if (ui.wifi.selectedRow == 0) {
                                toggleWifiPower(ui);
                            } else if (ui.wifi.selectedRow == 1) {
                                loadWifiNetworks(ui, true);
                            } else {
                                const auto& network =
                                    ui.wifi.networks[ui.wifi.selectedRow - 2];
                                startWifiConnection(ui, network, "");
                            }
                        } else if (screen == ConfirmShutdown &&
                                   KiddosServices::requestShutdown()) {
                            running = false;
                        }
                    } else if (inputAction == KiddosInput::Action::Up) {
                        if (screen == MusicPlaylist) {
                            moveMusicSelection(-1);
                        } else if (screen == Home) {
                            homeGrid.moveUp();
                        } else if (screen == Music) {
                            moveMusicSelection(-2);
                        } else if (screen == Movies) {
                            moveMovieSelection(-2);
                        } else if (screen == TVShows) {
                            moveTVSelection(-2);
                        } else if (screen == Games) {
                            gameGrid.moveUp();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 5) % 6;
                        } else if (screen == ParentSlots) {
                            parentSlotSelected = (parentSlotSelected + 3) % 4;
                        } else if (screen == ParentLibrary) {
                            parentLibrarySelected = std::max(0, parentLibrarySelected - 1);
                            if (parentLibrarySelected < parentLibraryScroll)
                                parentLibraryScroll = parentLibrarySelected;
                        } else if (screen == WifiSettings) {
                            moveWifiSelection(ui, -1);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, 0, -1);
                        } else if (screen == PinEntry) {
                            pinWorkflow.adjustDigit(1);
                        }
                    } else if (inputAction == KiddosInput::Action::Down) {
                        if (screen == MusicPlaylist) {
                            moveMusicSelection(1);
                        } else if (screen == Home) {
                            homeGrid.moveDown();
                        } else if (screen == Music) {
                            moveMusicSelection(2);
                        } else if (screen == Movies) {
                            moveMovieSelection(2);
                        } else if (screen == TVShows) {
                            moveTVSelection(2);
                        } else if (screen == Games) {
                            gameGrid.moveDown();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 1) % 6;
                        } else if (screen == ParentSlots) {
                            parentSlotSelected = (parentSlotSelected + 1) % 4;
                        } else if (screen == ParentLibrary) {
                            const int total = static_cast<int>(parentLibrary.size()) + 1;
                            parentLibrarySelected = std::min(total - 1, parentLibrarySelected + 1);
                            if (parentLibrarySelected >= parentLibraryScroll + 8)
                                parentLibraryScroll = parentLibrarySelected - 7;
                        } else if (screen == WifiSettings) {
                            moveWifiSelection(ui, 1);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, 0, 1);
                        } else if (screen == PinEntry) {
                            pinWorkflow.adjustDigit(-1);
                        }
                    } else if (inputAction == KiddosInput::Action::Left) {
                        if (screen == MusicPlaylist) {
                            moveMusicSelection(-1);
                        } else if (screen == Home) {
                            homeGrid.moveLeft();
                        } else if (screen == Music) {
                            moveMusicSelection(-1);
                        } else if (screen == Movies) {
                            moveMovieSelection(-1);
                        } else if (screen == TVShows) {
                            moveTVSelection(-1);
                        } else if (screen == Games) {
                            gameGrid.moveLeft();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, -1, 0);
                        } else if (screen == PinEntry) {
                            pinWorkflow.moveCursor(-1);
                        }
                    } else if (inputAction == KiddosInput::Action::Right) {
                        if (screen == MusicPlaylist) {
                            moveMusicSelection(1);
                        } else if (screen == Home) {
                            homeGrid.moveRight();
                        } else if (screen == Music) {
                            moveMusicSelection(1);
                        } else if (screen == Movies) {
                            moveMovieSelection(1);
                        } else if (screen == TVShows) {
                            moveTVSelection(1);
                        } else if (screen == Games) {
                            gameGrid.moveRight();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, 1, 0);
                        } else if (screen == PinEntry) {
                            pinWorkflow.moveCursor(1);
                        }
                    } else if (inputAction == KiddosInput::Action::Start && screen == WifiPassword) {
                        ui.wifi.keyboardRow = 3;
                        ui.wifi.keyboardCol = 5;
                        if (activateWifiKeyboardKey(ui)) {
                            screen = WifiSettings;
                            refreshWifiSettings(ui);
                        }
                    } else if (inputAction == KiddosInput::Action::Start &&
                               (screen == Home || screen == Games ||
                                screen == Movies || screen == TVShows || screen == Music || screen == MusicPlaylist ||
                                screen == NowPlaying || screen == SelectedGame || screen == GameInfo)) {
                        navigation.openSystemMenu();
                    }
                }

                if (event.type == SDL_JOYHATMOTION &&
                    screen == PinEntry) {
                    const Uint8 hat = event.jhat.value;

                    if (hat & SDL_HAT_UP) {
                        pinWorkflow.adjustDigit(1);
                    }
                    if (hat & SDL_HAT_DOWN) {
                        pinWorkflow.adjustDigit(-1);
                    }
                    if (hat & SDL_HAT_LEFT) {
                        pinWorkflow.moveCursor(-1);
                    }
                    if (hat & SDL_HAT_RIGHT) {
                        pinWorkflow.moveCursor(1);
                    }
                }
            }

            // Refresh the live carousel when remote slot assignments change.
            const Uint32 now = SDL_GetTicks();

            if (SDL_TICKS_PASSED(now, lastCatalogPoll + 1000)) {
                lastCatalogPoll = now;

                struct stat currentSlotsStat {};
                if (stat(slotsPath, &currentSlotsStat) == 0) {
                    const bool changed =
                        !haveSlotsStat ||
                        currentSlotsStat.st_ino != lastSlotsStat.st_ino ||
                        currentSlotsStat.st_mtim.tv_sec !=
                            lastSlotsStat.st_mtim.tv_sec ||
                        currentSlotsStat.st_mtim.tv_nsec !=
                            lastSlotsStat.st_mtim.tv_nsec;

                    if (changed && KiddosServices::loadGameSlots() &&
                        KiddosServices::loadMovies() &&
                        KiddosServices::loadMusic() &&
                        KiddosServices::loadTVShows()) {
                        lastSlotsStat = currentSlotsStat;
                        haveSlotsStat = true;

                        refreshGameArtwork(ui);
                        refreshMediaArtwork(ui);
                        updateSelectedGameText(ui, selected);

                        if ((screen == SelectedGame || screen == GameInfo) &&
                            !KiddosServices::games()[selected].available) {
                            screen = Games;
                        }
                    } else if (changed) {
                        std::fprintf(stderr,
                            "KiddOS: changed catalog could not be loaded; "
                            "will retry.\n");
                    }
                }
            }

            if (currentMusicTrack >= 0 &&
                !KiddosServices::musicPaused() &&
                !KiddosServices::musicPlaying()) {
                const auto& tracks = KiddosServices::music();
                if (!tracks.empty()) {
                    const int next =
                        (currentMusicTrack + 1) % static_cast<int>(tracks.size());
                    if (!playMusicTrack(next))
                        currentMusicTrack = -1;
                } else {
                    currentMusicTrack = -1;
                }
            }

            if (screen == NowPlaying &&
                !KiddosServices::musicPlaying() &&
                !KiddosServices::musicPaused()) {
                screen = nowPlayingReturnScreen;
            }

            if (running && !launch && !launchMovie && !launchTVShow) {
                updateBattery(ui);
                pollWifiConnection(ui);
                if (ui.wifi.needsPassword) {
                    ui.wifi.needsPassword = false;
                    if (screen == WifiSettings) {
                        ui.wifi.password.clear();
                        ui.wifi.passwordCursor = 0;
                        ui.wifi.passwordChar = 0;
                        ui.wifi.keyboardRow = 0;
                        ui.wifi.keyboardCol = 0;
                        ui.wifi.keyboardShift = false;
                        ui.wifi.keyboardSymbols = false;
                        screen = WifiPassword;
                    }
                }
                if (screen == Home) drawHome(ui, homeSelected);
                else if (screen == Games) drawUI(ui, selected);
                else if (screen == Movies) drawMovies(ui);
                else if (screen == TVShows) drawTVShows(ui);
                else if (screen == Music) drawMusic(ui);
                else if (screen == MusicPlaylist) drawMusicPlaylist(ui);
                else if (screen == NowPlaying) drawNowPlaying(ui);
                else if (screen == SelectedGame) drawSelectedGame(ui, selected, action);
                else if (screen == GameInfo) drawGameInfo(ui);
                else if (screen == SystemMenu) drawSystemMenu(ui, menuSelected);
                else if (screen == PinEntry)
                    drawPinEntry(ui, pinWorkflow.digits(), pinWorkflow.position(),
                                 pinWorkflow.hasError());
                else if (screen == ParentCenter)
                    drawParentCenter(ui, parentSelected);
                else if (screen == ParentSlots || screen == ParentLibrary) {
                    SDL_Renderer* r = ui.renderer;
                    drawSimplePanel(r);
                    drawText(r, ui.heading, 360, 90);
                    drawWifiLine(r, ui.headingFont, categoryName(), 75, 145,
                                 {245,248,255,255});
                    if (!parentSlotsReady) {
                        drawWifiLine(r, ui.labelFont, "SLOT FILE UNAVAILABLE - NO CHANGES SAVED",
                                     75, 240, {255,115,115,255});
                    } else if (screen == ParentSlots) {
                        for (int i = 0; i < 4; ++i) {
                            const int x = (i % 2) ? 375 : 75;
                            const int y = (i / 2) ? 375 : 220;
                            SDL_Rect tile{x,y,270,125};
                            SDL_SetRenderDrawColor(r,28,38,68,255);
                            SDL_RenderFillRect(r,&tile);
                            if (i == parentSlotSelected) {
                                SDL_SetRenderDrawColor(r,80,190,255,255);
                                SDL_Rect outline{x-3,y-3,276,131};
                                SDL_RenderDrawRect(r,&outline);
                            }
                            const auto& ref = parentSlots[static_cast<std::size_t>(parentCategory)][static_cast<std::size_t>(i)];
                            std::string label = "SLOT " + std::to_string(i+1) + " - EMPTY";
                            if (!ref.empty()) {
                                auto found = std::find_if(parentLibrary.begin(),parentLibrary.end(),
                                    [&](const KiddosLibrary::Item& item) { return itemReference(item) == ref; });
                                label = found == parentLibrary.end() ? "SLOT " + std::to_string(i+1) + " - ITEM NOT FOUND" : "SLOT " + std::to_string(i+1) + " - " + found->title;
                            }
                            drawWifiLine(r,ui.labelFont,label,x+12,y+48,{245,248,255,255});
                        }
                        drawWifiLine(r,ui.footerFont,"A CHOOSE / REPLACE     B BACK",145,625,{190,195,210,255});
                    } else {
                        drawWifiLine(r,ui.footerFont,"0: CLEAR THIS SLOT",75,190,{255,190,110,255});
                        const int total = static_cast<int>(parentLibrary.size()) + 1;
                        for (int row = 0; row < 8; ++row) {
                            const int index = parentLibraryScroll + row;
                            if (index >= total) break;
                            const int y = 225 + row * 45;
                            SDL_Rect line{65,y-5,590,38};
                            if (index == parentLibrarySelected) {
                                SDL_SetRenderDrawColor(r,28,65,105,255);
                                SDL_RenderFillRect(r,&line);
                                SDL_SetRenderDrawColor(r,80,190,255,255);
                                SDL_RenderDrawRect(r,&line);
                            }
                            const std::string label = index == 0 ? "CLEAR SLOT ASSIGNMENT" : parentLibrary[static_cast<std::size_t>(index-1)].title;
                            drawWifiLine(r,ui.labelFont,label,82,y,
                                index == 0 ? SDL_Color{255,190,110,255} : SDL_Color{245,248,255,255});
                        }
                        char count[64];
                        std::snprintf(count,sizeof(count),"%zu ARTWORK-BACKED ITEMS",parentLibrary.size());
                        drawWifiLine(r,ui.footerFont,count,75,590,{190,195,210,255});
                        drawWifiLine(r,ui.footerFont,"A SELECT     B BACK",440,625,{190,195,210,255});
                    }
                    drawBattery(ui);
                    SDL_RenderPresent(r);
                }
                else if (screen == WifiSettings)
                    drawWifiSettings(ui);
                else if (screen == WifiPassword)
                    drawWifiPassword(ui);
                else drawShutdownConfirm(ui);
                SDL_Delay(16);
            }
        }

        cancelWifiConnection(ui);
        std::fill(ui.wifi.password.begin(), ui.wifi.password.end(), '\0');
        if (!running || launch || launchMovie || launchTVShow)
            KiddosServices::stopMusic();
        closeUI(ui);
        if (running && launch && KiddosServices::games()[selected].available) {
            if (!KiddosServices::launchGame(KiddosServices::games()[selected]))
                SDL_Delay(1500);
            screen = Games;
        }
        if (running && launchTVShow) {
            const auto& list = KiddosServices::tvShows();
            if (mediaBrowser_.tvShows.selectedRow >= 0 &&
                mediaBrowser_.tvShows.selectedRow < std::min(
                    4, static_cast<int>(list.size()))) {
                if (!KiddosServices::launchTVShow(
                        list[static_cast<std::size_t>(mediaBrowser_.tvShows.selectedRow)]))
                    SDL_Delay(1500);
            }
            screen = TVShows;
        }
        if (running && launchMovie) {
            const auto& list = KiddosServices::movies();
            if (mediaBrowser_.movies.selectedRow >= 0 &&
                mediaBrowser_.movies.selectedRow < static_cast<int>(list.size())) {
                if (!KiddosServices::launchMovie(
                        list[static_cast<std::size_t>(mediaBrowser_.movies.selectedRow)]))
                    SDL_Delay(1500);
            }
            screen = Movies;
        }
    }

    return 0;
}
