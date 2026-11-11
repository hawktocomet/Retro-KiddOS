# Game Artwork Research References

These are visual research examples for the initial nine-platform KiddOS library.
They are not approved redistribution assets and must not be copied into a
release, ROM pack, or installed game library without confirming the relevant
rights and source terms.

| Platform | Representative game | Example artwork | Source |
|---|---|---|---|
| NES | Super Mario Bros. 3 | Cartridge front | https://cdn11.bigcommerce.com/s-kzjsut/images/stencil/1280x1280/products/1926/7761/Super-Mario-Bros_1__60712.1585850034.jpg?c=2%3Fimbypass%3Don |
| SNES | Super Mario World | Cartridge front | https://dickspawn.com/cdn/shop/files/Super-Mario-World---Super-Nintendo_2048x.jpg?v=1731001355 |
| Genesis | Sonic the Hedgehog 2 | Cartridge front | https://down-br.img.susercontent.com/file/br-11134201-7r98o-lu5w90dxrnhob7 |
| GB | Tetris | Cartridge front | https://superretro.com.au/cdn/shop/products/tetris-game-boy-104428-388271_1600x.jpg?v=1681383136 |
| GBC | Pokémon Crystal | Cartridge front | https://theexchange.com/cdn/shop/products/Pokemon-CrystalVersion_USA_1024x1024.png?v=1618596543 |
| GBA | Metroid Fusion | Cartridge front | https://media.gamestop.com/i/gamestop/10022264/Metroid-Fusion---Game-Boy-Advance |
| Arcade (FB Neo) | Metal Slug | Arcade artwork reference | https://i.pinimg.com/originals/6a/9c/22/6a9c2282afc06843a98c0856c45a7677.jpg |
| Neo Geo Pocket Color | Sonic the Hedgehog Pocket Adventure | Cartridge front | https://i.ebayimg.com/images/g/4WYAAOSwNktfH6kq/s-l1600.jpg |
| Master System | Alex Kidd in Miracle World | Cartridge front | https://static.gamesmen.com.au/media/catalog/product/cache/57ddbad6affa8d28869fa47188b75842/a/l/alex_kidd_in_miracle_world_sega_master_system_pre-owned_cartridge_1_.jpg |

## Artwork direction

- Prefer real cartridge scans when a suitable, rights-cleared source exists.
- Where cartridge scans are unavailable, use box art, title screens, or
  representative game screenshots rather than generic platform icons.
- Arcade titles may need a distinct treatment: cabinet art, marquees, flyers,
  or title screens rather than forcing them into a console-cartridge template.
- Keep each platform's artwork aspect ratio and silhouette intact.
- The selected game should be larger, sharper, and subtly lifted from the
  background; neighboring items should be quieter without becoming unreadable.
- Do not put every piece of art inside an identical thick rectangular card.

## Future metadata and scraping research

ScreenScraper documents media categories such as box art, flyers, screenshots,
title screens, and marquees. Its API documentation does not establish that
cartridge scans are available for every platform. Treat cartridge imagery as
a source-by-source discovery task.

- ScreenScraper: https://www.screenscraper.fr/
- ScreenScraper API documentation: https://www.screenscraper.fr/webapi2.php
- FBNeo project: https://github.com/finalburnneo/FBNeo

Before implementing an automatic scraper, document API access requirements,
rate limits, attribution expectations, and the rights/terms for each media
source. Parent-enabled games should be the only games sent through the
eventual media-scraping workflow.
