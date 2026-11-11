#include "kiddos/app/home_carousel_screen.hpp"

#include "kiddos/ui/panel.hpp"

#include <algorithm>
#include <array>

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
constexpr ui::Rect title_rect{220, 525, 280, 42};

constexpr ui::Color side_fill{16, 41, 75, 255};
constexpr ui::Color side_border{55, 105, 165, 190};
constexpr ui::Color center_fill{18, 63, 117, 255};
constexpr ui::Color center_border{36, 164, 255, 255};
constexpr ui::Color center_glow{36, 164, 255, 150};

constexpr std::array<const char*, HomeCarouselScreen::game_count>
    artwork_paths{
        "assets/artwork/generated/carousel-star.bmp",
        "assets/artwork/generated/carousel-mushroom.bmp",
        "assets/artwork/generated/carousel-car.bmp"
    };

constexpr const char* title_path =
    "assets/artwork/generated/carousel-title.bmp";

void fill_triangle(
    ui::Renderer& renderer,
    ui::Point a,
    ui::Point b,
    ui::Point c,
    ui::Color color
) {
    const ui::Point points[3] = {a, b, c};

    const int min_y = std::min({a.y, b.y, c.y});
    const int max_y = std::max({a.y, b.y, c.y});

    for (int y = min_y; y <= max_y; ++y) {
        int intersections[3] = {0, 0, 0};
        int count = 0;

        for (int i = 0; i < 3; ++i) {
            const int j = (i + 1) % 3;

            if ((points[i].y <= y && points[j].y > y) ||
                (points[j].y <= y && points[i].y > y)) {
                intersections[count++] = points[i].x +
                    (y - points[i].y) * (points[j].x - points[i].x) /
                    (points[j].y - points[i].y);
            }
        }

        if (count == 2) {
            renderer.draw_line(
                {std::min(intersections[0], intersections[1]), y},
                {std::max(intersections[0], intersections[1]), y},
                color
            );
        }
    }
}

// Draws the sound and battery indicators in the top corners of the screen,
// matching SDLPlatform::draw_status.
class StatusIndicators final : public ui::Element {
protected:
    void draw(ui::Renderer& renderer) override {
        const ui::Color white{205, 222, 255, 220};
        const ui::Color waves{80, 157, 255, 220};
        const ui::Color charge{67, 157, 255, 240};

        // Sound status: speaker and sound waves.
        fill_triangle(renderer, {31, 34}, {43, 25}, {43, 51}, white);
        renderer.fill_rect({25, 33, 10, 12}, white);

        renderer.draw_line({49, 29}, {54, 34}, waves);
        renderer.draw_line({54, 34}, {54, 42}, waves);
        renderer.draw_line({54, 42}, {49, 47}, waves);

        // Battery status: outline, terminal, charge fill.
        renderer.outline_rect({657, 27, 34, 19}, white);
        renderer.fill_rect({691, 33, 4, 7}, white);
        renderer.fill_rect({661, 31, 22, 11}, charge);
    }
};

} // namespace

HomeCarouselScreen::HomeCarouselScreen(SDL_Renderer* renderer) {
    // Reuse the existing generated artwork; the magenta background is
    // treated as transparent, as in the platform renderer.
    for (int index = 0; index < game_count; ++index) {
        artworks_[static_cast<std::size_t>(index)].load(
            renderer, artwork_paths[static_cast<std::size_t>(index)], true);
    }
    title_texture_.load(renderer, title_path, true);

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

    title_ = &root().add<ui::Image>(&title_texture_);
    title_->set_bounds(title_rect);
    title_->set_fit(ui::ImageFit::stretch);

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

    // The generated title artwork belongs to the middle game only.
    if (title_) title_->set_visible(index == 1);
}

} // namespace kiddos::app
