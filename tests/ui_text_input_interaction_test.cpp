#include "kiddos/ui/text_input_interaction.hpp"
#include "kiddos/ui/text_input.hpp"
#include "kiddos/ui/button.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    Container root;
    auto& input = root.add<TextInput>("Hello");
    auto& button = root.add<Button>("Continue");

    input.set_bounds({0, 0, 200, 50});
    button.set_bounds({220, 0, 200, 50});

    FocusManager focus(root);
    TextInputInteraction interaction(focus);

    assert(focus.focus_first());
    assert(focus.focused() == &input);

    assert(interaction.insert_text(" world"));
    assert(input.text() == "Hello world");

    assert(interaction.handle(TextInputKey::Home));
    assert(input.cursor_position() == 0);

    assert(interaction.insert_text("Say "));
    assert(input.text() == "Say Hello world");

    assert(interaction.handle(TextInputKey::Left));
    assert(input.cursor_position() == 3);

    assert(interaction.handle(TextInputKey::Backspace));
    assert(input.text() == "Sa Hello world");

    assert(interaction.handle(TextInputKey::Home));
    assert(interaction.handle(TextInputKey::Delete));
    assert(input.text() == "a Hello world");

    assert(interaction.handle(TextInputKey::End));
    assert(input.cursor_position() == input.text().size());

    assert(interaction.handle(TextInputKey::Left));
    assert(interaction.handle(TextInputKey::Delete));
    assert(input.text() == "a Hello worl");

    assert(focus.move(Direction::Right));
    assert(focus.focused() == &button);
    assert(!interaction.insert_text("ignored"));
    assert(!interaction.handle(TextInputKey::Backspace));

    assert(focus.move(Direction::Left));
    assert(focus.focused() == &input);

    input.set_enabled(false);
    assert(!interaction.insert_text("ignored"));
    assert(!interaction.handle(TextInputKey::Left));

    input.set_enabled(true);
    input.set_visible(false);
    assert(!interaction.insert_text("ignored"));
    assert(!interaction.handle(TextInputKey::Right));

    return 0;
}
