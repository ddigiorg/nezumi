// =============================================================================
// quad.vert.glsl
//
// Instance draw Quad render objects: a textured quad with 4 free corners.
// =============================================================================

#version 330 core

layout(std140) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2 v_pos;       // vertex   {x, y} and {u, v}
layout(location = 1) in vec4 i_corners01; // instance {x0, y0, x1, y1}: bottom-left, bottom-right (world, y up)
layout(location = 2) in vec4 i_corners23; // instance {x2, y2, x3, y3}: top-right, top-left
layout(location = 3) in vec4 i_tex_box;   // instance {min_u, min_v, max_u, max_v} (v = 0 at image top)
layout(location = 4) in vec4 i_tint;      // instance {r, g, b, a}

out vec2 v_tex;
out vec4 v_tint;

void main() {
    // Bilinear blend of the corners: exact for parallelograms (projected
    // tiles), and still a filled quad for any other convex shape
    vec2 bottom = mix(i_corners01.xy, i_corners01.zw, v_pos.x);
    vec2 top    = mix(i_corners23.zw, i_corners23.xy, v_pos.x);
    vec2 world  = mix(bottom, top, v_pos.y);

    // Same UV convention as sprite.vert.glsl: the bottom edge samples the
    // texture's bottom row (max_v)
    v_tex = vec2(
        mix(i_tex_box.x, i_tex_box.z, v_pos.x),
        mix(i_tex_box.w, i_tex_box.y, v_pos.y));

    v_tint = i_tint;

    gl_Position = view_proj * vec4(world, 0.0, 1.0);
}
