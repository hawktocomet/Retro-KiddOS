#include "kiddos/ui/image.hpp"

#include <cassert>

int main() {
    using namespace kiddos::ui;

    const Rect bounds{0, 0, 200, 100};

    const auto contain = calculate_image_geometry(
        100, 100, bounds, ImageFit::contain
    );
    assert(contain.valid);
    assert(contain.destination.x == 50);
    assert(contain.destination.y == 0);
    assert(contain.destination.w == 100);
    assert(contain.destination.h == 100);

    const auto cover = calculate_image_geometry(
        100, 100, bounds, ImageFit::cover
    );
    assert(cover.valid);
    assert(cover.source.x == 0);
    assert(cover.source.y == 25);
    assert(cover.source.w == 100);
    assert(cover.source.h == 50);
    assert(cover.destination.x == 0);
    assert(cover.destination.y == 0);
    assert(cover.destination.w == 200);
    assert(cover.destination.h == 100);

    const auto stretch = calculate_image_geometry(
        100, 100, bounds, ImageFit::stretch
    );
    assert(stretch.valid);
    assert(stretch.source.w == 100);
    assert(stretch.source.h == 100);
    assert(stretch.destination.w == 200);
    assert(stretch.destination.h == 100);

    assert(!calculate_image_geometry(0, 100, bounds, ImageFit::contain).valid);
    assert(!calculate_image_geometry(100, 100, {}, ImageFit::cover).valid);

    Image image;
    assert(image.opacity() == 255);
    image.set_opacity(128);
    assert(image.opacity() == 128);
    image.set_opacity(0);
    assert(image.opacity() == 0);

    return 0;
}
