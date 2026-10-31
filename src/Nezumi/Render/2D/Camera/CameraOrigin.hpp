// =============================================================================
// CameraOrigin.hpp
//
// Controls where camera origin (0, 0) sits in the viewport.
//
// Centered and BottomLeft keep world +y pointing UP the screen, which is what
// the sprite shader, text layout and SDF shapes assume. TopLeft flips it (+y
// points down), so sprites and text drawn through a TopLeft camera come out
// upside down; use BottomLeft for screen-space UI instead (1 unit = 1 pixel,
// y up). Mouse input converts with Camera2D::screen_to_world either way.
// =============================================================================

#pragma once

namespace Nezumi::Render {

enum class CameraOrigin {
    Centered,   // origin at viewport center      (world-space camera)
    BottomLeft, // origin at viewport bottom-left (screen-space camera: 1 unit = 1 px, y up)
    TopLeft,    // origin at viewport top-left, +y DOWN (flips sprites and text; see above)
};

} // namespace Nezumi::Render
