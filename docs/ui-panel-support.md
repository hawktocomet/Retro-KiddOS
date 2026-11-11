# KiddOS UI Panel

`Panel` is a reusable rectangular visual element.

Its style includes a background color, border color, and border thickness.
A zero border thickness disables the border. Negative thickness values are
clamped to zero.

Panels draw only their own background and border. They do not own child
elements; use the existing `Container` for element ownership and layout.
