// =============================================================================
// sprite.frag.glsl
//
// Instance draw Sprite render objects.
// =============================================================================

#version 460 core

// Group 1 is a Texture (binding 0) + Sampler (binding 1). The RHI GL backend
// maps (group, binding) to GL binding point group * 16 + binding, and applies
// the group's sampler to that texture unit, so the atlas is on texture unit
// 1 * 16 + 0 = 16.
layout(binding = 16) uniform sampler2D u_atlas;

layout(location = 0) in vec2 v_tex;
layout(location = 1) in vec4 v_tint;

layout(location = 0) out vec4 o_color;

void main() {
    vec4 color = texture(u_atlas, v_tex) * v_tint;
    if (color.a <= 0.0) discard;
    o_color = color;
}
