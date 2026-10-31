// =============================================================================
// Arrow.hpp
//
// Per-instance arrow data.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct Arrow {
    // Position (world)  {     x,     y }
    glm::vec2 position = {  0.0f,  0.0f };

    // Size (world space relative to unit arrow)
    float size = 1.0f;

    // Angle (radians)
    float angle = 0.0f;

    // Color          {    r,    g,    b,    a }
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

static_assert(sizeof(Arrow) == 32, "Arrow is 32 bytes");

} // namespace Nezumi::Render
