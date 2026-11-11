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
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)

add_library(kiddos_ui STATIC
    src/ui/renderer.cpp
    src/ui/layout.cpp
    src/ui/focus.cpp
    src/ui/button.cpp
    src/ui/controller.cpp
    src/ui/font.cpp
    src/ui/text.cpp
    src/ui/texture.cpp
    src/ui/image.cpp
    src/ui/label.cpp
    src/ui/panel.cpp
    src/ui/progress_bar.cpp
    src/ui/toggle.cpp
    src/ui/slider.cpp
    src/ui/theme.cpp
    src/ui/list_view.cpp
)

target_include_directories(kiddos_ui PUBLIC include)
target_link_libraries(kiddos_ui PUBLIC PkgConfig::SDL2 PkgConfig::SDL2_TTF PkgConfig::SDL2_IMAGE)

target_compile_options(kiddos_ui PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(kiddos
    src/main.cpp
    src/app/application.cpp
    src/platform/sdl_platform.cpp
)

target_include_directories(kiddos PRIVATE include)
target_link_libraries(kiddos PRIVATE kiddos_ui)

target_compile_options(kiddos PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
    add_executable(kiddos_ui_geometry_test
        tests/ui_geometry_test.cpp
    )

    target_include_directories(kiddos_ui_geometry_test PRIVATE include)

    add_executable(kiddos_ui_layout_test
        tests/ui_layout_test.cpp
        src/ui/layout.cpp
    )

    target_include_directories(kiddos_ui_layout_test PRIVATE include)
    target_link_libraries(kiddos_ui_layout_test PRIVATE kiddos_ui)

    add_test(
        NAME kiddos_ui_layout
        COMMAND kiddos_ui_layout_test
    )

    add_test(
        NAME kiddos_ui_geometry
        COMMAND kiddos_ui_geometry_test
    )

    add_executable(kiddos_ui_focus_test
        tests/ui_focus_test.cpp
    )

    target_include_directories(kiddos_ui_focus_test PRIVATE include)
    target_link_libraries(kiddos_ui_focus_test PRIVATE kiddos_ui)

    add_test(
        NAME kiddos_ui_focus
        COMMAND kiddos_ui_focus_test
    )

    add_executable(kiddos_ui_button_test
        tests/ui_button_test.cpp
    )

    target_include_directories(kiddos_ui_button_test PRIVATE include)
    target_link_libraries(kiddos_ui_button_test PRIVATE kiddos_ui)

    add_test(
        NAME kiddos_ui_button
        COMMAND kiddos_ui_button_test
    )

    add_executable(kiddos_ui_controller_test
        tests/ui_controller_test.cpp
    )

    target_include_directories(kiddos_ui_controller_test PRIVATE include)
    target_link_libraries(kiddos_ui_controller_test PRIVATE kiddos_ui)

    add_test(
        NAME kiddos_ui_controller
        COMMAND kiddos_ui_controller_test
    )

    add_executable(kiddos_ui_image_test tests/ui_image_test.cpp)
    target_include_directories(kiddos_ui_image_test PRIVATE include)
    target_link_libraries(kiddos_ui_image_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_image COMMAND kiddos_ui_image_test)

    add_executable(kiddos_ui_label_test tests/ui_label_test.cpp)
    target_include_directories(kiddos_ui_label_test PRIVATE include)
    target_link_libraries(kiddos_ui_label_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_label COMMAND kiddos_ui_label_test)

    add_executable(kiddos_ui_panel_test tests/ui_panel_test.cpp)
    target_include_directories(kiddos_ui_panel_test PRIVATE include)
    target_link_libraries(kiddos_ui_panel_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_panel COMMAND kiddos_ui_panel_test)

    add_executable(kiddos_ui_progress_bar_test tests/ui_progress_bar_test.cpp)
    target_include_directories(kiddos_ui_progress_bar_test PRIVATE include)
    target_link_libraries(kiddos_ui_progress_bar_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_progress_bar COMMAND kiddos_ui_progress_bar_test)

    add_executable(kiddos_ui_toggle_test tests/ui_toggle_test.cpp)
    target_include_directories(kiddos_ui_toggle_test PRIVATE include)
    target_link_libraries(kiddos_ui_toggle_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_toggle COMMAND kiddos_ui_toggle_test)

    add_executable(kiddos_ui_slider_test tests/ui_slider_test.cpp)
    target_include_directories(kiddos_ui_slider_test PRIVATE include)
    target_link_libraries(kiddos_ui_slider_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_slider COMMAND kiddos_ui_slider_test)

    add_executable(kiddos_ui_theme_test tests/ui_theme_test.cpp)
    target_include_directories(kiddos_ui_theme_test PRIVATE include)
    target_link_libraries(kiddos_ui_theme_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_theme COMMAND kiddos_ui_theme_test)
    add_executable(kiddos_ui_list_view_test tests/ui_list_view_test.cpp)
    target_include_directories(kiddos_ui_list_view_test PRIVATE include)
    target_link_libraries(kiddos_ui_list_view_test PRIVATE kiddos_ui)
    add_test(NAME kiddos_ui_list_view COMMAND kiddos_ui_list_view_test)
endif()



