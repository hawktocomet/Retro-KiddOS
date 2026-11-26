# Child UI Architecture Audit and Refactor Plan

Date: November 23, 2026
Status: Audit complete; shared interaction model agreed; implementation in progress.

## Purpose

Establish a consistent, maintainable child-facing experience for Games,
Movies, Music, and TV Shows without building an unnecessarily elaborate UI
framework or changing media-specific behavior that already works.

The priority is a reliable, easy-to-use experience for young children.
Every refactor should make browsing, selecting, or playing content easier,
or make the implementation meaningfully more reliable.

This work concerns the child-facing SDL application. Parent-facing Remote
Management remains a separate browser interface and management service.

## Architectural direction

All four content categories use the same child-facing selection experience.
The Games workflow is the interaction reference, simplified to remove
unnecessary choices.

1. Shared content-selection window.
   - Every category opens the same 2×2 artwork grid.
   - Shared layout, artwork sizing, selection highlighting, empty states,
     navigation, and confirmation behavior.
   - Artwork is the primary way children recognize content; text is
     supplemental, never required to operate the interface.
   - All four workflows use the same D-pad/controller behavior.

2. Shared selected-media confirmation experience.
   - Selecting a grid item opens a screen showing that item's artwork.
   - The screen has one primary action, activated with A.
   - B always returns to the previous screen; communicate this through the
     controller hint rather than a separate on-screen Back button.
   - The selected screen is also an accidental-selection guard before
     launching or starting playback.

3. Media-specific primary actions.
   - Games: Play.
   - Movies: Resume. If no saved playback position exists, start from the
     beginning. Do not offer Start Over or other choices.
   - Music: Play Playlist. A playlist, not an individual track, is the
     selectable content item.
   - TV Shows: Resume the saved episode/playback position; if no saved
     progress exists, begin the appropriate first episode.
   - Game Info is removed from the child-facing workflow.

4. Media-specific follow-on experiences remain distinct where necessary.
   - Music Now Playing retains playback controls and displays the current
     MP3's embedded artwork.
   - TV episode progression and resume behavior remain in the playback
     service; children do not have to navigate a text-heavy track/episode
     list to start the selected show.
   - Parent-side Remote Management remains the place for complex content
     management and metadata.

Share presentation and common interaction behavior. Do not force different
media types into a generic content model or generic playback implementation.
The shared component should be small and driven by the actual four workflows.

## Audit findings

### Browsing and presentation

Games, Movies, and TV Shows independently implement similar four-tile
browsing layouts. Their drawing code repeats tile geometry, artwork scaling,
selection outlines, and fallback presentation.

`LibraryGrid.hpp` provides a basic grid-selection abstraction, but its use
is limited and it does not currently provide a shared media browsing
experience.

Music uses a separate playlist/track presentation. Its current top-level
playlist grid is a placeholder: it presents one generic playlist slot when
tracks exist, while the underlying catalog is a flat list of tracks.

Movie and TV browsing currently restrict selection and display to the first
four entries. Confirm intended slot and catalog behavior before changing
that constraint.

### Selected-media behavior

The four media types do not currently share a selected-media experience.

- Games have SelectedGame and GameInfo screens, with Play and Game Info
  actions.
- Movies launch directly from the browsing grid.
- TV Shows launch directly from the browsing grid.
- Music opens a flat track list from its top-level screen.

The target is one selected-media confirmation pattern with one primary
action. A proceeds; B returns. Games play, Movies resume, Music plays the
selected playlist, and TV Shows resume. Game Info and child-facing
text-heavy track selection are removed from the target workflow.

Movie and TV confirmation screens are intentional accidental-selection
guards, not menus. If no saved progress exists, their Resume action starts
from the beginning (or the first appropriate TV episode).

The current Music implementation does not have a playlist catalog or
playlist model. Its top-level grid is a placeholder and its track list is
not the intended child-facing selection experience. Implement actual
playlists as the selectable content type.

### State and navigation

MovieBrowserState, TVBrowserState, and MusicBrowserState currently have
identical fields.

Application and UI both hold copies of these browser states. Application
copies them into UI before rendering and copies them back afterward.
Choose one authoritative owner.

Application.cpp contains a large event loop with duplicated physical-keyboard
and controller handling, screen-specific selection logic, and back transitions.
Physical-keyboard input is not part of the handheld interaction model and will
be removed. The controller path is authoritative.

Navigation::currentRef() permits direct mutation of navigation state.
LibraryNavigator has incomplete section behavior and does not include TV
Shows. Reassess these abstractions rather than extending them blindly.

### Catalogs and services

The media models and catalog implementations are inconsistent:

- Games use the shared device-side Python catalog also used by Remote
  Management.
- Movies use a Python-based flat-directory catalog.
- Music uses a Python-based flat-track catalog.
- TV Shows use a C++ filesystem catalog.

Game, Movie, and Music catalog services repeat subprocess and parsing
patterns. TV Shows use a different implementation.

Standardize useful service boundaries, identifiers, and error handling
where practical. Preserve media-specific metadata and behavior.

### Playback behavior to preserve

- Game launch and return to the library.
- Movie MPV playback and resume behavior.
- Music playback controls and automatic track progression.
- TV episode progression, controller shortcuts, and saved episode position.
- Existing MP3 embedded artwork during playback.

Music playlist artwork and track artwork are separate concepts:
each playlist needs its own artwork for its child-facing slot, while
Now Playing continues to display the embedded artwork of the current MP3.

### Resource ownership and reliability

