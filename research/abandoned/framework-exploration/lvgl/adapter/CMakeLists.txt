cmake_minimum_required(VERSION 3.16)
project(kiddos_lvgl_sdl_adapter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(LV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LVGL_DIR "/tmp/kiddos-lvgl-cff1c632206a267534102ee1065f51ac87be4099" CACHE PATH "" FORCE)

find_package(SDL2 REQUIRED)
add_subdirectory("${LVGL_DIR}" "${CMAKE_BINARY_DIR}/lvgl")

add_executable(kiddos_lvgl_sdl_adapter main.c)
target_include_directories(kiddos_lvgl_sdl_adapter PRIVATE "${LVGL_DIR}")
target_link_libraries(kiddos_lvgl_sdl_adapter PRIVATE lvgl SDL2::SDL2)
