#pragma once
#include "kiddos/ui/color.hpp"
#include "kiddos/ui/font.hpp"
#include "kiddos/ui/geometry.hpp"
#include "kiddos/ui/renderer.hpp"
#include <string>

namespace kiddos::ui {
class TextRenderer final {
public:
    explicit TextRenderer(Renderer& renderer) noexcept;
    bool draw(const std::string& text, const Font& font,
              Point position, Color color);
    bool draw_centered(const std::string& text, const Font& font,
                       Rect area, Color color);
private:
    Renderer& renderer_;
};
}
