#pragma once

#include kiddos/platform/platform.hpp

namespace kiddos {

class Application {
public:
    explicit Application(Platform& platform);

    int run();

private:
    Platform& platform_;
    bool running_ = true;

    void handle_action(Action action);
    void render();
};

} // namespace kiddos
