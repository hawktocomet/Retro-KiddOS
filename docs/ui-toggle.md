# KiddOS UI Toggle

`Toggle` is a focusable on/off control for settings and management screens.

It supports checked state, a change callback, and configurable off, on,
knob, and focus-border colors. Hidden or disabled toggles cannot be changed
through `toggle()`.

The controller Confirm action activates either a focused Button or a focused
Toggle. Toggle state changes invoke the registered callback only when the
state actually changes.
