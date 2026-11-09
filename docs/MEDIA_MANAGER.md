# Media Manager

The Media Manager is part of adding a game to Kid Mode.

## Intended workflow

1. Adult selects a game to enable in Kid Mode.
2. KiddOS identifies the game and platform.
3. Available artwork, screenshots, and gameplay previews are retrieved.
4. Adult previews available media and chooses what to display.
5. Adult may crop or position artwork and preview the handheld layout.
6. Adult confirms the selection and the game becomes visible in Kid Mode.

A one-click acceptance of automatically selected media should also be
available.

Manual import of local artwork and videos is supported by the design.

## Scope and preservation

- Do not scrape the entire installed library.
- Scrape only games being enabled for Kid Mode.
- Hiding a game retains its cached media.
- Re-enabling a game reuses existing media where possible.
- Re-scraping must not silently overwrite adult-selected media.
- Partial media availability must not prevent a game from being enabled.

## Media status

Not requested, Queued, Downloading, Ready, Partially available, Failed.

## Preview behavior

Static artwork appears first. A muted gameplay preview may begin after
approximately 2.5 seconds. The preview uses the same screen area and loops.
No emulator should run in the background to generate previews.

Media source terms, attribution, API access, and licensing must be researched
before integrating any scraper.
