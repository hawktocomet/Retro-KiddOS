#pragma once

#include "kiddos/ui/button.hpp"
#include "kiddos/ui/panel.hpp"
#include "kiddos/ui/progress_bar.hpp"
#include "kiddos/ui/slider.hpp"
#include "kiddos/ui/toggle.hpp"

namespace kiddos::ui {

struct ThemeColors {
    Color background{12, 20, 40, 255};
    Color surface{20, 34, 65, 255};
    Color surface_alt{27, 43, 76, 255};
    Color primary{45, 125, 230, 255};
    Color primary_bright{75, 155, 255, 255};
    Color primary_dark{25, 75, 145, 255};
    Color text{245, 248, 255, 255};
    Color text_muted{160, 175, 200, 255};
    Color disabled{45, 50, 65, 255};
    Color border{65, 105, 165, 255};
};

class Theme final {
public:
    Theme() = default;
    explicit Theme(ThemeColors colors) noexcept;

    static Theme kiddos_default() noexcept;

    const ThemeColors& colors() const noexcept;
    void set_colors(ThemeColors colors) noexcept;

    ButtonStyle button_style() const noexcept;
    PanelStyle panel_style() const noexcept;
    ToggleStyle toggle_style() const noexcept;
    SliderStyle slider_style() const noexcept;
    ProgressBarStyle progress_bar_style() const noexcept;

private:
    ThemeColors colors_{};
};

}
