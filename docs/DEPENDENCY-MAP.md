# Source-Level Dependency Map

**Status:** Investigation completed; migration not started
**Date:** 2026-11-21
**Scope:** Current Retro KiddOS implementation on stock Retro Pixel Pocket firmware v1.1.3

This document records the source-level dependencies identified while separating Retro KiddOS from the future Retro Pixel Foundation.

It describes the implementation observed today and the intended ownership boundaries. It does not claim that the Foundation interfaces described here already exist.

Related documents:

- [Project Boundaries](PROJECT-BOUNDARIES.md)
- [Architecture](ARCHITECTURE.md)
- [Product and Technical Roadmap](PRODUCT-AND-TECHNICAL-ROADMAP.md)
- [Hardware Service Migration](HARDWARE-SERVICE-MIGRATION.md)

## 1. Goals

The dependency map exists to help us:

1. Identify where KiddOS currently depends on stock firmware and Linux internals.
2. Distinguish product responsibilities from platform responsibilities.
3. Preserve the working stock-firmware implementation while boundaries are introduced.
4. Define stable interfaces that could eventually be implemented by Retro Pixel Foundation.
5. Avoid moving or rewriting working code before its responsibilities are understood.
6. Give future contributors a clear picture of what belongs in each project.

The purpose is not to immediately refactor every dependency. It is to establish an accurate baseline and a safe migration direction.

## 2. Current dependency overview

Retro KiddOS currently runs as an application on the stock Retro Pixel Pocket firmware.

It uses existing firmware components and Linux facilities for hardware access, networking, power management, and game launching.

The current implementation therefore contains a mixture of:

- KiddOS product behavior.
- Direct stock-firmware integrations.
- Hardware-specific operations.
- Linux process and service management.
- Product management APIs.
- Platform status reporting.

These responsibilities are not yet fully separated.

### Current application and platform relationship

```text
Retro KiddOS application
    |
    +-- SDL frontend and screens
    |
    +-- Product services
    |     |
    |     +-- Game catalog
    |     +-- Game launcher
    |     +-- Parent management
    |     +-- PIN management
    |
    +-- Device-facing services
          |
          +-- Battery
          +-- Wi-Fi
          +-- Power
          |
          v
    Stock Retro Pixel Pocket firmware
          |
          +-- Linux userspace
          +-- ConnMan
          +-- systemd
          +-- runemu.py
          +-- sysfs
          +-- Hardware devices
          |
          v
    Retro Pixel Pocket hardware
```

This is a description of the current implementation, not the desired final architecture.

### Main dependency concerns

The most important issues identified are:

- Frontend services directly read hardware-specific sysfs paths.
- Frontend services invoke Linux commands and manage processes.
- Different components independently collect battery and network status.
- Game launching depends on the stock firmware's `runemu.py` interface.
- The management server mixes KiddOS product operations with platform status reporting.
- Hardware service code contains board-specific GPIO, input, and backlight assumptions.

These are candidates for future Foundation interfaces.

They are not reasons to destabilize the existing stock-firmware implementation.

## 3. Frontend services

The frontend service layer is located at:

```text
device/working-ui/services/
```

Current service files:

```text
BatteryService.cpp
BatteryService.hpp
GameCatalog.cpp
GameCatalog.hpp
GameLauncher.cpp
GameLauncher.hpp
ParentManagementService.cpp
ParentManagementService.hpp
PowerService.cpp
PowerService.hpp
WifiService.cpp
WifiService.hpp
```

### 3.1 BatteryService

Current public interface:

```cpp
struct BatteryReading {
    bool capacityValid = false;
    int percent = -1;
    bool externalPower = false;
};

BatteryReading readBattery();
```

The implementation reads RK817-specific sysfs paths under:

```text
/sys/devices/platform/ff180000.i2c/i2c-0/0-0020/
```

It reads battery capacity and external power state, including AC and USB online information.

