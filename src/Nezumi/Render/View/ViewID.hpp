// =============================================================================
// ViewID.hpp
//
// View identification. Index into the renderer's view array which selects which
// camera a draw goes through. See Renderer::set_view().
//
// Range: 0-15 (the sort key's 4 view bits).
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::Render {

using ViewID = std::uint8_t;

inline constexpr std::uint32_t VIEW_COUNT = 16;

struct View {
    static constexpr ViewID Player = 0; // world-space (default)
    static constexpr ViewID Screen = 1; // screen-space, i.e. UI
};

} // namespace Nezumi::Render
