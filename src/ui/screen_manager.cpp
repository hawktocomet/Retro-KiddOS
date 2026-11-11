#include "kiddos/ui/screen_manager.hpp"
namespace kiddos::ui {
ScreenManager::~ScreenManager() { clear(); }
bool ScreenManager::push(std::unique_ptr<Screen> screen) {
    if (!screen) return false;
    if (auto* active = current()) active->on_suspend();
    auto* next = screen.get();
    stack_.push_back(std::move(screen));
    next->on_enter();
    next->focus().focus_first();
    return true;
}
bool ScreenManager::pop() {
    if (stack_.empty()) return false;
    stack_.back()->on_exit();
    stack_.pop_back();
    if (auto* active = current()) { active->on_resume(); active->focus().focus_first(); }
    return true;
}
bool ScreenManager::replace(std::unique_ptr<Screen> screen) {
    if (!screen) return false;
    if (auto* active = current()) { active->on_exit(); stack_.pop_back(); }
    auto* next = screen.get();
    stack_.push_back(std::move(screen));
    next->on_enter();
    next->focus().focus_first();
    return true;
}
void ScreenManager::clear() noexcept {
    while (!stack_.empty()) { stack_.back()->on_exit(); stack_.pop_back(); }
}
Screen* ScreenManager::current() noexcept { return stack_.empty() ? nullptr : stack_.back().get(); }
const Screen* ScreenManager::current() const noexcept { return stack_.empty() ? nullptr : stack_.back().get(); }
std::size_t ScreenManager::depth() const noexcept { return stack_.size(); }
void ScreenManager::update(float dt) { if (auto* s=current()) s->update(dt); }
void ScreenManager::render(Renderer& r) { if (auto* s=current()) s->render(r); }
bool ScreenManager::handle_action(UIAction a) {
    if (a == UIAction::Back) return pop();
    auto* s=current(); return s ? s->handle_action(a) : false;
}
bool ScreenManager::handle_text_input(const ::kiddos::TextInputEvent& e) {
    auto* s=current(); return s ? s->handle_text_input(e) : false;
}
}
