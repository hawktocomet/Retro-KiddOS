#include "kiddos/ui/theme.hpp"
#include "kiddos/ui/list_view.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Theme theme = Theme::kiddos_default();
    const ThemeColors& colors = theme.colors();

    assert(colors.background.r == 12);
    assert(colors.background.g == 20);
    assert(colors.background.b == 40);

    assert(colors.primary.r == 45);
    assert(colors.primary.g == 125);
    assert(colors.primary.b == 230);

    const ButtonStyle button = theme.button_style();
    assert(button.normal.r == colors.surface.r);
    assert(button.focused.g == colors.primary_dark.g);
    assert(button.text.r == colors.text.r);

    const PanelStyle panel = theme.panel_style();
    assert(panel.background.r == colors.surface.r);
    assert(panel.border.r == colors.border.r);

    const ToggleStyle toggle = theme.toggle_style();
    assert(toggle.on.r == colors.primary.r);
    assert(toggle.off.r == colors.disabled.r);

    const SliderStyle slider = theme.slider_style();
    assert(slider.fill.g == colors.primary.g);
    assert(slider.track.r == colors.disabled.r);

    const ProgressBarStyle progress = theme.progress_bar_style();
    assert(progress.fill.b == colors.primary.b);
    assert(progress.background.r == colors.disabled.r);

    ThemeColors alternate = colors;
    alternate.primary = {200, 100, 50, 255};
    theme.set_colors(alternate);

    assert(theme.colors().primary.r == 200);
    assert(theme.button_style().focused.r == colors.primary_dark.r);
    assert(theme.toggle_style().on.r == 200);

    const auto list_style = theme.list_view_style();
    assert(list_style.background.r == theme.colors().surface.r);
    assert(list_style.text.r == theme.colors().text.r);

    return 0;
}
