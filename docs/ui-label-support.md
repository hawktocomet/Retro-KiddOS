# KiddOS UI Label

`Label` is a reusable text element built on the shared SDL_ttf text renderer.

A label stores its text, a non-owning pointer to a `Font`, a text color, and
horizontal alignment (`left`, `center`, or `right`). Its text can be updated
without recreating the element.

The font must outlive the label. SDL_ttf initialization and shutdown remain
the responsibility of the application/platform lifecycle.
