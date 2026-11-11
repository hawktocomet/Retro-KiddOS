#include "kiddos/app/home_carousel_screen.hpp"

#include "kiddos/ui/panel.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace kiddos::app {

namespace {

// Geometry and colors mirror SDLPlatform::draw_carousel exactly so the
// UI-library home screen preserves the established Kid Mode appearance.
constexpr ui::Rect side_left_card{25, 250, 190, 175};
constexpr ui::Rect side_right_card{505, 250, 190, 175};
constexpr ui::Rect center_card{145, 135, 430, 370};
constexpr ui::Rect center_glow_rect{
    center_card.x - 3, center_card.y - 3,
    center_card.w + 6, center_card.h + 6};

constexpr ui::Rect side_left_art{
    side_left_card.x + 55, side_left_card.y + 42, 80, 90};
constexpr ui::Rect side_right_art{
    side_right_card.x + 55, side_right_card.y + 42, 80, 90};
constexpr ui::Rect center_art_rect{
    center_card.x + 115, center_card.y + 70, 200, 205};

// The selected game's title sits directly below the center card and is
// centered on it. Images fit with `contain`, so each title keeps its
// natural proportions inside this centered area.
constexpr ui::Rect title_rect{145, 523, 430, 44};

// KiddOS wordmark at the top and physical control hints along the bottom.
constexpr ui::Rect wordmark_rect{235, 12, 250, 46};
constexpr ui::Rect hints_rect{0, 682, 720, 34};

constexpr ui::Color side_fill{16, 41, 75, 255};
constexpr ui::Color side_border{55, 105, 165, 190};
constexpr ui::Color center_fill{18, 63, 117, 255};
constexpr ui::Color center_border{36, 164, 255, 255};
constexpr ui::Color center_glow{36, 164, 255, 150};

// Artwork and titles are exported with a transparent background from the
// committed SVG sources, so no magenta keying artifacts remain on screen.
// The original magenta-keyed BMP files stay in use by the platform's
// manual rendering path.
constexpr std::array<const char*, HomeCarouselScreen::game_count>
    artwork_paths{
        "assets/artwork/generated/carousel-star.png",
        "assets/artwork/generated/carousel-mushroom.png",
        "assets/artwork/generated/carousel-car.png"
    };

// Per-game title assets, exported with a transparent background from the
// same SVG sources and typography as the original carousel title.
constexpr std::array<const char*, HomeCarouselScreen::game_count>
    title_paths{
        "assets/artwork/generated/carousel-title-star.png",
        "assets/artwork/generated/carousel-title.png",
        "assets/artwork/generated/carousel-title-car.png"
    };

constexpr const char* wordmark_path =
    "assets/artwork/generated/kiddos-wordmark.png";

constexpr const char* hints_path =
    "assets/artwork/generated/navigation-hints.png";

// Fills the speaker cone: a trapezoid with a vertical narrow edge flaring
// out to a vertical wide edge, drawn one scanline at a time so no seam
// appears between decomposition triangles.
void fill_speaker_cone(
    ui::Renderer& renderer,
    int narrow_x,
    int narrow_top,
    int narrow_bottom,
    int wide_x,
    int wide_top,
    int wide_bottom,
    ui::Color color
) {
    for (int y = wide_top; y <= wide_bottom; ++y) {
        int left = narrow_x;

        if (y < narrow_top) {
            left = narrow_x +
                (narrow_top - y) * (wide_x - narrow_x) /
                (narrow_top - wide_top);
        } else if (y > narrow_bottom) {
            left = narrow_x +
                (y - narrow_bottom) * (wide_x - narrow_x) /
                (wide_bottom - narrow_bottom);
        }

        renderer.draw_line({left, y}, {wide_x, y}, color);
    }
}

// Draws a circular arc as short connected line segments, used for the
// sound waves next to the speaker icon.
void draw_arc(
    ui::Renderer& renderer,
    ui::Point center,
    int radius,
    double start_degrees,
    double end_degrees,
    ui::Color color
) {
    constexpr double kPi = 3.14159265358979323846;

    const int steps = std::max(2, radius * 2);
    const double delta = (end_degrees - start_degrees) / steps;

    const auto point_at = [&](double degrees) {
        const double radians = degrees * kPi / 180.0;
        return ui::Point{
            center.x + static_cast<int>(std::lround(
                radius * std::cos(radians))),
            center.y + static_cast<int>(std::lround(
                radius * std::sin(radians)))
        };
    };

    ui::Point previous = point_at(start_degrees);
    for (int step = 1; step <= steps; ++step) {
        const ui::Point point = point_at(start_degrees + delta * step);
        renderer.draw_line(previous, point, color);
        previous = point;
    }
}

// Draws the sound and battery indicators in the top corners of the screen,
// matching SDLPlatform::draw_status.
class StatusIndicators final : public ui::Element {
protected:
    void draw(ui::Renderer& renderer) override {
        const ui::Color body{205, 222, 255, 220};
        const ui::Color waves{80, 157, 255, 220};
        const ui::Color charge{67, 157, 255, 240};

        // Sound status: speaker cabinet, flared cone, and two sound waves.
        renderer.fill_rect({25, 33, 8, 14}, body);
        fill_speaker_cone(renderer, 33, 33, 46, 44, 27, 52, body);

        draw_arc(renderer, {47, 39}, 6, -50.0, 50.0, waves);
        draw_arc(renderer, {47, 39}, 12, -50.0, 50.0, waves);

        // Battery status: outline, terminal, charge fill.
        renderer.outline_rect({657, 27, 34, 19}, body);
        renderer.fill_rect({691, 33, 4, 7}, body);
        renderer.fill_rect({661, 31, 22, 11}, charge);
    }
};

} // namespace

