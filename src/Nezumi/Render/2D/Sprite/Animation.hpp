// =============================================================================
// Animation.hpp
//
// A run of sprites in a SpriteSheet: `frames` per direction, for each stored
// direction, direction-major:
//
//   sprite = first + direction * frames + frame
//
// Headings count clockwise-or-not however the game likes; the engine only
// assumes heading 0 and heading headings/2 face opposite ways along the
// mirror axis. When fewer directions are stored than there are headings,
// the stored ones are headings 0 .. headings/2 and the rest are drawn
// mirrored: heading h > headings/2 uses stored direction headings - h.
// So 8 headings store 5 directions, 16 store 9.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Sprite/SpriteID.hpp"
#include <cstdint> // uint16_t, uint32_t

namespace Nezumi::Render {

struct Animation {
    SpriteID      first      = 0;
    std::uint32_t frames     = 1; // per direction
    std::uint32_t directions = 1; // stored
    std::uint32_t headings   = 1; // drawn (directions, or 2 * (directions - 1) when mirrored)
    float         fps        = 0.0f;
    bool          loop       = true;

    bool mirrored() const { return headings != directions; }
};

} // namespace Nezumi::Render
