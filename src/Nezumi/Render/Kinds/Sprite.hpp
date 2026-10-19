// =============================================================================
// Sprite.hpp
//
// Per-instance rectangle data (48 bytes).
//
// Drawn with Renderer::draw_sprite(sprite, { .material = atlas->material() }).
//
// For depth sorting put the sort value in DrawParams::order, e.g.
//
//   .order = -int(pos.y * 16)
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

struct Sprite {
    // Pos Box (world)  { min_x, min_y, max_x, max_y }
    glm::vec4 pos_box = {  0.0f,  0.0f,  1.0f,  1.0f };

    // Tex Box          { min_u, min_v, max_u, max_v }
    glm::vec4 tex_box = {  0.0f,  0.0f,  1.0f,  1.0f };
    // Note: v = 0 at the image's top row.
    // Note: Swap min_u/max_u to mirror horizontally.

    // Tint             {     r,     g,     b,     a }
    glm::vec4 tint    = {  1.0f,  1.0f,  1.0f,  1.0f };
    // Note: Multiplied with the texture: white = unchanged, alpha fades.
};

static_assert(sizeof(Sprite) == 48, "Sprite is 48 bytes");

} // namespace Nezumi::Render
