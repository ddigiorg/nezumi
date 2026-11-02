// =============================================================================
// Sprite.hpp
//
// Per-instance rectangle data (40 bytes).
//
// Drawn with Renderer2D::draw_sprite(sprite, { .material = sheet->material() }).
// SpriteSheet::sprite_at() fills tex_box and layer from a sprite's region.
//
// For depth sorting put the sort value in DrawParams::order, e.g.
//
//   .order = -int(pos.y * 16)
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Color/Rgba8.hpp"
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

struct Sprite {
    // Pos Box (world)  { min_x, min_y, max_x, max_y }
    glm::vec4 pos_box = {  0.0f,  0.0f,  1.0f,  1.0f };

    // Tex Box          { min_u, min_v, max_u, max_v }
    glm::vec4 tex_box = {  0.0f,  0.0f,  1.0f,  1.0f };
    // Note: v = 0 at the image's top row.
    // Note: Swap min_u/max_u to mirror horizontally.

    // Tint, packed RGBA8 (see rgba8()). Multiplied with the texture: white =
    // unchanged, alpha fades.
    std::uint32_t tint  = RGBA8_WHITE;

    // Layer (page) of the material's array texture that tex_box is on
    std::uint32_t layer = 0;
};

static_assert(sizeof(Sprite) == 40, "Sprite is 40 bytes");

} // namespace Nezumi::Render
