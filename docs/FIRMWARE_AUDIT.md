# Manufacturer Firmware Audit

**Project:** Retro KiddOS  
**Audit date:** 2026-11-10  
**Repository:** https://github.com/FRetroPixel/RetroPixel_Pocket  
**Audited revision:** `13e1c48d86c95ae2b8a99d6baf02e4e394629cb3`  
**Revision date:** 2025-12-02  
**Local checkout:** `research/firmware/manufacturer/RetroPixel_Pocket`

## 1. Purpose and scope

This audit examines the publicly available manufacturer firmware source to
identify reusable platform components and gaps relevant to Retro KiddOS.

The objective is to learn from the existing firmware without assuming that
its inherited device utilities are compatible with the Retro Pixel Pocket.

This is a source-code audit. The physical device has not yet been inspected,
and no firmware has been built or flashed as part of this work.

## 2. Firmware lineage and build structure

The manufacturer repository is based on the EmuELEC/AmberELEC family of
firmware projects.

The manufacturer adds a Rockchip device target named `RPPOCKET`.

Relevant source locations:

- `projects/Rockchip/devices/RPPOCKET/options`
- `projects/Rockchip/devices/RPPOCKET/linux/linux.aarch64.conf`
- `projects/Rockchip/packages/linux/package.mk`
- `projects/Rockchip/packages/u-boot/package.mk`
- `projects/Rockchip/bootloader/install`
- `projects/Rockchip/bootloader/release`

The RPPOCKET build target is not a completely independent firmware
implementation. It uses existing platform infrastructure and inherited
package and service conventions.

The checked-out repository does not contain all external build dependencies.
In particular, its Linux and U-Boot package definitions refer to external
source directories:

- `extpackage/linux-kernel-rppocket`
- `extpackage/u-boot-rppocket`

Those sources were not present in the inspected checkout. They must be
obtained and audited separately before making source-level claims about the
complete kernel or bootloader implementation.

## 3. Target configuration

The RPPOCKET options select the following build configuration:

| Setting | Configured value |
|---|---|
| Target architecture | ARM64 |
| CPU target | Cortex-A35 |
| CPU features | CRC, FP, SIMD, crypto |
| Kernel image | `Image` |
| Device tree | `rockchip/rk3326-rppocket_v6_0-linux.dtb` |
| GPU family | Mali G31 |
| OpenGL ES implementation | `libmali` |
| Console | `ttyFIQ0,115200n8` |
| Linux platform identifier | `RG351MP` |
| U-Boot system identifier | `rg351mp` |

The configuration also contains an ARM build branch.

**Interpretation:** The checked-in target configuration is strong evidence
that the manufacturer builds for an RK3326-oriented platform and selects a
specific Retro Pixel Pocket device tree. It is not independent confirmation
of the physical device's silicon or board revision. Those details must be
verified on the actual hardware.

The inherited `RG351MP` identifiers are a warning against assuming that
every generic platform script or path is correct for this device.

## 4. Kernel configuration capabilities

The ARM64 kernel configuration enables several useful platform facilities.

### Display and graphics

Relevant configuration includes:

- Rockchip DRM/KMS support
- MIPI DSI support
- Panel support
- Mali Bifrost Linux driver configuration
- Framebuffer compatibility
- Backlight class and PWM support
- Rockchip RGA2 support

The device options select `libmali` as the OpenGL ES implementation.

**KiddOS implication:** There is a promising Linux graphics foundation for
a native SDL2 interface. We still need to verify the actual display device,
available DRM nodes, Mali userspace libraries, SDL video backend, and
rendering performance on the hardware.

### Input

Relevant configuration includes:

- Linux input subsystem
- evdev
- joydev
- ADC joystick support
- ODROID GO 3 joystick driver
- XU10 joystick driver
- GPIO input support
- Rockchip keyboard support

SARADC support is also enabled.

**KiddOS implication:** Linux input interfaces should provide a foundation
for physical controls. The actual event devices, button mappings, analog
ranges, and repeat behavior must be discovered on the device. We should
build our own input mapping layer rather than assume inherited event paths.

### Battery and charging

Relevant configuration includes:

- Power supply subsystem
- GPIO charger support
- RK817 battery support
- RK817 charger support

**KiddOS implication:** The kernel configuration anticipates battery and
charging integration. We need to inspect the actual power-supply devices,
their reported properties, and charging behavior before implementing
battery UI or protection logic.

### Thermal management

Relevant configuration includes:

- Generic thermal framework
- Rockchip thermal support
- CPU thermal support
- Devfreq thermal support

**KiddOS implication:** The kernel has thermal-management facilities.
Actual sensor names, temperature readings, cooling devices, and throttling
behavior remain to be verified.

## 5. Existing utilities and services

The manufacturer firmware includes an `odroidgoa-utils` package with
scripts and systemd services.

These are useful examples of firmware integration, but several scripts
contain explicit device checks for other handhelds.

### Battery utility

`odroidgoa-utils/sources/battery.sh`

The script:

- Reads battery capacity and status from
  `/sys/class/power_supply/battery/`.
- Controls GPIO77 for power LED behavior.
- Polls battery state every 15 seconds.
- Uses several capacity thresholds for LED behavior.

A systemd service starts this script when the expected battery capacity
path exists.

**Assessment:** The battery interface is plausible, but the GPIO number,
LED behavior, service policy, and sysfs path must be verified. KiddOS
should use discovered Linux power-supply and LED interfaces rather than
copy these assumptions.

### Headphone detection

`odroidgoa-utils/sources/headphone_sense.sh`

The script explicitly handles:

