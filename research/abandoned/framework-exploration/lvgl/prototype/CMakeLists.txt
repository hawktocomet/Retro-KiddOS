cmake_minimum_required(VERSION 3.16)
project(kiddos_lvgl_prototype C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)

if(NOT DEFINED LVGL_SOURCE_DIR)
    message(FATAL_ERROR "Pass -DLVGL_SOURCE_DIR=/path/to/pinned/lvgl")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_INCLUDE_SIMPLE ON CACHE BOOL "" FORCE)

add_subdirectory("${LVGL_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/lvgl")

add_executable(kiddos_lvgl_prototype main.c)
target_include_directories(kiddos_lvgl_prototype PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kiddos_lvgl_prototype PRIVATE lvgl PkgConfig::SDL2)
