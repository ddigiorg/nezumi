// =============================================================================
// FrameRecipe.hpp
//
// The passes of a frame, in order. RenderCore runs it in end_frame(). The
// recipe is the game's choice; the Engine sets up a default one:
//
//   main: clear the backbuffer, then Renderer2D draws every view
//
// A game that adds a 3D renderer could instead use:
//
//   recipe.passes = {
//       { .name = "scene", .features = { { &renderer3d } } },
//       { .name = "ui",    .clear = false,
//         .features = { { &renderer2d, view_bit(View::Screen) } } },
//   };
//
// This is a plain ordered list, not a render graph: passes don't declare what
// they read and write, and nothing is reordered or allocated for them.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/PassConfig.hpp"
#include <vector> // vector

namespace Nezumi::Render {

struct FrameRecipe {
    std::vector<PassConfig> passes;
};

} // namespace Nezumi::Render
