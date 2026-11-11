# KiddOS UI Image Support

KiddOS UI provides a move-only `Texture` resource that loads image files through
SDL_image and owns the resulting SDL texture. The SDL renderer remains owned by
the platform layer and must outlive textures created from it.

Call `IMG_Init` for the desired image formats during application startup and
`IMG_Quit` during shutdown. SDL_image initialization is intentionally left to
the application lifecycle.

`Image` is a reusable element that holds a non-owning pointer to a `Texture`.
The texture must outlive the image element.

Supported sizing modes:

- `ImageFit::contain`: preserve the full image and fit it inside the element.
- `ImageFit::cover`: preserve aspect ratio and crop the centered source region.
- `ImageFit::stretch`: fill the element bounds without preserving aspect ratio.

The image element safely ignores missing textures and empty geometry.
