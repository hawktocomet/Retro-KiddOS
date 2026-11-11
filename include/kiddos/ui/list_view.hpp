#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"

#include <functional>
#include <string>
#include <vector>

namespace kiddos::ui {

struct ListViewStyle {
    Color background{20, 34, 65, 255};
    Color selected{45, 125, 230, 255};
    Color text{245, 248, 255, 255};
    Color disabled_text{130, 145, 170, 255};
    Color border{65, 105, 165, 255};
};

class ListView final : public Element {
public:
    ListView();

    void set_items(std::vector<std::string> items);
    const std::vector<std::string>& items() const noexcept;

    int selected_index() const noexcept;
    bool set_selected_index(int index);
    bool move_selection(int direction);
    bool activate_selected();

    void set_row_height(int height) noexcept;
    int row_height() const noexcept;

    void set_font(const Font* font) noexcept;
    void set_style(ListViewStyle style) noexcept;
    const ListViewStyle& style() const noexcept;

    void set_on_activate(std::function<void(int)> callback);

private:
    std::vector<std::string> items_;
    int selected_index_ = -1;
    int row_height_ = 48;
    int scroll_offset_ = 0;
    const Font* font_ = nullptr;
    ListViewStyle style_{};
    std::function<void(int)> on_activate_;

    void ensure_selection_visible() noexcept;
    void draw(Renderer& renderer) override;
};

}
