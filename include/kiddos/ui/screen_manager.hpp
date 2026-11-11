#pragma once
#include "kiddos/ui/screen.hpp"
#include <memory>
#include <vector>
namespace kiddos::ui {
class ScreenManager final {
public:
    ~ScreenManager();
    ScreenManager() = default;
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;
    bool push(std::unique_ptr<Screen> screen);
    bool pop();
    bool replace(std::unique_ptr<Screen> screen);
    void clear() noexcept;
    Screen* current() noexcept;
    const Screen* current() const noexcept;
    std::size_t depth() const noexcept;
    void update(float delta_seconds);
    void render(Renderer& renderer);
    bool handle_action(UIAction action);
    bool handle_text_input(const ::kiddos::TextInputEvent& event);
private:
    std::vector<std::unique_ptr<Screen>> stack_;
};
}
