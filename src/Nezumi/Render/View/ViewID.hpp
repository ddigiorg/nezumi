// =============================================================================
// ViewID.hpp
//
// View identification. Index into the renderer's view array which selects which
// camera a draw goes through. See Renderer::set_view().
//
// Range: 0-15 (the sort key's 4 view bits). Lower views draw first, so
// View::Screen (UI) lands on top of View::World.
//
// Games can name their own views next to the defaults:
//
//   namespace GameView {
//       inline constexpr Render::ViewID Minimap = 2;
//   }
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::Render {

using ViewID = std::uint8_t;

inline constexpr std::uint32_t VIEW_CAPACITY = 16;

// Views the renderer creates a camera for at construction.
struct View {
    static constexpr ViewID World  = 0; // CameraOrigin::Centered, world units
    static constexpr ViewID Screen = 1; // CameraOrigin::BottomLeft, 1 unit = 1 point (display_scale pixels)
};

} // namespace Nezumi::Render
