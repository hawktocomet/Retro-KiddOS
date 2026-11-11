#pragma once

#include "kiddos/ui/element.hpp"

#include <functional>
#include <string>
#include <utility>

namespace kiddos::ui {

struct ButtonStyle {
    Color normal{20, 34, 65, 255};
    Color focused{35, 83, 150, 255};
    Color pressed{18, 55, 105, 255};
    Color disabled{35, 39, 50, 255};
    Color border{69, 151, 255, 255};
    Color focused_border{125, 190, 255, 255};
};

class Button final : public Element {
public:
    explicit Button(std::string label = {});

    void set_label(std::string label);
    const std::string& label() const noexcept;

    void set_style(ButtonStyle style) noexcept;
    const ButtonStyle& style() const noexcept;

    void set_pressed(bool pressed) noexcept;
    bool pressed() const noexcept;

    void set_on_activate(std::function<void()> callback);
    bool activate();

private:
    std::string label_;
    ButtonStyle style_{};
    std::function<void()> on_activate_;
    bool pressed_ = false;

    void draw(Renderer& renderer) override;
};

} // namespace kiddos::ui
