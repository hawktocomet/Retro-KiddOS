// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "Application.hpp"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>

#include "UI.hpp"
#include "PinWorkflow.hpp"
#include "models/Games.hpp"
#include "services/GameLauncher.hpp"
#include "services/GameCatalog.hpp"
#include "services/PowerService.hpp"

Application::Application()
    : pinWorkflow_(std::make_unique<KiddosPin::PinWorkflow>(
          "/storage/.config/kiddos")) {}

Application::~Application() = default;

int Application::run() {
    using namespace KiddosNavigation;

    KiddosUI::LibraryGrid& gameGrid = library_.grid();
    int& selected = gameGrid.selectedRef();
    int& action = action_;
    int& menuSelected = menuSelected_;
    Navigation& navigation = navigation_;
    Screen& screen = navigation.currentRef();
    KiddosPin::PinWorkflow& pinWorkflow = *pinWorkflow_;
    int& parentSelected = parentSelected_;
    bool& running = running_;

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
        UI ui;
        if (!openUI(ui)) return 1;
        updateSelectedGameText(ui, selected);

        // The management server atomically replaces slots.json on assignment.
        // Track inode and nanosecond mtime to detect each replacement.
        constexpr const char* slotsPath =
            "/storage/.kiddos-management/slots.json";
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

        auto beginPinChange = [&]() {
            pinWorkflow.beginChange();
            refreshPinPrompt();
            screen = PinEntry;
        };

        auto cancelPinChange = [&]() {
            pinWorkflow.cancelChange();
            screen = ParentCenter;
            refreshParentStatus(ui);
        };

        auto submitPin = [&]() {
            const KiddosPin::WorkflowResult result = pinWorkflow.submit();
            refreshPinPrompt();
            if (result == KiddosPin::WorkflowResult::Unlocked) {
                screen = ParentCenter;
                parentSelected = 0;
                refreshParentStatus(ui);
            } else if (result == KiddosPin::WorkflowResult::Changed) {
                screen = ParentCenter;
                parentSelected = 2;
                refreshParentStatus(ui);
            }
        };

        bool launch = false;
        while (running && !launch) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    continue;
                }

                if (event.type == SDL_KEYDOWN) {
                    const SDL_Keycode key = event.key.keysym.sym;

                    if (key == SDLK_ESCAPE) {
                        running = false;
                        continue;
                    }

                    if (key == SDLK_BACKSPACE) {
                        if (screen == WifiPassword) {
                            if (ui.wifi.passwordCursor > 0) {
                                ui.wifi.password.erase(ui.wifi.password.begin() + --ui.wifi.passwordCursor);
                            } else {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                        } else if (screen == WifiSettings)
                            screen = ParentCenter;
                        else if (screen == PinEntry && pinWorkflow.isChanging())
                            cancelPinChange();
                        else if (screen == ConfirmShutdown ||
                                 screen == PinEntry || screen == ParentCenter)
                            screen = SystemMenu;
                        else if (screen == SystemMenu)
                            navigation.closeSystemMenu();
                        else if (screen == GameInfo)
                            screen = SelectedGame;
                        else if (screen == SelectedGame)
                            screen = Games;
                        continue;
                    }

                    if (screen == PinEntry) {
                        if (key == SDLK_UP) {
                            pinWorkflow.adjustDigit(1);
                        } else if (key == SDLK_DOWN) {
                            pinWorkflow.adjustDigit(-1);
                        } else if (key == SDLK_LEFT) {
                            pinWorkflow.moveCursor(-1);
                        } else if (key == SDLK_RIGHT) {
                            pinWorkflow.moveCursor(1);
                        } else if (key >= SDLK_0 && key <= SDLK_9) {
                            pinWorkflow.setDigit(static_cast<int>(key - SDLK_0));
                        } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            submitPin();
                        }
                        continue;
                    }

                    if (screen == WifiPassword) {
                        if (key == SDLK_UP) moveWifiKeyboard(ui, 0, -1);
                        else if (key == SDLK_DOWN) moveWifiKeyboard(ui, 0, 1);
                        else if (key == SDLK_LEFT) moveWifiKeyboard(ui, -1, 0);
                        else if (key == SDLK_RIGHT) moveWifiKeyboard(ui, 1, 0);
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (activateWifiKeyboardKey(ui)) {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                        } else if (key == SDLK_BACKSPACE ||
                                   key == SDLK_DELETE) {
                            if (!ui.wifi.password.empty())
                                ui.wifi.password.pop_back();
                            ui.wifi.passwordCursor =
                                static_cast<int>(ui.wifi.password.size());
                        } else if (key == SDLK_ESCAPE) {
                            screen = WifiSettings;
                            refreshWifiSettings(ui);
                        }
                        continue;
                    }

                    if (screen == WifiSettings) {
                        if (key == SDLK_UP)
                            moveWifiSelection(ui, -1);
                        else if (key == SDLK_DOWN)
                            moveWifiSelection(ui, 1);
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (ui.wifi.selectedRow == 0) {
                                toggleWifiPower(ui);
                            } else if (ui.wifi.selectedRow == 1) {
                                loadWifiNetworks(ui, true);
                            } else {
                                const auto& network =
                                    ui.wifi.networks[ui.wifi.selectedRow - 2];
                                startWifiConnection(ui, network, "");
                            }
                        }
                        continue;
                    }

                    if (screen == ParentCenter) {
                        if (key == SDLK_UP)
                            parentSelected = (parentSelected + 2) % 3;
                        else if (key == SDLK_DOWN)
                            parentSelected = (parentSelected + 1) % 3;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (parentSelected == 0)
                                toggleParentManagement(ui);
                            else if (parentSelected == 1) {
                                updateWifiStatus(ui);
                                ui.wifi.commandError = false;
                                refreshWifiMenuText(ui);
                                loadWifiNetworks(ui, true);
                                screen = WifiSettings;
                            } else
                                beginPinChange();
                        }
                        continue;
                    }

                    if (screen == Games) {
                        switch (key) {
                            case SDLK_UP:    gameGrid.moveUp(); break;
                            case SDLK_DOWN:  gameGrid.moveDown(); break;
                            case SDLK_LEFT:  gameGrid.moveLeft(); break;
                            case SDLK_RIGHT: gameGrid.moveRight(); break;
                            case SDLK_RETURN:
                            case SDLK_SPACE:
                                if (games[selected].available) {
                                    screen = SelectedGame;
                                    action = 0;
                                }
                                break;
                            case SDLK_s:
                                navigation.openSystemMenu();
                                break;
                        }
                        updateSelectedGameText(ui, selected);
                    } else if (screen == SelectedGame) {
                        if (!games[selected].available) {
                            screen = Games;
                        } else if (key == SDLK_UP || key == SDLK_DOWN) {
                            action = (action + 1) % 2;
                        } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (action == 0) launch = true;
                            else if (action == 1) screen = GameInfo;
                        }
                    } else if (screen == GameInfo) {
                        if (key == SDLK_RETURN || key == SDLK_SPACE)
                            screen = SelectedGame;
                    } else if (screen == SystemMenu) {
                        if (key == SDLK_UP || key == SDLK_DOWN)
                            menuSelected = (menuSelected + 1) % 2;
                        else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                            if (menuSelected == 1) {
                                screen = ConfirmShutdown;
                            } else {
                                pinWorkflow.beginUnlock();
                                refreshPinPrompt();
                                screen = PinEntry;
                            }
                        }
                    } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                        if (KiddosServices::requestShutdown()) running = false;
                    }
                }

                if (event.type == SDL_JOYBUTTONDOWN) {
                    const KiddosInput::Action inputAction =
                        KiddosInput::fromJoystickButton(event.jbutton.button);

                    if (inputAction == KiddosInput::Action::Back) {
                        if (screen == WifiPassword) {
                            if (!ui.wifi.password.empty())
                                ui.wifi.password.pop_back();
                            else {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                            ui.wifi.passwordCursor =
                                static_cast<int>(ui.wifi.password.size());
                        } else if (screen == WifiSettings)
                            screen = ParentCenter;
                        else if (screen == PinEntry && pinWorkflow.isChanging())
                            cancelPinChange();
                        else if (screen == ConfirmShutdown ||
                                 screen == PinEntry || screen == ParentCenter)
                            screen = SystemMenu;
                        else if (screen == SystemMenu)
                            navigation.closeSystemMenu();
                        else if (screen == GameInfo)
                            screen = SelectedGame;
                        else if (screen == SelectedGame)
                            screen = Games;
                    } else if (inputAction == KiddosInput::Action::Confirm) {
                        if (screen == WifiPassword) {
                            if (activateWifiKeyboardKey(ui)) {
                                screen = WifiSettings;
                                refreshWifiSettings(ui);
                            }
                        } else if (screen == Games) {
                            if (games[selected].available) {
                                screen = SelectedGame;
                                action = 0;
                            }
                        } else if (screen == SelectedGame) {
                            if (!games[selected].available) {
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
                            if (parentSelected == 0)
                                toggleParentManagement(ui);
                            else if (parentSelected == 1) {
                                updateWifiStatus(ui);
                                ui.wifi.commandError = false;
                                refreshWifiMenuText(ui);
                                loadWifiNetworks(ui, true);
                                screen = WifiSettings;
                            } else
                                beginPinChange();
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
                        if (screen == Games) {
                            gameGrid.moveUp();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 2) % 3;
                        } else if (screen == WifiSettings) {
                            moveWifiSelection(ui, -1);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, 0, -1);
                        } else if (screen == PinEntry) {
                            pinWorkflow.adjustDigit(1);
                        }
                    } else if (inputAction == KiddosInput::Action::Down) {
                        if (screen == Games) {
                            gameGrid.moveDown();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == SelectedGame) {
                            action = (action + 1) % 2;
                        } else if (screen == SystemMenu) {
                            menuSelected = (menuSelected + 1) % 2;
                        } else if (screen == ParentCenter) {
                            parentSelected = (parentSelected + 1) % 3;
                        } else if (screen == WifiSettings) {
                            moveWifiSelection(ui, 1);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, 0, 1);
                        } else if (screen == PinEntry) {
                            pinWorkflow.adjustDigit(-1);
                        }
                    } else if (inputAction == KiddosInput::Action::Left) {
                        if (screen == Games) {
                            gameGrid.moveLeft();
                            updateSelectedGameText(ui, selected);
                        } else if (screen == WifiPassword) {
                            moveWifiKeyboard(ui, -1, 0);
                        } else if (screen == PinEntry) {
                            pinWorkflow.moveCursor(-1);
                        }
                    } else if (inputAction == KiddosInput::Action::Right) {
                        if (screen == Games) {
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
                               (screen == Games || screen == SelectedGame ||
                                screen == GameInfo)) {
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

                    if (changed && KiddosServices::loadGameSlots()) {
                        lastSlotsStat = currentSlotsStat;
                        haveSlotsStat = true;

                        refreshGameArtwork(ui);
                        updateSelectedGameText(ui, selected);

                        if ((screen == SelectedGame || screen == GameInfo) &&
                            !games[selected].available) {
                            screen = Games;
                        }
                    } else if (changed) {
                        std::fprintf(stderr,
                            "KiddOS: changed catalog could not be loaded; "
                            "will retry.\n");
                    }
                }
            }

            if (running && !launch) {
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
                if (screen == Games) drawUI(ui, selected);
                else if (screen == SelectedGame) drawSelectedGame(ui, selected, action);
                else if (screen == GameInfo) drawGameInfo(ui);
                else if (screen == SystemMenu) drawSystemMenu(ui, menuSelected);
                else if (screen == PinEntry)
                    drawPinEntry(ui, pinWorkflow.digits(), pinWorkflow.position(),
                                 pinWorkflow.hasError());
                else if (screen == ParentCenter)
                    drawParentCenter(ui, parentSelected);
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
        closeUI(ui);
        if (running && launch && games[selected].available) {
            if (!KiddosServices::launchGame(games[selected]))
                SDL_Delay(1500);
            screen = Games;
        }
    }

    return 0;
}
