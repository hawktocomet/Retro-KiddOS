#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/texture.hpp"

namespace kiddos::ui {

enum class ImageFit {
    contain,
    cover,
    stretch
};

struct ImageGeometry {
    Rect source{};
    Rect destination{};
    bool valid = false;
};

ImageGeometry calculate_image_geometry(
    int texture_width,
    int texture_height,
    Rect bounds,
    ImageFit fit
) noexcept;

class Image final : public Element {
public:
    explicit Image(Texture* texture = nullptr) noexcept;

    void set_texture(Texture* texture) noexcept;
    Texture* texture() const noexcept;

    void set_fit(ImageFit fit) noexcept;
    ImageFit fit() const noexcept;

    ImageGeometry geometry() const noexcept;

private:
    Texture* texture_ = nullptr;
    ImageFit fit_ = ImageFit::contain;

    void draw(Renderer& renderer) override;
};

}
