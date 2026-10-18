// =============================================================================
// circle.vert.glsl
//
// Instance draw Circle render objects.
// =============================================================================

#version 460 core

layout(std140, binding = 0) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2 v_pos;    // vertex (unit quad)
layout(location = 1) in vec2 i_pos;    // instance { x, y }
layout(location = 2) in vec4 i_color;  // instance { r, g, b, a }
layout(location = 3) in vec2 i_params; // instance { radius, width }

layout(location = 0) out vec2 v_local;  // position relative to the center (world units)
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec2 v_params;

void main() {
    float radius = max(i_params.x, 0.0);
    vec2  local  = (v_pos * 2.0 - 1.0) * radius;

    v_local  = local;
    v_color  = i_color;
    v_params = vec2(radius, i_params.y);

    gl_Position = view_proj * vec4(i_pos + local, 0.0, 1.0);
}
