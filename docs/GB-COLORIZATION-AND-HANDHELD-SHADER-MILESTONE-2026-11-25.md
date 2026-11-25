# GB Colorization and Handheld LCD Shader Milestone — 2026-11-25

## Status

Completed, deployed, and hardware-verified on RPPBLUE
(`root@192.168.0.89`).

## Game Boy compatibility colorization

Configured Gambatte to use its GBC compatibility colorization mode:

```ini
gambatte_gb_colorization = "GBC"
This uses game-specific Game Boy Color compatibility palettes where
available and Gambatte's GBC Dark Green fallback otherwise. Game Boy Color
titles retain their native color output.

The existing palette and color-correction settings were otherwise preserved.
Shared handheld LCD shader
Selected the firmware-provided lcd1x.glsl shader.

The intended appearance is a lightweight pixel-grid treatment with subtle
horizontal scanlines, without RGB subpixel separation, CRT glow, or obvious
blur. The lcd3x variant was deliberately not selected because it includes
RGB separation.

The preset is tracked at:

device/working-ui/retroarch-profiles/handheld-lcd.glslp

It references the firmware shader at:

/usr/share/common-shaders/handheld/shaders/lcd1x.glsl

The preset uses nearest filtering and the shader's default parameters.
Launcher integration
The native GameLauncher passes RetroArch's documented --set-shader
argument when launching GB and GBC content:

/storage/.config/kiddos/retroarch/handheld-lcd.glslp

This was chosen after the profile-based video_shader setting did not load
the preset, and correcting it to video_shader_preset still did not produce
the expected visible effect in testing.

The explicit command-line option successfully loads the preset through the
normal KiddOS game-launch flow.

The launcher change is limited to the existing GB/GBC native launch path.
Controller mappings, the Start+Select exit chord, and save behavior were
not intentionally changed.
Build and deployment
- Local ARM64 build completed successfully.
- Build SHA-256:
  30ed8a27d45f5f38bf180d5247f36d389f090d0070897e3dc6909b0bf138f0c8
- Deployed using device/working-ui/deploy-kiddos-ui.sh.
- Deployment script verified the staged binary checksum and profiles.
- New KiddOS service started successfully.
- Deployment target: RPPBLUE only.
- Purple and green devices were not changed.
Hardware verification
User confirmed the result works on the device.

Verified:
- Game Boy compatibility colorization works.
- The handheld LCD shader is visibly active for Game Boy.
- The handheld LCD shader is visibly active for Game Boy Color.
- Existing native game launching and gameplay controls continue to work.
- Existing save behavior remains intact.
Future platform expansion
Treat lcd1x as the candidate shared KiddOS handheld shader for future
systems, not as a universally approved effect. Evaluate it on each platform
as support is added, especially where aspect ratio and scaling differ.

Keep per-system override or disable capability available. Do not assume
that a shader tested on GB/GBC is automatically appropriate for every system.
Deployment rollback
Executable backup:

/storage/roms/.kiddos-ui-test-20261116/kiddos-ui.pre-deploy-20261125-172039

The deployment script also created backups of the MPV input config,
GB/GBC profiles, and controller profile. No rollback was needed.