HomeCarouselScreen::HomeCarouselScreen(SDL_Renderer* renderer) {
    for (int index = 0; index < game_count; ++index) {
        artworks_[static_cast<std::size_t>(index)].load(
            renderer, artwork_paths[static_cast<std::size_t>(index)], false);
    }
    for (int index = 0; index < game_count; ++index) {
        title_textures_[static_cast<std::size_t>(index)].load(
            renderer, title_paths[static_cast<std::size_t>(index)], false);
    }
    wordmark_texture_.load(renderer, wordmark_path, false);
    hints_texture_.load(renderer, hints_path, false);

    // Smaller neighboring cards first, drawn behind the main card.
    auto& side_left = root().add<ui::Panel>(
        ui::PanelStyle{side_fill, side_border, 1, 24});
    side_left.set_bounds(side_left_card);

    side_left_art_ = &root().add<ui::Image>();
    side_left_art_->set_bounds(side_left_art);
    side_left_art_->set_fit(ui::ImageFit::stretch);
    side_left_art_->set_opacity(180);

    auto& side_right = root().add<ui::Panel>(
        ui::PanelStyle{side_fill, side_border, 1, 24});
    side_right.set_bounds(side_right_card);

    side_right_art_ = &root().add<ui::Image>();
    side_right_art_->set_bounds(side_right_art);
    side_right_art_->set_fit(ui::ImageFit::stretch);
    side_right_art_->set_opacity(180);

    // Main card is drawn after the neighbors so it overlaps both of them.
    auto& center = root().add<ui::Panel>(
        ui::PanelStyle{center_fill, center_border, 1, 30});
    center.set_bounds(center_card);

    // Outer glow outline, matching the platform's second border pass.
    auto& glow = root().add<ui::Panel>(
        ui::PanelStyle{ui::colors::transparent, center_glow, 1, 32});
    glow.set_bounds(center_glow_rect);

    center_art_ = &root().add<ui::Image>();
    center_art_->set_bounds(center_art_rect);
    center_art_->set_fit(ui::ImageFit::stretch);

    // The title always shows the currently selected game's name, centered
    // below the main card.
    title_ = &root().add<ui::Image>();
    title_->set_bounds(title_rect);
    title_->set_fit(ui::ImageFit::contain);

    // KiddOS wordmark centered at the top of the screen.
    wordmark_ = &root().add<ui::Image>(&wordmark_texture_);
    wordmark_->set_bounds(wordmark_rect);
    wordmark_->set_fit(ui::ImageFit::contain);

    // Physical control hints centered along the bottom of the screen.
    hints_ = &root().add<ui::Image>(&hints_texture_);
    hints_->set_bounds(hints_rect);
    hints_->set_fit(ui::ImageFit::contain);

    root().add<StatusIndicators>();

    set_selected_game(1);
}

HomeCarouselScreen::~HomeCarouselScreen() {
    // Detach the images from the screen-owned textures before those
    // textures are destroyed, so no element references a dead texture.
    if (side_left_art_) side_left_art_->set_texture(nullptr);
    if (side_right_art_) side_right_art_->set_texture(nullptr);
    if (center_art_) center_art_->set_texture(nullptr);
    if (title_) title_->set_texture(nullptr);
    if (wordmark_) wordmark_->set_texture(nullptr);
    if (hints_) hints_->set_texture(nullptr);
}

void HomeCarouselScreen::set_selected_game(int index) {
    if (index < 0 || index >= game_count) return;

    selected_game_ = index;

    // Neighbors use the same wrap-around mapping as draw_carousel.
    if (side_left_art_) {
        side_left_art_->set_texture(
            &artworks_[static_cast<std::size_t>((index + 2) % game_count)]);
    }
    if (side_right_art_) {
        side_right_art_->set_texture(
            &artworks_[static_cast<std::size_t>((index + 1) % game_count)]);
    }
    if (center_art_) {
        center_art_->set_texture(
            &artworks_[static_cast<std::size_t>(index)]);
    }

    // The title below the card follows the selected game.
    if (title_) {
        title_->set_texture(
            &title_textures_[static_cast<std::size_t>(index)]);
    }
}

} // namespace kiddos::app
