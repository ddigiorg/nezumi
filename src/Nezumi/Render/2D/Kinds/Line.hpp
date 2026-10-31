// =============================================================================
// Line.hpp
//
// Per-instance line data.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct Line {
    // Position (world)    {     x,     y }
    glm::vec2 position_a = {  0.0f,  0.0f };
    glm::vec2 position_b = {  0.0f,  0.0f };

    // Color          {    r,    g,    b,    a }
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Width (world pixels)
    float width = 1.0f;

    // TODO:
    // float gap_length:  Length of the gaps that this line has. Default is 0.
    // float dash_length: Length of the dashes that this line has. Used only if
    //                    gap_length > 0. Default is 0.
    // float dash_offset: Starting offset to apply to dashes. Cannot be greater
    //                    than dash_length + gap_length. Default is 0.
};

static_assert(sizeof(Line) == 36, "Line is 36 bytes");

} // namespace Nezumi::Render
