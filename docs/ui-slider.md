# KiddOS UI Slider

`Slider` is a focusable numeric control with a configurable minimum,
maximum, current value, and adjustment step.

The controller's left and right actions adjust a focused slider. Other
controls retain normal directional focus navigation. Value changes invoke
the registered callback only when the value actually changes.

Values are clamped to the configured range. The range is normalized if its
endpoints are supplied in reverse order.
