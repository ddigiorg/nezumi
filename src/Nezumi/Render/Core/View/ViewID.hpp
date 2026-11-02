// =============================================================================
// ViewID.hpp
//
// View identification. A view is one camera's matrices for one frame, in a
// fixed slot of the uniform buffer (ViewBindings). Feature renderers publish
// their cameras into slots with RenderCore::set_view(); Renderer2D maps each
// view to a Camera2D (see Renderer2D::set_view()).
//
// Range: 0-15 (the sort key's 4 view bits). Lower views draw first, so
// View::Screen (UI) lands on top of View::World.
//
// Games can name their own views next to the defaults:
//
//   namespace GameView {
//       inline constexpr Render::ViewID Minimap = 4;
//   }
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::Render {

using ViewID = std::uint8_t;

inline constexpr std::uint32_t VIEW_CAPACITY = 16;

// A set of views, one bit per ViewID. Passes use it to say which views a
// feature records (see FrameRecipe).
using ViewMask = std::uint16_t;

inline constexpr ViewMask ALL_VIEWS = 0xFFFF;

constexpr ViewMask view_bit(ViewID view) {
    return ViewMask(1u << view);
}

// Views Renderer2D sets up at construction. Lights and Emissive draw through
// the world camera, so they line up with View::World; a lit FrameRecipe
// records each into its own target (see PostProcess.hpp).
struct View {
    static constexpr ViewID World    = 0; // CameraOrigin::Centered, world units
    static constexpr ViewID Screen   = 1; // CameraOrigin::BottomLeft, 1 unit = 1 point (display_scale pixels)
    static constexpr ViewID Lights   = 2; // world camera: lights, into the light target
    static constexpr ViewID Emissive = 3; // world camera: unlit glow (flames, spell cores), after the composite
};

} // namespace Nezumi::Render
