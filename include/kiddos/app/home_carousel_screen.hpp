#pragma once

#include "kiddos/ui/image.hpp"
#include "kiddos/ui/screen.hpp"
#include "kiddos/ui/texture.hpp"

#include <SDL.h>

#include <array>

namespace kiddos::app {

// Kid Mode home carousel built from KiddOS UI library elements.
// It mirrors the geometry and appearance of SDLPlatform::draw_carousel,
// which remains the visual reference for this screen.
class HomeCarouselScreen final : public ui::Screen {
public:
    static constexpr int game_count = 3;

    // The renderer must be the platform-owned SDL renderer; the screen
    // never creates or destroys a renderer. Textures are created from it
    // and remain owned by this screen for the lifetime of its elements.
    explicit HomeCarouselScreen(SDL_Renderer* renderer);
    ~HomeCarouselScreen() override;

    void set_selected_game(int index);
    int selected_game() const noexcept { return selected_game_; }

private:
    std::array<ui::Texture, game_count> artworks_{};
    ui::Texture title_texture_{};

    // Non-owning references into the root container, used to update the
    // carousel when the selection changes.
    ui::Image* side_left_art_ = nullptr;
    ui::Image* side_right_art_ = nullptr;
    ui::Image* center_art_ = nullptr;
    ui::Image* title_ = nullptr;

    int selected_game_ = 1;
};

} // namespace kiddos::app
