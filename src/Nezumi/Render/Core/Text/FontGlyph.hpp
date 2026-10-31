// =============================================================================
// FontGlyph.hpp
//
// Visual representation of an ASCII letter, number, symbol, etc.
// =============================================================================

#pragma once

#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct FontGlyph {
    glm::vec4 tex_box = { 0.0f, 0.0f, 0.0f, 0.0f }; // min_u, min_v, max_u, max_v (v down)
    glm::vec2 size    = { 0.0f, 0.0f };             // bitmap size, px (includes the SDF spread)
    glm::vec2 bearing = { 0.0f, 0.0f };             // pen -> bitmap's top-left, px (y up)
    float     advance =   0.0f;                     // pen movement, px

    bool visible() const { return size.x > 0.0f && size.y > 0.0f; } // false for ' '
};

} // namespace Nezumi::Render
