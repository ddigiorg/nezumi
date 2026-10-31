// =============================================================================
// Circle.hpp
//
// Per-instance circle data.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct Circle {
    // Position (world)  {     x,     y }
    glm::vec2 position = {  0.0f,  0.0f };

    // Color          {    r,    g,    b,    a }
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Radius (world): distance from the center to the outer edge
    float radius = 1.0f;

    // Width (world pixels): ring thickness, measured inward
    // if width >= radius then filled circle, otherwise unfilled
    float width  = 1.0f;
};

static_assert(sizeof(Circle) == 32, "Circle is 32 bytes");

} // namespace Nezumi::Render
