#include "kiddos/platform/text_input_event.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace kiddos;

    TextInputEvent text{TextInputEventType::Text, "Hello 世界"};
    assert(text.type == TextInputEventType::Text);
    assert(text.text == "Hello 世界");

    const TextInputEventType editing_keys[] = {
        TextInputEventType::Backspace,
        TextInputEventType::Delete,
        TextInputEventType::Left,
        TextInputEventType::Right,
        TextInputEventType::Home,
        TextInputEventType::End
    };

    for (const auto key : editing_keys) {
        TextInputEvent event{key, {}};
        assert(event.type == key);
        assert(event.text.empty());
    }

    return 0;
}
