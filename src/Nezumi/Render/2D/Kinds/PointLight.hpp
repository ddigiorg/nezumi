// =============================================================================
// PointLight.hpp
//
// Per-instance light (64 bytes): a smooth radial falloff over a quad, added
// into whatever it draws to. Drawn into View::Lights it lights the scene (see
// PostProcess); drawn into View::Emissive it is an unlit glow halo.
//
// The light fills the disc inscribed in its 4 corners, so pass the corners
// of the light's square in the WORLD, projected: in 2:1 isometric the pool of
// light becomes a 2:1 ellipse on the ground, top-down a circle.
//
//   const glm::vec4 box = { p.x - r, p.y - r, p.x + r, p.y + r };
//   renderer.draw_light(PointLight{ .corners = proj.box_to_view(box),
//                                   .color   = { 1.0f, 0.6f, 0.3f, 2.0f } });
//
//   renderer.draw_light(PointLight::circle(center, radius, color)); // no projection
//
// Falloff (d = distance from the center / radius, 0..1):
//
//   hot   = (1 - d^4)^2 / (1 + core * d^2)   windowed inverse square
//   soft  = (1 - d^2)^2                      broad, even aura
//   I(d)  = mix(hot, soft, softness)
//
// `inner` keeps full brightness out to that fraction of the radius first.
// Colors are LINEAR and may exceed 1: rgb * a is the light's peak, and
// anything over 1 in the lit image blooms.
// =============================================================================

#pragma once
#include <array>       // array
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

struct PointLight {
    // Corners (view)   bottom-left, bottom-right, top-right, top-left of the
    //                  light's square
    std::array<glm::vec2, 4> corners = {{
        { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f },
    }};

    // Color            {     r,     g,     b, intensity } (linear)
    glm::vec4 color   = {  1.0f,  1.0f,  1.0f,      1.0f };

    // Falloff          {  core, softness, inner, unused }
    glm::vec4 falloff = {  8.0f,     0.0f,  0.0f,   0.0f };

    // An axis-aligned light of `radius` around `center`, in view space
    static PointLight circle(
        glm::vec2 center,
        float     radius,
        glm::vec4 color,
        glm::vec4 falloff = { 8.0f, 0.0f, 0.0f, 0.0f })
    {
        return PointLight{
            .corners = {{
                center + glm::vec2(-radius, -radius),
                center + glm::vec2( radius, -radius),
                center + glm::vec2( radius,  radius),
                center + glm::vec2(-radius,  radius),
            }},
            .color   = color,
            .falloff = falloff,
        };
    }
};

static_assert(sizeof(PointLight) == 64, "PointLight is 64 bytes");

} // namespace Nezumi::Render
