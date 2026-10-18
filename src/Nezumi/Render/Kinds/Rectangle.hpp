// =============================================================================
// Rectangle.hpp
//
// Per-instance rectangle data.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

struct Rectangle {
    // Pos Box (world)  { min_x, min_y, max_x, max_y }
    glm::vec4 pos_box = {  0.0f,  0.0f,  1.0f,  1.0f };

    // Color            {     r,     g,     b,     a }
    glm::vec4 color   = {  1.0f,  1.0f,  1.0f,  1.0f };

    // TODO: width
    // if width >1.0 need to make an unfilled kind with 4 quads
};

static_assert(sizeof(Rectangle) == 32, "Rectangle is 32 bytes");

} // namespace Nezumi::Render
