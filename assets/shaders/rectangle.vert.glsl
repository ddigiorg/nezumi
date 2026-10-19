// =============================================================================
// rectangle.vert.glsl
//
// Instance draw rectangle render objects.
// =============================================================================

#version 330 core

layout(std140, binding = 0) uniform FrameUniforms {
    mat4 u_view_proj;
};

layout(location = 0) in vec2 v_pos;     // vertex (unit quad)
layout(location = 1) in vec4 i_pos_box; // instance { min_x, min_y, max_x, max_y }
layout(location = 2) in vec4 i_color;   // instance {     r,     g,     b,     a }

out vec4 v_color;

void main() {
    vec2 world_pos = i_pos_box.xy + v_pos * (i_pos_box.zw - i_pos_box.xy);
    gl_Position    = u_view_proj * vec4(world_pos, 0.0, 1.0);
    v_color        = i_color;
}