**Current responsibility:** Provide battery information to the KiddOS frontend.

**Current platform dependency:** Direct access to a specific battery/charger hardware implementation through Linux sysfs.

**Future boundary:** Foundation should provide a normalized battery reading independent of the underlying battery controller and sysfs layout.

KiddOS should not need to know which battery controller is installed or where its kernel driver exposes information.

### 3.2 GameLauncher

Current public interface:

```cpp
bool launchGame(const Game& game);
```

The implementation:

- Rejects games that are unavailable or incomplete.
- Copies a staged patched Gambatte core from:

  ```text
  /storage/roms/.kiddos-ui-test-20261116/gambatte-link_libretro.so
  ```

  to:

  ```text
  /tmp/cores/gambatte_libretro.so
  ```

- Invokes:

  ```text
  /usr/bin/runemu.py
  ```

- Supplies the ROM, platform, emulator, and core arguments.
- Uses RetroArch with the Gambatte core.
- Waits for the emulator process to exit.

The current implementation intentionally hardwires Gambatte for the prototype.

This is a deliberate choice. Core licensing and distribution requirements have not yet been fully resolved, and generalizing emulator management is not necessary for the current prototype.

**Current responsibility:** Apply KiddOS game-launch rules and start the selected game.

**Current platform dependency:** Stock firmware's `runemu.py`, RetroArch installation, core paths, and process behavior.

**Future boundary:** KiddOS should request that Foundation launch supported content. Foundation should own the runtime integration and report available capabilities.

The current Gambatte behavior should remain intact until a replacement is implemented and tested.

Core selection and core management are post-MVP concerns for the remote KiddOS management experience. RetroArch's existing core distribution infrastructure should be evaluated before building a competing downloader.

### 3.3 GameCatalog

Current public interface:

```cpp
bool loadGameSlots();
```

The implementation handles KiddOS game catalog and slot state.

This is product behavior, not general hardware or operating-system functionality.

**Ownership:** Retro KiddOS.

The game catalog, child-facing slots, game curation, and game eligibility rules should remain independent of Foundation.

Foundation should not know about KiddOS slots or which games are appropriate for a particular child.

### 3.4 WifiService

Current public operations include:

```cpp
WifiReading readWifiStatus();

bool setWifiPower(bool enabled);

WifiScanResult loadWifiNetworks(
    const std::string& helperPath,
    bool scan
);

bool startWifiConnection(
    WifiConnection&,
    const std::string& helperPath,
    const WifiNetwork&,
    const std::string& password
);

WifiConnectionResult pollWifiConnection(
    WifiConnection&
);

void cancelWifiConnection(
    WifiConnection&
);
```

The implementation currently:

- Uses `connmanctl` to inspect technologies and control Wi-Fi power.
- Invokes a Python helper for scanning, listing, and connecting to networks.
- Uses `fork()` to start helper processes.
- Uses pipes and file descriptors to communicate with those processes.
- Tracks process IDs and polls for completion.
- Parses command and helper output.
- Sends connection passwords through standard input.
- Attempts to clear the password string after use.

The frontend service therefore knows about both Wi-Fi behavior and Linux process-management mechanics.

**Current responsibility:** Provide Wi-Fi operations and connection state to the KiddOS frontend.

**Current platform dependencies:**

- ConnMan.
- `connmanctl`.
- Python helper scripts.
- Linux process creation.
- Pipes and file descriptors.
- Process polling and termination.
- Stock firmware network behavior.

**Future boundary:** Foundation should expose normalized network operations and results.

A future interface should support:

- Reading network status.
- Scanning for available networks.
- Connecting to a network.
- Disconnecting from a network.
- Enabling and disabling Wi-Fi.
- Reporting connection progress.
- Reporting success and failure.
- Cancelling an in-progress operation.

The public interface should not expose process IDs, file descriptors, command syntax, or ConnMan-specific implementation details.

