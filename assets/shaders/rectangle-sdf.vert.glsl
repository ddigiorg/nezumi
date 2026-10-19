// =============================================================================
// rectangle-sdf.vert.glsl
//
// Instance draw RectangleSDF render objects.
// =============================================================================

#version 330 core

layout(std140, binding = 0) uniform FrameUniforms {
    mat4 u_view_proj;
};

layout(location = 0) in vec2 v_pos;          // vertex (unit quad)
layout(location = 1) in vec4 i_pos_box;      // instance { min_x, min_y, max_x, max_y }
layout(location = 2) in vec4 i_color;        // instance fill
layout(location = 3) in vec4 i_border_color; // instance border
layout(location = 4) in vec2 i_params;       // instance { corner_radius, border_width }

out vec2 v_local;  // position relative to box center
out vec2 v_half;   // half size
out vec4 v_color;
out vec4 v_border_color;
out vec2 v_params; // { radius, border }, clamped

void main() {
    vec2 size      = i_pos_box.zw - i_pos_box.xy;
    vec2 world_pos = i_pos_box.xy + v_pos * size;
    vec2 half_size = 0.5 * abs(size);
    float max_r    = min(half_size.x, half_size.y);

    v_local        = (v_pos - 0.5) * size;
    v_half         = half_size;
    v_color        = i_color;
    v_border_color = i_border_color;
    // Clamp once per vertex instead of once per pixel.
    v_params       = vec2(clamp(i_params.x, 0.0, max_r),
                          clamp(i_params.y, 0.0, max_r));

    gl_Position = u_view_proj * vec4(world_pos, 0.0, 1.0);
}
