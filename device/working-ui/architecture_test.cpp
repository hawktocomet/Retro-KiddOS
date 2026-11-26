// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 hawktocomet

#include "Input.hpp"
#include "Navigation.hpp"
#include "LibraryGrid.hpp"
#include "LibraryNavigator.hpp"
#include "PinWorkflow.hpp"
#include "screens/ScreenState.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace {
int failures = 0;

void check(bool condition, const char* description) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++failures;
}
}

int main() {
    using KiddosInput::Action;
    check(KiddosInput::fromJoystickButton(0) == Action::Back, "B maps to Back");
    check(KiddosInput::fromJoystickButton(1) == Action::Confirm, "A maps to Confirm");
    check(KiddosInput::fromJoystickButton(4) == Action::PreviousSection, "L maps to previous section");
    check(KiddosInput::fromJoystickButton(5) == Action::NextSection, "R maps to next section");
    check(KiddosInput::fromJoystickButton(8) == Action::Up, "D-pad up mapping");
    check(KiddosInput::fromJoystickButton(9) == Action::Down, "D-pad down mapping");
    check(KiddosInput::fromJoystickButton(10) == Action::Left, "D-pad left mapping");
    check(KiddosInput::fromJoystickButton(11) == Action::Right, "D-pad right mapping");
    check(KiddosInput::fromJoystickButton(13) == Action::Start, "Start mapping");
    check(KiddosInput::fromJoystickButton(2) == Action::None, "unknown button ignored");

    KiddosNavigation::Navigation navigation;
    check(navigation.current() == KiddosNavigation::Home, "navigation starts at Home");
    navigation.openSystemMenu();
    check(navigation.current() == KiddosNavigation::SystemMenu, "Start opens System Menu");
    navigation.closeSystemMenu();
    check(navigation.current() == KiddosNavigation::Home, "System Menu returns to Home");
    navigation.show(KiddosNavigation::GameInfo);
    navigation.openSystemMenu();
    navigation.closeSystemMenu();
    check(navigation.current() == KiddosNavigation::GameInfo, "System Menu preserves non-home return screen");

    KiddosScreens::MediaBrowserState browserState;
    browserState.movies.selectedRow = 2;
    browserState.tvShows.selectedRow = 1;
    browserState.music.selectedRow = 7;
    check(browserState.movies.selectedRow == 2 &&
          browserState.tvShows.selectedRow == 1 &&
          browserState.music.selectedRow == 7,
          "media browser selections remain independent");

    KiddosUI::LibraryGrid grid(4);
    grid.moveRight();
    check(grid.selected() == 1, "grid moves right");
    grid.moveDown();
    check(grid.selected() == 3, "grid moves down");
    grid.moveDown();
    check(grid.selected() == 3, "grid clamps at final row");
    grid.moveLeft();
    check(grid.selected() == 2, "grid moves left");
    grid.moveUp();
    check(grid.selected() == 0, "grid moves up");
    grid.setItemCount(2);
    grid.select(3);
    check(grid.selected() == 1, "grid clamps selection when item count shrinks");
    KiddosUI::LibraryNavigator library(4);
    library.grid().select(3);
    library.nextSection();
    check(library.section() == KiddosUI::LibrarySection::Movies &&
          library.grid().selected() == 0,
          "next library section skips unimplemented Music");
    library.previousSection();
    check(library.section() == KiddosUI::LibrarySection::Games &&
          library.grid().selected() == 0,
          "previous library section returns to Games");
    library.previousSection();
    check(library.section() == KiddosUI::LibrarySection::Movies,
          "previous library section wraps from Games to Movies");

    char directory[] = "/tmp/kiddos-workflow-test-XXXXXX";
    const bool madeDirectory = mkdtemp(directory) != nullptr;
    check(madeDirectory, "temporary PIN store directory created");
    if (madeDirectory) {
        check(KiddosPin::initializePinStore(directory), "PIN store initialized");
        KiddosPin::PinWorkflow workflow(directory);
        check(workflow.prompt() == std::string("ENTER PARENT PIN"), "PIN workflow has unlock prompt");
        workflow.setDigit(1);
        workflow.setDigit(2);
        workflow.setDigit(3);
        workflow.setDigit(4);
        check(workflow.submit() == KiddosPin::WorkflowResult::Unlocked,
              "correct PIN unlocks Parent Mode");
        workflow.beginUnlock();
        workflow.setDigit(0); workflow.setDigit(0);
        workflow.setDigit(0); workflow.setDigit(0);
        check(workflow.submit() == KiddosPin::WorkflowResult::Pending &&
              workflow.hasError(), "incorrect PIN is rejected and marked");

        workflow.beginChange();
        workflow.setDigit(1); workflow.setDigit(2);
        workflow.setDigit(3); workflow.setDigit(4);
        check(workflow.submit() == KiddosPin::WorkflowResult::Pending &&
              workflow.prompt() == std::string("ENTER NEW PIN"),
              "current PIN advances to new PIN step");
        workflow.setDigit(5); workflow.setDigit(6);
        workflow.setDigit(7); workflow.setDigit(8);
        check(workflow.submit() == KiddosPin::WorkflowResult::Pending &&
              workflow.prompt() == std::string("CONFIRM NEW PIN"),
              "new PIN advances to confirmation step");
        workflow.setDigit(5); workflow.setDigit(6);
        workflow.setDigit(7); workflow.setDigit(9);
        check(workflow.submit() == KiddosPin::WorkflowResult::Pending &&
              workflow.hasError() && workflow.prompt() == std::string("ENTER NEW PIN"),
              "mismatched confirmation returns to new PIN step");
        workflow.setDigit(5); workflow.setDigit(6);
        workflow.setDigit(7); workflow.setDigit(8);
        check(workflow.submit() == KiddosPin::WorkflowResult::Pending &&
              workflow.prompt() == std::string("CONFIRM NEW PIN"),
              "new PIN can be re-entered after mismatch");
        workflow.setDigit(5); workflow.setDigit(6);
        workflow.setDigit(7); workflow.setDigit(8);
        check(workflow.submit() == KiddosPin::WorkflowResult::Changed,
              "confirmed new PIN is saved");
        check(KiddosPin::verifyPin(directory, "5678"), "new PIN verifies in store");
        workflow.beginUnlock();
        workflow.setDigit(5); workflow.setDigit(6);
        workflow.setDigit(7); workflow.setDigit(8);
        check(workflow.submit() == KiddosPin::WorkflowResult::Unlocked,
              "new PIN unlocks Parent Mode");

        char hashPath[512];
        std::snprintf(hashPath, sizeof(hashPath), "%s/pin.hash", directory);
        unlink(hashPath);
        rmdir(directory);
    }

    return failures == 0 ? 0 : 1;
}
