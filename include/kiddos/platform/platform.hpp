#pragma once

#include "kiddos/core/action.hpp"

namespace kiddos {

class Platform {
public:
    virtual ~Platform() = default;

    virtual bool initialize() = 0;
    virtual bool poll_action(Action& action) = 0;
    virtual void begin_frame() = 0;
    virtual void draw_carousel(int selected_index) = 0;
    virtual void present() = 0;
    virtual void shutdown() = 0;
};

} // namespace kiddos
