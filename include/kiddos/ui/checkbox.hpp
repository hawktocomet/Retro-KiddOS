#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"

#include <functional>
#include <string>

namespace kiddos::ui {

struct CheckboxStyle {
    Color background{20, 34, 65, 255};
    Color checked{45, 125, 230, 255};
    Color checkmark{245, 248, 255, 255};
    Color border{65, 105, 165, 255};
    Color focused_border{75, 155, 255, 255};
    Color text{245, 248, 255, 255};
    Color disabled_text{160, 175, 200, 255};
};

class Checkbox final : public Element {
public:
    explicit Checkbox(std::string label = {}, bool checked = false);

    void set_label(std::string label);
    const std::string& label() const noexcept;

    void set_checked(bool checked);
    bool checked() const noexcept;
    bool toggle();

    void set_font(const Font* font) noexcept;
    const Font* font() const noexcept;

    void set_style(CheckboxStyle style) noexcept;
    const CheckboxStyle& style() const noexcept;

    void set_on_change(std::function<void(bool)> callback);

private:
    std::string label_;
    bool checked_ = false;
    const Font* font_ = nullptr;
    CheckboxStyle style_{};
    std::function<void(bool)> on_change_;

    void draw(Renderer& renderer) override;
};

}
