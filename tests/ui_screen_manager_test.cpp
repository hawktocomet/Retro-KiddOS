#include "kiddos/ui/screen_manager.hpp"

#include <cassert>
#include <memory>

struct Counts {
    int entered = 0;
    int exited = 0;
    int suspended = 0;
    int resumed = 0;
    int activated = 0;
};

class ProbeScreen final : public kiddos::ui::Screen {
public:
    explicit ProbeScreen(Counts& counts) : counts_(counts) {}

    void on_enter() override {
        ++counts_.entered;
        auto& button = root().add<kiddos::ui::Button>("Activate");
        button.set_on_activate([this] { ++counts_.activated; });
    }

    void on_exit() override { ++counts_.exited; }
    void on_suspend() override { ++counts_.suspended; }
    void on_resume() override { ++counts_.resumed; }

private:
    Counts& counts_;
};

int main() {
    using namespace kiddos::ui;

    ScreenManager manager;
    Counts first_counts;
    Counts second_counts;
    Counts third_counts;
    Counts fourth_counts;

    assert(manager.depth() == 0);
    assert(manager.current() == nullptr);
    assert(!manager.pop());
    assert(!manager.handle_action(UIAction::Back));

    auto first = std::make_unique<ProbeScreen>(first_counts);
    auto* first_ptr = first.get();
    assert(manager.push(std::move(first)));
    assert(manager.current() == first_ptr);
    assert(manager.depth() == 1);
    assert(first_counts.entered == 1);

    assert(manager.handle_action(UIAction::Confirm));
    assert(first_counts.activated == 1);

    auto second = std::make_unique<ProbeScreen>(second_counts);
    auto* second_ptr = second.get();
    assert(manager.push(std::move(second)));
    assert(manager.current() == second_ptr);
    assert(manager.depth() == 2);
    assert(first_counts.suspended == 1);
    assert(second_counts.entered == 1);

    assert(manager.handle_action(UIAction::Confirm));
    assert(second_counts.activated == 1);

    assert(manager.handle_action(UIAction::Back));
    assert(manager.current() == first_ptr);
    assert(manager.depth() == 1);
    assert(second_counts.exited == 1);
    assert(first_counts.resumed == 1);

    auto third = std::make_unique<ProbeScreen>(third_counts);
    auto* third_ptr = third.get();
    assert(manager.replace(std::move(third)));
    assert(manager.current() == third_ptr);
    assert(manager.depth() == 1);
    assert(first_counts.exited == 1);
    assert(third_counts.entered == 1);

    auto fourth = std::make_unique<ProbeScreen>(fourth_counts);
    assert(manager.push(std::move(fourth)));
    assert(manager.depth() == 2);
    assert(third_counts.suspended == 1);

    manager.clear();
    assert(manager.depth() == 0);
    assert(manager.current() == nullptr);
    assert(fourth_counts.exited == 1);
    assert(third_counts.exited == 1);

    assert(!manager.handle_action(UIAction::Confirm));
    assert(!manager.handle_action(UIAction::Back));

    return 0;
}
