// =============================================================================
// DrawParams.hpp
//
// Per-draw view, ordering, and state.  Passed alongside the draw object itself.
//
//   renderer.draw(rect,   { .view = View::World .layer = 0 });
//   renderer.draw(text,   { .view = View::Screen           });
//
//   renderer.draw(sprite,
//     { .view = View::World .layer = 1, .order = int(pos.y) });
//
// Painter's order: lower layer first, then lower order first. Items with the
// same layer AND order are drawn in submission order, and are the only ones
// the renderer is free to group by state. So leave `order` at 0 for content
// whose relative order doesn't matter (tiles, decals, UI backgrounds): that's
// what lets it batch into a handful of draws.
// =============================================================================

#pragma once
#include "Nezumi/Render/Material/MaterialID.hpp"
#include "Nezumi/Render/View/ViewID.hpp"
#include <cstdint> // uint8_t, int32_t

namespace Nezumi::Render {

struct DrawParams {
    ViewID       view     = View::Screen;
    std::uint8_t layer    = 0;
    std::int32_t order    = 0; // clamped to +/- 2^23 (see SortKey.hpp)
    MaterialID   material = Material::None;
};

} // namespace Nezumi::Render
