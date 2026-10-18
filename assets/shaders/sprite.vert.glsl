// =============================================================================
// sprite.vert.glsl
//
// Instance draw Sprite render objects.
// =============================================================================

#version 460 core

layout(std140, binding = 0) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2 v_pos;     // vertex   {x, y} and {u, v}
layout(location = 1) in vec4 i_pos_box; // instance {min_x, min_y, max_x, max_y} (world, y up)
layout(location = 2) in vec4 i_tex_box; // instance {min_u, min_v, max_u, max_v} (v = 0 at image top)
layout(location = 3) in vec4 i_tint;    // instance {r, g, b, a}

layout(location = 0) out vec2 v_tex;
layout(location = 1) out vec4 v_tint;

void main() {
    vec2 world = mix(i_pos_box.xy, i_pos_box.zw, v_pos);

    // World y goes up, image v goes down: the quad's bottom (corner.y = 0)
    // samples the sprite's bottom row (max_v).
    v_tex = vec2(
        mix(i_tex_box.x, i_tex_box.z, v_pos.x),
        mix(i_tex_box.w, i_tex_box.y, v_pos.y));

    v_tint = i_tint;

    gl_Position = view_proj * vec4(world, 0.0, 1.0);
}
