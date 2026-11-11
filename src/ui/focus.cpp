#include "kiddos/ui/focus.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <vector>

namespace kiddos::ui {

namespace {

void collect_focusable(Container& container,
                       std::vector<Element*>& result) {
    for (std::size_t i = 0; i < container.child_count(); ++i) {
        Element* element = container.child(i);
        if (!element || !element->visible()) continue;

        if (element->focusable() && element->enabled()) {
            result.push_back(element);
        }

        if (auto* nested = dynamic_cast<Container*>(element)) {
            collect_focusable(*nested, result);
        }
    }
}

int center_x(const Rect& rect) {
    return rect.x + rect.w / 2;
}

int center_y(const Rect& rect) {
    return rect.y + rect.h / 2;
}

} // namespace

FocusManager::FocusManager(Container& root) noexcept
    : root_(root) {
}

Element* FocusManager::focused() const noexcept {
    return focused_;
}

bool FocusManager::eligible(const Element* element) const noexcept {
    return element && element->visible() && element->enabled()
        && element->focusable();
}

void FocusManager::clear() noexcept {
    if (focused_) focused_->set_focused(false);
    focused_ = nullptr;
}

bool FocusManager::focus_first() noexcept {
    std::vector<Element*> candidates;
    collect_focusable(root_, candidates);

    if (candidates.empty()) {
        clear();
        return false;
    }

    if (eligible(focused_)) return true;

    clear();
    focused_ = candidates.front();
    focused_->set_focused(true);
    return true;
}

bool FocusManager::move(Direction direction) noexcept {
    std::vector<Element*> candidates;
    collect_focusable(root_, candidates);

    if (candidates.empty()) {
        clear();
        return false;
    }

    if (!eligible(focused_)) {
        clear();
        focused_ = candidates.front();
        focused_->set_focused(true);
        return true;
    }

    const Rect current = focused_->bounds();
    const int cx = center_x(current);
    const int cy = center_y(current);

    Element* best = nullptr;
    long long best_score = std::numeric_limits<long long>::max();

    for (Element* candidate : candidates) {
        if (candidate == focused_) continue;

        const Rect target = candidate->bounds();
        const int dx = center_x(target) - cx;
        const int dy = center_y(target) - cy;

        int primary = 0;
        int secondary = 0;

        switch (direction) {
            case Direction::Left:
                primary = -dx;
                secondary = std::abs(dy);
                break;
            case Direction::Right:
                primary = dx;
                secondary = std::abs(dy);
                break;
            case Direction::Up:
                primary = -dy;
                secondary = std::abs(dx);
                break;
            case Direction::Down:
                primary = dy;
                secondary = std::abs(dx);
                break;
        }

        if (primary <= 0) continue;

        const long long score =
            static_cast<long long>(primary) * 1000LL + secondary;

        if (score < best_score) {
            best_score = score;
            best = candidate;
        }
    }

    // No candidate in that direction: leave focus unchanged.
    if (!best) return false;

    focused_->set_focused(false);
    focused_ = best;
    focused_->set_focused(true);
    return true;
}

} // namespace kiddos::ui
