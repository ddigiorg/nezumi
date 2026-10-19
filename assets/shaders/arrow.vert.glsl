// =============================================================================
// arrow.vert.glsl
//
// Instance draw Arrow render objects.
// =============================================================================

#version 330 core

layout(std140, binding = 0) uniform FrameUniforms {
    mat4 u_view_proj;
};

layout(location = 0) in vec2  v_pos;   // vertex (unit arrow)
layout(location = 1) in vec2  i_pos;   // instance { x, y }
layout(location = 2) in float i_size;  // instance
layout(location = 3) in float i_angle; // instance (radians)
layout(location = 4) in vec4  i_color; // instance { r, g, b, a }

out vec4 v_color;

void main() {
    float c = cos(i_angle);
    float s = sin(i_angle);
    mat2 rot = mat2(c,  s, -s,  c); // CCW rotationn
    vec2 scaled = v_pos * i_size;
    vec2 rotated = rot * scaled;
    vec2 position = rotated + i_pos;
    gl_Position = u_view_proj * vec4(position, 0.0, 1.0);
    v_color = i_color;
}
