// =============================================================================
// Quad.hpp
//
// Per-instance textured quad with 4 free corners (64 bytes).
//
// A Sprite is an axis-aligned box; a Quad is any parallelogram (or any convex
// quad, sampled bilinearly). It draws a square texture as a diamond on an
// isometric ground, and rotated or sheared sprites in general:
//
//   const auto corners = proj.box_to_view(tile.bounds().to_vec4());
//   renderer.draw_quad(Quad{ .corners = corners, .tex_box = grass }, params);
//
// Drawn with Renderer2D::draw_quad(quad, { .material = atlas->material() }).
// =============================================================================

#pragma once
#include <array>       // array
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct Quad {
    // Corners (world)  bottom-left, bottom-right, top-right, top-left of the
    //                  texture: tex_box's (min_u, max_v) lands on corners[0]
    std::array<glm::vec2, 4> corners = {{
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
    }};

    // Tex Box          { min_u, min_v, max_u, max_v }
    glm::vec4 tex_box = {  0.0f,  0.0f,  1.0f,  1.0f };
    // Note: v = 0 at the image's top row, as for Sprite.

    // Tint             {     r,     g,     b,     a }
    glm::vec4 tint    = {  1.0f,  1.0f,  1.0f,  1.0f };
};

static_assert(sizeof(Quad) == 64, "Quad is 64 bytes");

} // namespace Nezumi::Render
