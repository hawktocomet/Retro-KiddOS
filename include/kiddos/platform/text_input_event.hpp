#pragma once

#include <string>

namespace kiddos {

enum class TextInputEventType {
    Text,
    Backspace,
    Delete,
    Left,
    Right,
    Home,
    End
};

struct TextInputEvent {
    TextInputEventType type = TextInputEventType::Text;
    std::string text;
};

} // namespace kiddos
