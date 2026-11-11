#include "kiddos/ui/text_input.hpp"
#include "kiddos/ui/theme.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace kiddos::ui;

    TextInput input("Hello");
    assert(input.focusable());
    assert(input.text() == "Hello");
    assert(input.cursor_position() == 5);

    int changes = 0;
    std::string latest;
    input.set_on_change([&](const std::string& value) {
        ++changes;
        latest = value;
    });

    assert(input.insert_text(" world"));
    assert(input.text() == "Hello world");
    assert(latest == "Hello world");

    input.move_cursor_home();
    assert(input.cursor_position() == 0);
    assert(input.insert_text("Say "));
    assert(input.text() == "Say Hello world");

    input.move_cursor_left();
    assert(input.backspace());
    assert(input.text() == "Say Hello world");

    input.move_cursor_home();
    assert(input.delete_forward());
    assert(input.text() == "ay Hello world");

    input.set_cursor_position(999);
    assert(input.cursor_position() == input.text().size());
    input.move_cursor_end();
    assert(!input.delete_forward());

    input.set_max_length(5);
    assert(input.text() == "ay He");
    assert(input.max_length() == 5);

    input.set_text("123456");
    assert(input.text() == "12345");
    input.set_max_length(20);
    input.set_text("A");
    assert(input.text() == "A");

    input.set_placeholder("Enter name");
    assert(input.placeholder() == "Enter name");

    input.set_enabled(false);
    assert(!input.insert_text("B"));
    assert(!input.backspace());
    input.set_enabled(true);
    input.set_visible(false);
    assert(!input.insert_text("B"));
    input.set_visible(true);

    TextInput unicode("Aé🙂");
    unicode.move_cursor_home();
    unicode.move_cursor_right();
    assert(unicode.cursor_position() == 1);
    unicode.move_cursor_right();
    assert(unicode.backspace());
    assert(unicode.text() == "A🙂");

    TextInputStyle style;
    style.background = {1, 2, 3, 255};
    input.set_style(style);
    assert(input.style().background.r == 1);

    Theme theme = Theme::kiddos_default();
    const TextInputStyle themed = theme.text_input_style();
    assert(themed.background.r == theme.colors().surface.r);
    assert(themed.cursor.b == theme.colors().primary_bright.b);

    return 0;
}
