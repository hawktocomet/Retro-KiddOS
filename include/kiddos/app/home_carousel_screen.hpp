#pragma once

#include "kiddos/app/platform.hpp"
#include "kiddos/ui/image.hpp"
#include "kiddos/ui/screen.hpp"
#include "kiddos/ui/texture.hpp"

#include <SDL.h>

#include <array>

namespace kiddos::app {

class HomeCarouselScreen final : public ui::Screen {
public:
    static constexpr int game_count = static_cast<int>(platform_count);

    explicit HomeCarouselScreen(SDL_Renderer* renderer);
    ~HomeCarouselScreen() override;

    void set_selected_game(int index);
    int selected_game() const noexcept { return selected_game_; }
    void update(float delta_seconds) override;

private:
    std::array<ui::Texture, game_count> game_textures_{};

    ui::Texture wordmark_texture_{};
    ui::Texture hints_texture_{};

    static constexpr std::size_t visible_slot_count_ = 3;
    std::array<ui::Image*, visible_slot_count_> game_images_{};
    std::array<ui::Rect, visible_slot_count_> target_bounds_{};
    std::array<int, visible_slot_count_> target_opacity_{};

    ui::Image* wordmark_ = nullptr;
    ui::Image* hints_ = nullptr;

    int selected_game_ = -1;
};

} // namespace kiddos::app
