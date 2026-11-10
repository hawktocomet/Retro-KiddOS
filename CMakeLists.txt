cmake_minimum_required(VERSION 3.20)

project(KiddOS
    VERSION 0.2.0
    DESCRIPTION "A child-friendly gaming environment for the Retro Pixel Pocket"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_executable(kiddos
    src/main.cpp
    src/app/application.cpp
    src/platform/sdl_platform.cpp
)

target_include_directories(kiddos PRIVATE include)
target_link_libraries(kiddos PRIVATE PkgConfig::SDL2)

target_compile_options(kiddos PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