- `SZDiiER D007 Plus`
- `PowKiddy Magicx XU10`

It uses model-specific GPIOs, fixed input event paths, mixer controls,
and EmuELEC settings commands.

There is no RPPOCKET-specific branch.

**Assessment:** This script does not provide a working RPPOCKET headphone
implementation based on the inspected source. Its general approach is
useful, but the actual headphone event and audio-routing interfaces must
be discovered.

### Joystick LED handling

`odroidgoa-utils/sources/joyled.sh`

The script only runs its LED behavior when the device-tree model is
`SZDiiER D007 Plus`.

It assumes:

- An ODROID GO 3 joystick path
- Specific joystick axis numbering
- `left_joystick` and `right_joystick` LED devices

There is no RPPOCKET-specific implementation.

**Assessment:** Do not reuse this script as the basis for KiddOS input or
LED support.

### ADC key handling

The ADC key scripts contain device-specific checks and assumptions about
input paths and event codes. They are not a generic input abstraction.

**Assessment:** Treat these as inherited platform utilities, not as a
reliable button mapping specification for the Retro Pixel Pocket.

### Volume and brightness controls

The volume sensing and helper scripts use fixed event-device assumptions,
EmuELEC settings helpers, ALSA mixer controls, and a fixed backlight path.

**Assessment:** The underlying Linux interfaces are relevant, but the
implementation should be replaced with a device-independent KiddOS
hardware adapter after runtime discovery.

### Sleep and resume

The `enable-oga-sleep` package includes suspend configuration and a
pre-sleep/post-resume script.

The script saves and restores brightness and ALSA state, unloads and
reloads `dwc2`, and stops and restarts headphone and volume services.

**Assessment:** Existing firmware support demonstrates that suspend and
resume are part of the platform's intended behavior. The particular script
contains assumptions about backlight paths, services, storage, and USB
module handling. KiddOS should implement and test its own suspend/resume
sequence rather than adopting it unchanged.

## 6. RPPOCKET-specific build references

The inspected source contains RPPOCKET references in:

- Rockchip bootloader release selection
- Rockchip `rkbin` package selection
- U-Boot package selection
- Kernel device-tree selection
- Bootloader installation configuration

The bootloader installation configuration references:

`/extlinux/rk3326-rppocket_v6_0-linux.dtb.conf`

The kernel and U-Boot package definitions point to external source
directories, so the full implementation cannot yet be audited from this
checkout alone.

## 7. Implications for Retro KiddOS

The existing firmware provides a useful Linux foundation. KiddOS should
reuse platform capabilities where appropriate, but own its application
behavior and hardware integration.

### KiddOS should own

- Child-facing interface and navigation
- Parent Center and access control
- Game library and metadata
- Game launching and return-to-shell behavior
- Save management and save policies
- Media interface
- Audio and display settings UI
- Low-battery user experience
- Input mapping and navigation behavior
- Application lifecycle and recovery behavior

### KiddOS should integrate with Linux for

- Display and graphics
- Physical input
- Audio output and volume
- Battery and charging state
- Backlight control
- Thermal monitoring
- Suspend and resume
- Storage and filesystem access

### Recommended architecture direction

Create a small platform abstraction layer between KiddOS UI/application
code and Linux hardware interfaces.

The application should not directly depend on fixed event numbers, GPIO
numbers, sysfs paths, or manufacturer-specific helper scripts.

Hardware discovery should inform device adapters for:

- Display
- Input
- Audio
- Battery and charging
- Backlight
- LEDs, if present
- Thermal monitoring
- Power management

The desktop SDL2 build remains a development harness. The target firmware
integration must be tested on the actual handheld.

## 8. Runtime verification required

When the Retro Pixel Pocket arrives, collect the following before writing
hardware-specific adapters:

1. Device-tree model and compatible strings
2. Kernel version and boot command line
3. Display devices, DRM nodes, and available graphics drivers
4. SDL video drivers and renderer availability
5. Input devices, event capabilities, button mappings, and analog ranges
6. Power-supply devices and reported battery/charging properties
7. Backlight and LED interfaces
8. ALSA cards, playback devices, mixer controls, and headphone behavior
9. Thermal zones and cooling devices
10. Suspend/resume behavior
11. Storage devices, partition layout, and mount behavior
12. Existing game launcher, frontend, and service startup sequence

These checks should be read-only wherever possible. Do not flash firmware,
repartition storage, or change boot configuration as part of initial
discovery.

## 9. Audit conclusions

| Area | Source-level finding | Confidence |
|---|---|---|
| RPPOCKET build target | Explicit target and device-tree selection exist | High |
| ARM64 configuration | Explicit Cortex-A35 ARM64 build settings | High |
| Graphics foundation | DRM/KMS, Mali, RGA2 and display support configured | High |
| Input foundation | Linux input, ADC and joystick drivers configured | High |
| Battery foundation | RK817 battery and charger support configured | High |
| Thermal foundation | Rockchip thermal support configured | High |
| Headphone utility | No RPPOCKET branch in inspected script | High |
| Joystick LED utility | Only handles D007 Plus | High |
| Suspend/resume | Generic inherited implementation exists | High |
| Physical hardware details | Not independently verified | Pending |
| Actual graphics/input/audio behavior | Requires device testing | Pending |
| Complete kernel/U-Boot source | External source directories not present | Pending |

**Overall conclusion:** The manufacturer firmware is a useful starting point
for understanding the target Linux platform, but it is not a ready-made
KiddOS implementation. We should build KiddOS as its own application and
hardware integration layer, using the manufacturer firmware as a reference
for boot, kernel, and platform configuration.
