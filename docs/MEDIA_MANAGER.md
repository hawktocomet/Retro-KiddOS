# Media Manager

The Media Manager is part of the parent workflow for adding a game to
the child-facing KiddOS library.

## Intended workflow

1. A parent selects a game to enable in KiddOS.
2. KiddOS identifies the game and platform.
3. Available artwork, screenshots, and gameplay previews are retrieved.
4. The parent previews available media and chooses what to display.
5. The parent may crop or position artwork and preview the handheld layout.
6. The parent confirms the selection and the game becomes visible to the child.

A one-click acceptance of automatically selected media should also be
available.

Manual import of local artwork and videos is supported by the design.

## Scope and preservation

- Do not scrape the entire installed library.
- Scrape only games being enabled for the child-facing library.
- Hiding a game retains its cached media.
- Re-enabling a game reuses existing media where possible.
- Re-scraping must not silently overwrite parent-selected media.
- Partial media availability must not prevent a game from being enabled.

## Media status

Not requested, Queued, Downloading, Ready, Partially available, Failed.

## Preview behavior

Static artwork appears first. A muted gameplay preview may begin after
approximately 2.5 seconds. The preview uses the same screen area and loops.
No emulator should run in the background to generate previews.

Media source terms, attribution, API access, and licensing must be researched
before integrating any scraper.
