// =============================================================================
// line.vert.glsl
//
// Instance draw Line render objects.
// =============================================================================

#version 330 core

layout(std140, binding = 0) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2  v_pos;   // vertex (unit quad)
layout(location = 1) in vec2  i_pos_a; // instance { x, y }
layout(location = 2) in vec2  i_pos_b; // instance { x, y }
layout(location = 3) in vec4  i_color; // instance { r, g, b, a }
layout(location = 4) in float i_width; // instance width

out vec4 v_color;

void main() {
    vec2 dir    = i_pos_b - i_pos_a;
    vec2 across = vec2(-dir.y, dir.x); // perpendicular, length |dir|
    float len   = length(dir);
    across      = len > 0.0 ? across / len : vec2(0.0); // a == b: zero area, nothing drawn

    vec2 world = i_pos_a + dir * v_pos.x
               + across * (v_pos.y - 0.5) * i_width;

    v_color     = i_color;
    gl_Position = view_proj * vec4(world, 0.0, 1.0);
}
