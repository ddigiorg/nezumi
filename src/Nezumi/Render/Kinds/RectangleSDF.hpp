// =============================================================================
// RectangleSDF.hpp
//
// Per-instance data for a "fancy" rectangle: fill, border, rounded corners
// and antialiased edges, all from one signed distance in the fragment shader.
// Meant for UI panels, buttons, selection boxes. For thousands of plain debug
// boxes use Rectangle (32 bytes, no per-pixel math) instead.
//
//   RectangleSDF::filled (box, color)                 // solid
//   RectangleSDF::outline(box, color, width)          // border only
//   RectangleSDF{ .pos_box = box, .color = fill,      // both, rounded
//                 .border_color = edge, .corner_radius = 4, .border_width = 1 }
//
// Sizes are in the view's units (world units for View::World, pixels for a
// BottomLeft screen camera). The border and the antialiasing are both drawn
// inside pos_box, so an outline never spills outside the box you give it.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

struct RectangleSDF {

    // { min_x, min_y, max_x, max_y }
    glm::vec4 pos_box       = { 0.0f, 0.0f, 1.0f, 1.0f };

    // Fill RGBA (alpha 0 for an outline-only rectangle)
    glm::vec4 color         = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Border RGBA, used where border_width > 0
    glm::vec4 border_color  = { 0.0f, 0.0f, 0.0f, 0.0f };

    float     corner_radius = 0.0f; // clamped to half the shorter side
    float     border_width  = 0.0f; // inside pos_box; 0 = no border
    float     pad_[2]       = {};   // keeps the struct 64 bytes (room for 2 more params)
};

static_assert(sizeof(RectangleSDF) == 64, "RectangleSDF is 64 bytes");

} // namespace Nezumi::Render
