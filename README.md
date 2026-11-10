# Retro KiddOS

A child-friendly gaming environment for the **FunnyPlaying Retro Pixel
Pocket**.

KiddOS is designed to make a handheld gaming device simple and safe for
young children, with adult management controls.

## Product

KiddOS provides one child-facing gaming experience. Its PIN-protected
Parent Center exists to configure and manage that experience; it is not a
separate adult gaming mode or frontend.

The interface is designed for the device's 720 × 720 display and physical
controls.

### Core goals

- A simple, predictable child interface.
- Parent-controlled game selection and visibility.
- An artwork-driven game carousel.
- Play, Resume, and Start Over behavior.
- Automatic gameplay checkpoints where emulator support permits.
- Parent-managed artwork and gameplay previews.
- Low-battery gameplay protection.
- Parent management of available games, features, and settings.
- Potential future controls such as playtime limits and schedules.

## Target hardware

- FunnyPlaying Retro Pixel Pocket
- Reported Rockchip RK3326
- Reported quad-core ARM processor and 1 GB RAM
- Reported 720 × 720 IPS display
- Customized EmuELEC Linux

Specifications and software compatibility must be verified on the actual
device. See [Hardware](docs/HARDWARE.md).

## Development

Initial development takes place on Arch Linux using C++20, CMake, and SDL2.

The Arch Linux build is a temporary development and testing harness. It
simulates the handheld's logical 720 × 720 display and input during
development. It is not a supported desktop edition or product target.

The handheld implementation will be integrated with the actual Retro Pixel
Pocket firmware after its software and hardware environment are inspected.

## Project status

Early development. Product requirements and architecture are being
consolidated before hardware-specific implementation.

## Documentation

Start with the [Product Specification](docs/PRODUCT_SPECIFICATION.md).

Additional requirements and design references:

- [Project Vision](docs/PROJECT_VISION.md)
- [Architecture](docs/ARCHITECTURE.md)
- [UI Design](docs/UI_DESIGN.md)
- [Input Controls](docs/INPUT_CONTROLS.md)
- [Game Library](docs/GAME_LIBRARY.md)
- [Save-State System](docs/SAVE_STATE_SYSTEM.md)
- [Media Manager](docs/MEDIA_MANAGER.md)
- [Low-Battery Protection](docs/LOW_BATTERY_PROTECTION.md)
- [Hardware Verification](docs/HARDWARE.md)
- [Development Log](docs/DEVELOPMENT_LOG.md)
- [Contributing](CONTRIBUTING.md)

## License

Not yet selected.
