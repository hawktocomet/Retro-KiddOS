# KiddOS UI Image Support

KiddOS UI provides a move-only `Texture` resource that loads image files through
SDL_image and owns the resulting SDL texture. The SDL renderer remains owned by
the platform layer and must outlive textures created from it.

Call `IMG_Init` for the desired image formats during application startup and
`IMG_Quit` during shutdown. SDL_image initialization is intentionally left to
the application lifecycle.

`Texture::load` accepts an optional `magenta_key` flag that treats bright
magenta (255, 0, 255) pixels as transparent. This matches the generated
Kid Mode artwork, which is exported with a magenta background. Loaded
textures use alpha blending.

Artwork that must not show magenta keying fringes on anti-aliased edges
should be exported with a transparent background instead (PNG) and loaded
without the magenta key. The home carousel uses transparent PNG exports of
the same SVG sources for this reason.

`Image` is a reusable element that holds a non-owning pointer to a `Texture`.
The texture must outlive the image element.

Supported sizing modes:

- `ImageFit::contain`: preserve the full image and fit it inside the element.
- `ImageFit::cover`: preserve aspect ratio and crop the centered source region.
- `ImageFit::stretch`: fill the element bounds without preserving aspect ratio.

The image element safely ignores missing textures and empty geometry.
