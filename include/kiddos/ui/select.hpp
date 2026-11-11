#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"

#include <functional>
#include <string>
#include <vector>

namespace kiddos::ui {

struct SelectStyle {
    Color background{24, 32, 56, 255};
    Color border{90, 105, 135, 255};
    Color focused_border{80, 170, 255, 255};
    Color text{240, 244, 255, 255};
    Color disabled_text{130, 140, 160, 255};
    Color arrow{200, 215, 240, 255};
};

class Select final : public Element {
public:
    explicit Select(std::vector<std::string> options = {}, int selected = 0);

    void set_options(std::vector<std::string> options);
    const std::vector<std::string>& options() const noexcept;

    bool set_selected(int index);
    int selected() const noexcept;
    const std::string* selected_option() const noexcept;

    bool select_next();
    bool select_previous();

    void set_font(const Font* font) noexcept;
    const Font* font() const noexcept;

    void set_style(SelectStyle style) noexcept;
    const SelectStyle& style() const noexcept;

    void set_on_change(std::function<void(int)> callback);

private:
    std::vector<std::string> options_;
    int selected_{-1};
    const Font* font_{nullptr};
    SelectStyle style_{};
    std::function<void(int)> on_change_;

    void draw(Renderer& renderer) override;
};

}
