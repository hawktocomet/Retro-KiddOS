cmake_minimum_required(VERSION 3.16)
project(kiddos-ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

set(KIDDOS_SOURCES
    main.cpp
    Application.cpp
    UI.cpp
    screens/GameScreens.cpp
    screens/MovieScreens.cpp
    screens/SystemScreens.cpp
    screens/ParentScreen.cpp
    screens/WifiScreen.cpp
    services/GameCatalog.cpp
    services/MovieCatalog.cpp
    services/MovieLauncher.cpp
    services/GameLauncher.cpp
    services/BatteryService.cpp
    services/WifiService.cpp
    services/ParentManagementService.cpp
    services/PowerService.cpp
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(RSVG_CONVERT_EXECUTABLE rsvg-convert)
if(NOT RSVG_CONVERT_EXECUTABLE)
    message(FATAL_ERROR "rsvg-convert is required to embed Tabler icons")
endif()

set(TABLER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(TABLER_HEADER "${TABLER_GENERATED_DIR}/tabler_icons.hpp")
add_custom_command(
    OUTPUT "${TABLER_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TABLER_GENERATED_DIR}"
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_tabler_icons.py"
            "${TABLER_HEADER}"
    DEPENDS
        tools/embed_tabler_icons.py
        assets/tabler-filled/device-gamepad-2.svg
        assets/tabler-filled/file-music.svg
        assets/tabler-filled/device-tv-old.svg
        assets/tabler-filled/device-tv.svg
    VERBATIM
)
add_custom_target(tabler-icons DEPENDS "${TABLER_HEADER}")

add_executable(kiddos-ui ${KIDDOS_SOURCES})
add_dependencies(kiddos-ui tabler-icons)
target_include_directories(kiddos-ui PRIVATE "${TABLER_GENERATED_DIR}")

target_include_directories(kiddos-ui PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_link_directories(kiddos-ui PRIVATE
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_IMAGE_LIBRARY_DIRS}
    ${SDL2_TTF_LIBRARY_DIRS}
)
target_link_libraries(kiddos-ui PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)
target_compile_options(kiddos-ui PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_IMAGE_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
)

install(TARGETS kiddos-ui RUNTIME DESTINATION bin)
