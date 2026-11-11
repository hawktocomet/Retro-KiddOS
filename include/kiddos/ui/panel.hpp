#pragma once

#include "kiddos/ui/element.hpp"

namespace kiddos::ui {

struct PanelStyle {
    Color background{20, 30, 55, 255};
    Color border{55, 110, 190, 255};
    int border_thickness = 2;
};

class Panel final : public Element {
public:
    Panel() = default;
    explicit Panel(PanelStyle style);

    void set_style(PanelStyle style) noexcept;
    const PanelStyle& style() const noexcept;

    void set_background(Color color) noexcept;
    void set_border(Color color) noexcept;
    void set_border_thickness(int thickness) noexcept;

private:
    PanelStyle style_{};

    void draw(Renderer& renderer) override;
};

}