UI owns SDL windows, renderers, textures, fonts, and related resources.
Application also owns some screen state. Clarify ownership and lifecycle
boundaries during the refactor.

Now Playing creates and destroys several text textures on every frame.
Cache or update these only when their content changes.

Review partial SDL initialization and cleanup paths, artwork loading,
temporary files, and error propagation. Avoid unrelated rewrites.

Specific issues identified for follow-up:

- GameLauncher currently hardcodes a Gambatte core path and core name.
  This is not a production solution for the full supported game-system
  scope.
- TVShowLauncher contains valuable resume and episode-navigation behavior,
  but MPV startup failure and IPC error handling deserve review.
- Music currently has no playlist catalog or playlist model.
- Movie and TV browsing have no selected-media screen.
- The standalone CMake source list omits TV source files even though the
  actual ARM build script includes them. Reconcile build definitions so
  they describe the same application.

## Target ownership boundaries

- Application: application lifecycle and high-level workflow orchestration.
- Navigation: screen transitions and return destinations.
- Shared UI components: reusable browsing and selected-media presentation.
- Media models/catalogs: content metadata and stable identity.
- Media services/launchers: media-specific actions and playback lifecycle.
- UI/SDL layer: SDL resource ownership and rendering infrastructure.

Avoid duplicate state ownership. Rendering should consume authoritative
state rather than maintaining a second copy of application state.

## Input model

The handheld controller is the only supported input for KiddOS navigation.
D-pad directions navigate; A confirms or activates the primary action; B
returns; Start opens the system menu where supported.

Remove physical-keyboard event handling and keyboard-only navigation paths
from the application. Do not retain them as a parallel development interface.
The on-screen Wi-Fi keyboard and PIN controls remain, operated through the
handheld controller.

## Implementation sequence

### Phase 1: Shared UI foundation

- Establish one shared 2×2 content-selection window used by Games, Movies,
  Music playlists, and TV Shows.
- Share tile geometry, artwork presentation, selection highlighting,
  empty/missing-artwork states, and controller navigation.
- Define the shared selected-media confirmation pattern: selected artwork,
  one primary action, A to proceed, and B to return.
- Clarify behavior when a catalog contains more than four items. Do not
  silently make content beyond the first four inaccessible.
- Keep state ownership authoritative and avoid duplicate UI/Application
  copies.
- Keep abstractions small and driven by the four real workflows.

### Phase 2: Games

- Refactor Games onto the shared content-selection window.
- Retain the selected-game artwork/confirmation presentation.
- Reduce the selected-game action to Play; remove Game Info from the
  child-facing workflow.
- Preserve game launch and return-to-library behavior.
- Keep the existing Remote Management catalog integration intact.
- Track the Gambatte-only launcher limitation separately.

### Phase 3: Movies

- Adopt the shared content-selection and selected-media experiences.
- Selecting a movie opens its confirmation screen; A resumes playback and
  starts from the beginning when no saved position exists.
- B returns to the movie grid.
- Preserve MPV playback and resume behavior.
- Keep movie-specific metadata and playback implementation separate.

### Phase 4: Music

- Introduce actual playlists as the managed and browsable content type.
- Show playlists in the shared 2×2 selection window, each with its own
  artwork.
- Selecting a playlist opens its confirmation screen; A starts that
  playlist and B returns to the grid.
- Remove the child-facing flat track-selection list as the primary
  browsing workflow.
- Preserve current-track embedded artwork in Now Playing, distinct from
  playlist-slot artwork.
- Do not introduce albums or collections as separate managed content types.

### Phase 5: TV Shows

- Adopt the shared content-selection and selected-media experiences.
- Selecting a show opens its confirmation screen; A resumes the saved
  episode/playback position, or starts the first appropriate episode when
  no saved progress exists.
- B returns to the show grid.
- Preserve episode progression, resume position, and controller behavior.
- Do not require children to navigate a text-heavy episode list to begin
  watching.

### Phase 6: Integration and verification

Test all four content types, including:

- Empty and populated libraries.
- More than four items and access to all catalog entries.
- Missing and valid artwork.
- Playlist artwork versus embedded track artwork.
- Selection, navigation, and controller hints; no physical-keyboard input.
- A activation and B return behavior.
- Accidental-selection confirmation before launch/playback.
- Game launch, movie resume, playlist playback, and TV resume/progression.
- Return behavior after external playback.
- Resource cleanup and repeated screen transitions.
- Parent-side content changes and catalog refresh.
- ARM64 build and on-device behavior.

Update the main architecture documentation after the implementation
accurately reflects the resulting design.

## Scope boundaries

This refactor does not authorize or require:

- A wholesale SDL rewrite.
- A new generic media framework.
- Replacing the existing firmware or emulator stack.
- Rewriting working media launchers solely for stylistic consistency.
- Changing Remote Management API contracts without a demonstrated need.
- Deploying to devices as part of the architecture audit.
- Expanding game-system scope or beginning unrelated Foundation work.
- Retaining physical-keyboard navigation as a development convenience.

Preserve existing user work, build artifacts, and unrelated untracked files.

## Completion criteria

The refactor is complete when all four content types use the same 2×2
artwork-selection experience and selected-media confirmation pattern, each
has its intended single primary action, B consistently returns, media-specific
playback behavior remains intact, navigation and state have clear ownership,
and the complete experience passes build and on-device verification.

The child-facing experience should remain simple and visual. Complexity
belongs in the implementation and parent management tools, not in the
children's browsing workflow.
