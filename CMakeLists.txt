cmake_minimum_required(VERSION 3.20)

project(KiddOS
    VERSION 0.1.0
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
)

target_link_libraries(kiddos PRIVATE PkgConfig::SDL2)

target_compile_options(kiddos PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
