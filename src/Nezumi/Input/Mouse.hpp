// =============================================================================
// Mouse.hpp
//
// Mouse objects.
//
// SDL Reference: https://wiki.libsdl.org/SDL3/SDL_MouseButtonFlags
// =============================================================================

#pragma once
#include <cstdint>

namespace Nezumi {

enum class MouseButton : uint8_t {
    Unknown = 0,
    Left    = 1,
    Middle  = 2,
    Right   = 3,
    X1      = 4, // extra side button (back)
    X2      = 5, // extra side button (forward)
};

} // namespace Nezumi
