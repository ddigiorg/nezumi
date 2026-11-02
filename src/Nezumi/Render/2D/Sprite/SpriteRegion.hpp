// =============================================================================
// SpriteRegion.hpp
//
// Where a sprite is in its SpriteSheet, and how it sits on its anchor.
//
// Sprites may be trimmed: the packer cuts away transparent borders, so the
// stored box (size) is smaller than the frame the artist drew. offset and
// pivot are both in that original, untrimmed frame, in pixels, y down:
//
//   +-- untrimmed frame ---------+
//   |   offset                   |
//   |     +-- stored box --+     |
//   |     |     size       |     |
//   |     |                |     |
//   |     +---- * ---------+     |
//   |         pivot              |
//   +----------------------------+
//
// The pivot is the point that lands on the anchor when drawn: the feet of a
// unit, the base of a tree trunk.
// =============================================================================

#pragma once
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct SpriteRegion {
    glm::vec4     tex_box = {}; // UVs on its layer { min_u, min_v, max_u, max_v }, v down
    glm::vec2     size    = {}; // stored (trimmed) size, pixels
    glm::vec2     offset  = {}; // stored box's top-left in the untrimmed frame, pixels
    glm::vec2     pivot   = {}; // anchor point in the untrimmed frame, pixels
    std::uint32_t layer   = 0;  // page
};

} // namespace Nezumi::Render
