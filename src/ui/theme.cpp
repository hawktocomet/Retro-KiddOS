#include "kiddos/ui/theme.hpp"

namespace kiddos::ui {

Theme::Theme(ThemeColors colors) noexcept
    : colors_(colors) {
}

Theme Theme::kiddos_default() noexcept {
    return Theme{};
}

const ThemeColors& Theme::colors() const noexcept {
    return colors_;
}

void Theme::set_colors(ThemeColors colors) noexcept {
    colors_ = colors;
}

ButtonStyle Theme::button_style() const noexcept {
    ButtonStyle style;
    style.normal = colors_.surface;
    style.focused = colors_.primary_dark;
    style.pressed = colors_.primary;
    style.disabled = colors_.disabled;
    style.border = colors_.border;
    style.focused_border = colors_.primary_bright;
    style.text = colors_.text;
    style.disabled_text = colors_.text_muted;
    return style;
}

PanelStyle Theme::panel_style() const noexcept {
    PanelStyle style;
    style.background = colors_.surface;
    style.border = colors_.border;
    style.border_thickness = 2;
    return style;
}

ToggleStyle Theme::toggle_style() const noexcept {
    ToggleStyle style;
    style.off = colors_.disabled;
    style.on = colors_.primary;
    style.knob = colors_.text;
    style.focused_border = colors_.primary_bright;
    return style;
}

SliderStyle Theme::slider_style() const noexcept {
    SliderStyle style;
    style.track = colors_.disabled;
    style.fill = colors_.primary;
    style.knob = colors_.text;
    style.focused_border = colors_.primary_bright;
    return style;
}

ProgressBarStyle Theme::progress_bar_style() const noexcept {
    ProgressBarStyle style;
    style.background = colors_.disabled;
    style.fill = colors_.primary;
    style.border = colors_.border;
    style.border_thickness = 1;
    return style;
}

ListViewStyle Theme::list_view_style() const noexcept {
    ListViewStyle style;
    style.background = colors_.surface;
    style.selected = colors_.primary_dark;
    style.text = colors_.text;
    style.disabled_text = colors_.text_muted;
    style.border = colors_.border;
    return style;
}

CheckboxStyle Theme::checkbox_style() const noexcept {
    CheckboxStyle style;
    style.background = colors_.surface;
    style.checked = colors_.primary;
    style.checkmark = colors_.text;
    style.border = colors_.border;
    style.focused_border = colors_.primary_bright;
    style.text = colors_.text;
    style.disabled_text = colors_.text_muted;
    return style;
}


RadioGroupStyle Theme::radio_group_style() const noexcept {
    RadioGroupStyle style;
    style.background = colors_.surface;
    style.selected = colors_.primary;
    style.indicator = colors_.text;
    style.border = colors_.border;
    style.focused_border = colors_.primary_bright;
    style.text = colors_.text;
    style.disabled_text = colors_.text_muted;
    return style;
}


SelectStyle Theme::select_style() const noexcept {
    SelectStyle style;
    style.background = colors_.surface;
    style.border = colors_.border;
    style.focused_border = colors_.primary_bright;
    style.text = colors_.text;
    style.disabled_text = colors_.text_muted;
    style.arrow = colors_.primary_bright;
    return style;
}


} // namespace kiddos::ui
