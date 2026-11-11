#pragma once

#include "kiddos/ui/element.hpp"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace kiddos::ui {

class Container : public Element {
public:
    template<typename T, typename... Args>
    T& add(Args&&... args) {
        static_assert(std::is_base_of_v<Element, T>,
                      "Container children must derive from Element");

        auto child = std::make_unique<T>(
            std::forward<Args>(args)...
        );

        T& reference = *child;
        children_.push_back(std::move(child));
        return reference;
    }

    std::size_t child_count() const noexcept {
        return children_.size();
    }

    Element* child(std::size_t index) noexcept {
        if (index >= children_.size()) return nullptr;
        return children_[index].get();
    }

    const Element* child(std::size_t index) const noexcept {
        if (index >= children_.size()) return nullptr;
        return children_[index].get();
    }

protected:
    void draw(Renderer& renderer) override {
        for (auto& element : children_) {
            element->render(renderer);
        }
    }

private:
    std::vector<std::unique_ptr<Element>> children_;
};

} // namespace kiddos::ui
