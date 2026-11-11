#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"
#include "kiddos/ui/renderer.hpp"

#include <functional>
#include <string>
#include <vector>

namespace kiddos::ui {

struct RadioGroupStyle {
    Color background{24, 32, 56, 255};
    Color selected{50, 130, 240, 255};
    Color indicator{255, 255, 255, 255};
    Color border{90, 105, 135, 255};
    Color focused_border{80, 170, 255, 255};
    Color text{240, 244, 255, 255};
    Color disabled_text{130, 140, 160, 255};
};

class RadioGroup final : public Element {
public:
    explicit RadioGroup(std::vector<std::string> options = {}, int selected = 0);

    void set_options(std::vector<std::string> options);
    const std::vector<std::string>& options() const noexcept;

    bool set_selected(int index);
    int selected() const noexcept;
    bool select_next();
    bool select_previous();

    void set_font(const Font* font) noexcept;
    const Font* font() const noexcept;

    void set_style(RadioGroupStyle style) noexcept;
    const RadioGroupStyle& style() const noexcept;

    void set_on_change(std::function<void(int)> callback);
    void draw(Renderer& renderer) override;

private:
    std::vector<std::string> options_;
    int selected_{-1};
    const Font* font_{nullptr};
    RadioGroupStyle style_{};
    std::function<void(int)> on_change_;
};

}
