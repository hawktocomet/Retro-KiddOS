#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"

#include <string>

namespace kiddos::ui {

enum class LabelAlignment {
    left,
    center,
    right
};

class Label final : public Element {
public:
    explicit Label(std::string text = {});

    void set_text(std::string text);
    const std::string& text() const noexcept;

    void set_font(const Font* font) noexcept;
    const Font* font() const noexcept;

    void set_color(Color color) noexcept;
    Color color() const noexcept;

    void set_alignment(LabelAlignment alignment) noexcept;
    LabelAlignment alignment() const noexcept;

private:
    std::string text_;
    const Font* font_ = nullptr;
    Color color_{255, 255, 255, 255};
    LabelAlignment alignment_ = LabelAlignment::left;

    void draw(Renderer& renderer) override;
};

}
