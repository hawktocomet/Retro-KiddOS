#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace kiddos::app {

enum class PlatformId {
    GameBoy,
    GameBoyAdvance,
    SuperNintendo,
    Arcade
};

struct PlatformDefinition {
    PlatformId id;
    std::string_view short_name;
    std::string_view full_name;
};

inline constexpr std::array<PlatformDefinition, 4> initial_platforms{{
    {PlatformId::GameBoy, "GB/GBC", "Game Boy / Game Boy Color"},
    {PlatformId::GameBoyAdvance, "GBA", "Game Boy Advance"},
    {PlatformId::SuperNintendo, "SNES", "Super Nintendo"},
    {PlatformId::Arcade, "Arcade", "FinalBurn Neo"}
}};

inline constexpr std::size_t platform_count = initial_platforms.size();

} // namespace kiddos::app
