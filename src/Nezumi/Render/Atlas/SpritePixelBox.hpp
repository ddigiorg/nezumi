// =============================================================================
// SpritePixelBox.hpp
//
// A sprite's box in the image, in pixels, as two corners { x0, y0, x1, y1 }.
// Corners given right-to-left (x0 > x1) mirror the sprite horizontally, and
// likewise for y; no extra pixels are needed for a mirrored sprite.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2
#include <cstdint>     // int32_t

namespace Nezumi::Render {

// TODO

// struct SpritePixelBox {
//     std::int32_t min_x = 0;
//     std::int32_t min_y = 0;
//     std::int32_t max_x = 0;
//     std::int32_t max_y = 0;

//     glm::vec2 pivot = { 0.5f, 0.5f }; // 0..1, (0,0) = bottom-left; see SpriteRegion::box_at
// };

} // namespace Nezumi::Render
