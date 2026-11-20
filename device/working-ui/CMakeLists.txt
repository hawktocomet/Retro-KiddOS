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
    screens/SystemScreens.cpp
    screens/ParentScreen.cpp
    screens/WifiScreen.cpp
    services/GameCatalog.cpp
    services/GameLauncher.cpp
    services/BatteryService.cpp
    services/WifiService.cpp
    services/ParentManagementService.cpp
    services/PowerService.cpp
)

add_executable(kiddos-ui ${KIDDOS_SOURCES})

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
