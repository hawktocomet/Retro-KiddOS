#pragma once

#include "kiddos/ui/element.hpp"
#include "kiddos/ui/font.hpp"

#include <cstddef>
#include <functional>
#include <string>

namespace kiddos::ui {

struct TextInputStyle {
    Color background{20, 30, 52, 255};
    Color border{75, 95, 130, 255};
    Color focused_border{65, 155, 255, 255};
    Color text{240, 244, 255, 255};
    Color placeholder{135, 150, 175, 255};
    Color cursor{90, 180, 255, 255};
    Color disabled_text{125, 135, 155, 255};
};

class TextInput final : public Element {
public:
    explicit TextInput(std::string text = {});

    void set_text(std::string text);
    const std::string& text() const noexcept;

    void set_placeholder(std::string placeholder);
    const std::string& placeholder() const noexcept;

    void set_max_length(std::size_t length);
    std::size_t max_length() const noexcept;

    bool insert_text(const std::string& text);
    bool backspace();
    bool delete_forward();
    void move_cursor_left() noexcept;
    void move_cursor_right() noexcept;
    void move_cursor_home() noexcept;
    void move_cursor_end() noexcept;

    std::size_t cursor_position() const noexcept;
    void set_cursor_position(std::size_t position) noexcept;

    void set_font(const Font* font) noexcept;
    const Font* font() const noexcept;

    void set_style(TextInputStyle style) noexcept;
    const TextInputStyle& style() const noexcept;

    void set_on_change(std::function<void(const std::string&)> callback);

private:
    std::string text_;
    std::string placeholder_;
    std::size_t cursor_{0};
    std::size_t max_length_{256};
    const Font* font_{nullptr};
    TextInputStyle style_{};
    std::function<void(const std::string&)> on_change_;

    static std::size_t previous_boundary(const std::string& text,
                                         std::size_t position) noexcept;
    static std::size_t next_boundary(const std::string& text,
                                     std::size_t position) noexcept;
    static std::size_t count_codepoints(const std::string& text) noexcept;

    void notify_change();
    void draw(Renderer& renderer) override;
};

}