### 3.5 PowerService

Current public interface:

```cpp
bool requestShutdown();
```

The implementation forks a process and invokes:

```text
/bin/systemctl poweroff
```

**Current responsibility:** Request device shutdown.

**Current platform dependency:** systemd and the stock firmware's power-management environment.

**Future boundary:** Foundation should expose a platform power operation, such as requesting shutdown or reboot.

KiddOS should not need to know that systemd is responsible for carrying out the operation.

### 3.6 ParentManagementService

Current public interface:

```cpp
bool managementControl(
    const char* action,
    char* output,
    size_t outputSize
);
```

The implementation invokes:

```text
/storage/.kiddos-management/control.sh
```

and captures its output.

This is an integration with the KiddOS management system.

**Ownership:** Primarily Retro KiddOS.

The management interface may eventually consume Foundation services, but its product-specific workflows and commands should not become Foundation responsibilities.

## 4. Hardware service

The hardware service currently contains direct hardware and Linux dependencies.

Relevant files include:

```text
hardware-service/
    main.cpp
    VolumeController.cpp
    BrightnessPolicy.hpp
    VolumePolicy.hpp
```

### 4.1 VolumeController

The current implementation uses:

```text
/sys/class/gpio
/usr/bin/amixer
```

It contains GPIO and audio-control assumptions tied to the current board and firmware environment.

**Current responsibility:** Control device volume using the hardware and audio facilities available on the Retro Pixel Pocket.

**Future ownership:** Retro Pixel Foundation.

The volume capability should eventually be exposed through a stable interface that does not require applications to understand GPIO or mixer implementation details.

### 4.2 Hardware service main

The service currently interacts with:

```text
/dev/input/js0
/sys/class/backlight/backlight/brightness
/sys/class/backlight/backlight/max_brightness
```

These represent:

- Controller input.
- Display backlight control.
- Hardware-specific brightness information.

The current implementation contains assumptions about the available input device and backlight interface.

**Future ownership:** Retro Pixel Foundation.

The implementation is a candidate for Foundation, but its board-specific assumptions must be addressed before it can be described as a portable hardware service.

### 4.3 Policy functions

The following headers contain policy logic:

```text
BrightnessPolicy.hpp
VolumePolicy.hpp
```

These functions are useful candidates for independent testing because policy decisions can be separated from the hardware mechanisms used to apply them.

The distinction should be preserved:

- Policy determines what behavior is desired.
- Platform backends determine how that behavior is applied to the hardware.

The policy code should remain testable without requiring the physical device.

## 5. Management server

The management server is located at:

```text
management-server/
    server.py
    control.sh
```

It currently combines KiddOS product-management operations with device status reporting.

### 5.1 HTTP routes

Current routes include:

| Method | Route | Responsibility |
|---|---|---|
| GET | `/api/status` | Device status |
| GET | `/api/library` | Library information |
| GET | `/api/catalog` | Game catalog |
| POST | `/api/upload` | Game upload |
| POST | `/api/artwork/upload` | Artwork upload |
| POST | `/api/slots/assign` | Assign a game to a slot |
| POST | `/api/slots/unassign` | Remove a slot assignment |
| POST | `/api/games/delete` | Delete a game |

The library, catalog, upload, artwork, slot, and deletion operations are KiddOS product functionality.

### 5.2 Catalog implementation

The catalog implementation is located in:

```text
management-server/catalog.py
```

Relevant functions include:

```python
discover_games(rom_root)

_write_slots(state_root, assignments)

_load_or_initialize_slots(state_root, games)

_save_assignments(state_root, assignments)

assign_game(rom_root, state_root, slot_number, game_id, ...)

unassign_game(rom_root, state_root, slot_number)

delete_game(rom_root, state_root, game_id)

game_catalog(rom_root, state_root)
```

These functions manage game discovery, slot assignments, and catalog state.

**Ownership:** Retro KiddOS.

