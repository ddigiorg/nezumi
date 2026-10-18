// =============================================================================
// SpriteTexelBox.hpp
// =============================================================================

#pragma once

namespace Nezumi::Render {

// TODO

// struct SpriteTexelBox {
//     MaterialId material = NO_MATERIAL;                // the atlas's texture
//     glm::vec4  uv_box   = { 0.0f, 0.0f, 1.0f, 1.0f }; // min_u, min_v, max_u, max_v (v down); swapped = mirrored
//     glm::vec2  size     = { 0.0f, 0.0f };             // in pixels
//     glm::vec2  pivot    = { 0.5f, 0.5f };             // 0..1, (0,0) = bottom-left

//     // World box placing the pivot at `position`. units_per_pixel sets scale,
//     // e.g. 1.0f / 64 when 64 pixels make one world unit (one tile).
//     glm::vec4 box_at(glm::vec2 position, float units_per_pixel = 1.0f) const {
//         const glm::vec2 world_size = size * units_per_pixel;
//         const glm::vec2 min        = position - pivot * world_size;
//         return { min.x, min.y, min.x + world_size.x, min.y + world_size.y };
//     }
// };

} // namespace Nezumi::Render
