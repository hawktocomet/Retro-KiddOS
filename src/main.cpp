#include "kiddos/app/application.hpp"
#include "kiddos/platform/sdl_platform.hpp"

int main() {
    kiddos::SDLPlatform platform;
    kiddos::Application application(platform);
    return application.run();
}