The catalog is not a Foundation service. Foundation should provide storage and runtime capabilities, while KiddOS decides how games are organized and assigned to children.

### 5.3 Device status

The management server currently reports battery and network information.

Its battery implementation reads:

```text
/sys/class/power_supply/battery/capacity
/sys/class/power_supply/battery/status
```

It also reads AC and USB online state.

Its network implementation invokes:

```text
iw dev wlan0 link
ip -4 -o addr show dev wlan0
```

It parses the resulting output to obtain information such as:

- Connected network name.
- Signal information.
- IP address.
- Battery capacity.
- Charging or external power state.

### 5.4 Duplicate platform implementations

There are currently multiple implementations of device status:

| Capability | Frontend | Management server |
|---|---|---|
| Battery | RK817-specific sysfs path | `/sys/class/power_supply` |
| Wi-Fi | ConnMan and helper scripts | `iw` and `ip` |
| Power | `systemctl poweroff` | Device management integration |

This creates duplicated platform knowledge.

It also means the frontend and management interface may report different results or interpret platform state differently.

**Future boundary:** Foundation should become the authoritative source of normalized device status.

Both the KiddOS frontend and management server should consume the same platform-level information rather than independently interrogating Linux and hardware.

The management server itself does not need to be wholesale replaced or split immediately. Its HTTP infrastructure can remain useful while product operations and platform status responsibilities are separated.

## 6. Stock firmware compatibility layer

The current implementation includes several integrations that exist specifically to make KiddOS work on the stock Retro Pixel Pocket firmware.

Relevant components include:

```text
firmware/boot/custom_start.sh
management-server/control.sh
device/working-ui/services/GameLauncher.cpp
device/working-ui/services/BatteryService.cpp
device/working-ui/services/WifiService.cpp
device/working-ui/services/PowerService.cpp
```

### 6.1 Startup integration

The current startup hook is:

```text
firmware/boot/custom_start.sh
```

It integrates KiddOS into the existing stock firmware startup environment.

It also uses a KiddOS-specific log location:

```text
/storage/.config/kiddos-boot.log
```

This is a compatibility adapter for the existing firmware.

It should remain intact while KiddOS continues to run on stock firmware.

### 6.2 Management control integration

The management control script is:

```text
management-server/control.sh
```

It uses stock firmware facilities, including:

```text
systemctl
/usr/bin/python3
/storage/.kiddos-management/server.py
```

This is currently part of the KiddOS management integration.

Its platform operations should eventually use Foundation interfaces where appropriate.

### 6.3 Game runtime integration

The current game launcher depends on:

```text
/usr/bin/runemu.py
```

This is a stock firmware integration.

It should be treated as an adapter between the KiddOS game-launch request and the runtime available on the current firmware.

The adapter can eventually be replaced by a Foundation runtime implementation without changing the product-level game catalog or child-facing interface.

### 6.4 Transitional status

These integrations are not a third permanent project.

They are transitional components that allow KiddOS to use the existing stock firmware while the Foundation is developed independently.

The current working device path must be preserved until a replacement has been implemented, tested, and shown to work.

## 7. Intended dependency direction

The intended relationship is:

```text
                 Retro KiddOS
       Product UI, catalog, slots, profiles,
       content policy, parent workflows
                      |
                      | Public capability interfaces
                      v
             Retro Pixel Foundation
       Device services, hardware, runtime,
       operating system, storage, networking
                      |
                      v
             Hardware and Linux
       Board, kernel, drivers, userspace
```

The dependency direction must not be reversed:

- Foundation must not depend on KiddOS.
- Foundation must not know about child profiles, game slots, game curation, or KiddOS screens.
- KiddOS should not need to know about GPIO, sysfs, systemd, ConnMan command syntax, or emulator process mechanics.
- Foundation interfaces should be usable by a different frontend or application.

## 8. Candidate platform capability contracts

