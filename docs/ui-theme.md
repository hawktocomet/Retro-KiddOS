# KiddOS UI Theme

`Theme` centralizes semantic colors and provides consistent style presets for
the reusable UI widgets.

The default KiddOS palette uses a dark navy background, blue primary
highlights, bright focus accents, and high-contrast text.

The theme currently provides style presets for Button, Panel, Toggle, Slider,
and ProgressBar. Widgets receive styles explicitly; changing a Theme does not
silently mutate widgets that have already been created.

This allows future Kid Mode and Parent Center screens to share the same
palette while still using different theme instances when needed.
