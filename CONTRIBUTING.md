# Contributing to Retro KiddOS

## Development principles

- Treat `docs/PRODUCT_SPECIFICATION.md` as the central product baseline.
- Keep the FunnyPlaying Retro Pixel Pocket as the sole initial product target.
- Design handheld-first for a 720 × 720 display and physical controls.
- The Arch Linux desktop build is a temporary development harness only.
- Do not introduce a general desktop edition or desktop-first UX.
- Document meaningful decisions and test results.
- Prefer small, reviewable changes.
- Preserve existing game files and saves.
- Avoid destructive device operations without explicit approval.
- Verify hardware behavior rather than relying on assumptions.
- Preserve approved UI and interaction decisions unless a change is agreed.

## Before submitting changes

- Build the project.
- Test the behavior affected by the change.
- Test handheld behavior on actual hardware when relevant.
- Do not treat desktop simulation as proof of handheld compatibility.
- Update the product specification or relevant supporting documentation.
- Record significant changes and limitations in
  `docs/DEVELOPMENT_LOG.md`.

## Scope

KiddOS is a handheld product for the Retro Pixel Pocket. The desktop build
exists to support development and testing; it is not a product edition.

## License

A project license has not yet been selected.
