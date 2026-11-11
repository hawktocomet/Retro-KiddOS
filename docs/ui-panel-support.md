# KiddOS UI Panel

`Panel` is a reusable rectangular visual element.

Its style includes a background color, border color, and border thickness.
A zero border thickness disables the border. Negative thickness values are
clamped to zero.

Panels draw only their own background and border. They do not own child
elements; use the existing `Container` for element ownership and layout.

## Border rendering

For an opaque background the border is drawn as a clean, uniform ring: the
panel is first filled with the border color, then the area inset by the
border thickness is filled with the background color. This avoids stacked
nested outlines, which look crude at small corner radii.

For a transparent background (for example a glow outline around a selected
card) the border is drawn as rounded outlines instead, so the content
beneath the panel stays visible.

Rounded corners are rasterized by the shared renderer helpers: fills sample
the corner arc at pixel centers, and outlines trace the true circle with
dense, deduplicated angle steps, so borders stay continuous and are never
blended twice on the same pixel.
