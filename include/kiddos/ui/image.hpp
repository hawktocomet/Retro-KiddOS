#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/texture.hpp"

#include <cstdint>

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

    void set_opacity(std::uint8_t opacity) noexcept;
    std::uint8_t opacity() const noexcept;

    ImageGeometry geometry() const noexcept;

private:
    Texture* texture_ = nullptr;
    ImageFit fit_ = ImageFit::contain;
    std::uint8_t opacity_ = 255;

    void draw(Renderer& renderer) override;
};

}
