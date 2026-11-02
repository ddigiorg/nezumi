// =============================================================================
// sprite.frag.glsl
//
// Instance draw Sprite render objects.
// =============================================================================

#version 330 core

// The material's texture. MaterialTable's bind group layout names it, and the
// GL backend assigns its texture unit from that (see GLLimits.hpp).
uniform sampler2DArray u_texture; // every material texture is an array (see MaterialTable.hpp)

in vec2 v_tex;
in vec4 v_tint;
flat in float v_layer;

layout(location = 0) out vec4 o_color;

void main() {
    // Texels are premultiplied (see AtlasConfig.hpp); premultiply the
    // straight-alpha tint to match. Output is premultiplied.
    vec4 color = texture(u_texture, vec3(v_tex, v_layer)) * vec4(v_tint.rgb * v_tint.a, v_tint.a);
    if (color.a <= 0.0) discard;
    o_color = color;
}
