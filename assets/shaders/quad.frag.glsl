// =============================================================================
// quad.frag.glsl
//
// Instance draw Quad render objects (same as sprite.frag.glsl).
// =============================================================================

#version 330 core

// The material's texture. MaterialTable's bind group layout names it, and the
// GL backend assigns its texture unit from that (see GLLimits.hpp).
uniform sampler2D u_texture;

in vec2 v_tex;
in vec4 v_tint;

layout(location = 0) out vec4 o_color;

void main() {
    vec4 color = texture(u_texture, v_tex) * v_tint;
    if (color.a <= 0.0) discard;
    o_color = color;
}
