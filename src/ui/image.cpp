#include "kiddos/ui/image.hpp"

#include <algorithm>
#include <cmath>

namespace kiddos::ui {

ImageGeometry calculate_image_geometry(
    int texture_width,
    int texture_height,
    Rect bounds,
    ImageFit fit
) noexcept {
    ImageGeometry result{};
    if (texture_width <= 0 || texture_height <= 0 || bounds.empty()) {
        return result;
    }

    result.source = {0, 0, texture_width, texture_height};
    result.destination = bounds;

    if (fit == ImageFit::stretch) {
        result.valid = true;
        return result;
    }

    const double sx = static_cast<double>(bounds.w) / texture_width;
    const double sy = static_cast<double>(bounds.h) / texture_height;

    if (fit == ImageFit::contain) {
        const double scale = std::min(sx, sy);
        const int w = std::max(1, static_cast<int>(std::round(texture_width * scale)));
        const int h = std::max(1, static_cast<int>(std::round(texture_height * scale)));
        result.destination = {
            bounds.x + (bounds.w - w) / 2,
            bounds.y + (bounds.h - h) / 2,
            w,
            h
        };
    } else {
        const double scale = std::max(sx, sy);
        const int crop_w = std::max(1, static_cast<int>(std::round(bounds.w / scale)));
        const int crop_h = std::max(1, static_cast<int>(std::round(bounds.h / scale)));
        result.source = {
            (texture_width - crop_w) / 2,
            (texture_height - crop_h) / 2,
            crop_w,
            crop_h
        };
    }

    result.valid = true;
    return result;
}

Image::Image(Texture* texture) noexcept : texture_(texture) {}

void Image::set_texture(Texture* texture) noexcept { texture_ = texture; }
Texture* Image::texture() const noexcept { return texture_; }

void Image::set_fit(ImageFit fit) noexcept { fit_ = fit; }
ImageFit Image::fit() const noexcept { return fit_; }

ImageGeometry Image::geometry() const noexcept {
    if (!texture_ || !texture_->valid()) return {};
    return calculate_image_geometry(
        texture_->width(), texture_->height(), bounds(), fit_
    );
}

void Image::draw(Renderer& renderer) {
    if (!texture_ || !texture_->valid()) return;
    const ImageGeometry g = geometry();
    if (!g.valid) return;

    if (fit_ == ImageFit::cover) {
        renderer.set_clip(bounds());
        renderer.draw_texture(texture_->native(), g.source, g.destination);
        renderer.clear_clip();
    } else {
        renderer.draw_texture(texture_->native(), g.source, g.destination);
    }
}

}