These are design targets, not existing APIs.

| Capability | Candidate operations or data |
|---|---|
| Battery | Read normalized capacity, charging, and external-power state |
| Networking | Read status, scan, connect, disconnect, enable/disable |
| Power | Request shutdown or reboot |
| Display | Read and set brightness |
| Audio | Read and set volume |
| Input | Receive normalized controller events |
| Runtime | Report available capabilities and launch supported content |
| Device status | Provide normalized platform status to authorized clients |

Contracts should:

- Be independent of KiddOS product models.
- Describe capabilities and results rather than implementation commands.
- Handle unavailable hardware or services explicitly.
- Support more than one implementation where practical.
- Avoid exposing platform internals such as sysfs paths, shell commands, process IDs, and file descriptors.
- Be documented and testable independently of the KiddOS frontend.

These contracts should be developed incrementally. They should not require an immediate rewrite of the existing stock-firmware integrations.

## 9. Recommended migration sequence

The dependency map suggests the following migration order.

1. **Preserve the current implementation.** Keep the stock firmware startup path, working device behavior, and existing compatibility adapters intact.

2. **Establish the Foundation project boundary.** Define which hardware, operating-system, runtime, and device-service responsibilities belong to Foundation. Keep KiddOS product behavior outside it.

3. **Define public capability contracts.** Start with the capabilities already consumed by KiddOS: battery, networking, power, display, audio, input, and runtime.

4. **Introduce Foundation implementations behind adapters.** Replace direct Linux and hardware access incrementally. Do not change the KiddOS-facing behavior unnecessarily.

5. **Unify platform status.** Make the frontend and management server consume one authoritative source for normalized battery and network status.

6. **Validate independently.** Test Foundation capabilities without requiring KiddOS, and test KiddOS against the public interfaces rather than relying on undocumented firmware details.

7. **Retain stock compatibility until alternatives are proven.** The stock firmware adapter remains available until the Foundation implementation is working and tested on the actual device.

The migration should be driven by working capabilities, not by moving files simply to make the repository look more separated.

## 10. Non-goals and preservation rules

This investigation does not authorize:

- Rewriting the KiddOS frontend.
- Replacing the current game launcher or Gambatte prototype.
- Changing emulator cores.
- Replacing the stock firmware.
- Flashing a new device image.
- Removing working startup hooks.
- Moving large groups of files between projects.
- Replacing the management server.
- Generalizing every hardware implementation immediately.
- Changing the existing child-facing product experience.

The following rules apply during future work:

- Preserve the current stock-firmware compatibility path.
- Do not overwrite or discard existing user work.
- Do not stage or commit unrelated forensic artifacts.
- Keep hardware-specific implementation details inside platform backends.
- Keep child-facing product policy inside KiddOS.
- Make changes incrementally and review them before deployment.
- Test replacements on the actual hardware before removing existing implementations.
- Keep repository documentation synchronized with the implementation.
- Keep completed repository changes committed and pushed to GitHub.

## 11. Completion criteria

The dependency separation will be successful when:

- Retro Pixel Foundation can be built and used without Retro KiddOS.
- A different frontend can use Foundation's documented public interfaces.
- KiddOS can run without directly depending on undocumented stock firmware commands or hardware paths.
- Battery, networking, power, display, audio, and input capabilities have documented platform interfaces.
- Runtime capabilities and content launching are exposed through a documented interface.
- Platform status has one authoritative implementation.
- Foundation hardware implementations are clearly distinguished from board-specific backends.
- Foundation dependencies, build prerequisites, and licensing information are documented.
- A new contributor can build Foundation using documented, reproducible instructions.
- The existing KiddOS stock-firmware compatibility path remains available until a tested replacement exists.

The goal is not to eliminate every platform-specific implementation. Hardware support necessarily contains hardware-specific code.

The goal is to make those implementation details explicit, contained, replaceable, and accessible through stable public interfaces.
