// =============================================================================
// text-glyph-sdf.vert.glsl
//
// Instance draw TextGlyphSDF render objects.
// =============================================================================

#version 460 core

layout(std140, binding = 0) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2 v_pos;           // vertex (unit quad)
layout(location = 1) in vec4 i_pos_box;       // instance { min_x, min_y, max_x, max_y }
layout(location = 2) in vec4 i_tex_box;       // instance { min_u, min_v, max_u, max_v }
layout(location = 3) in vec4 i_color;         // instance {     r,     g,     b,     a }
layout(location = 4) in vec4 i_outline_color; // instance {     r,     g,     b,     a }
layout(location = 5) in vec4 i_params;        // instance { outline width, weight, outline offset (uv) }

layout(location = 0) out vec2 v_tex;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec4 v_outline_color;
layout(location = 3) out vec4 v_params;

void main() {
    vec2 world = mix(i_pos_box.xy, i_pos_box.zw, v_pos);

    // World y goes up, atlas v goes down: the quad's bottom samples max v.
    v_tex = vec2(mix(i_tex_box.x, i_tex_box.z, v_pos.x),
                 mix(i_tex_box.w, i_tex_box.y, v_pos.y));

    v_color         = i_color;
    v_outline_color = i_outline_color;
    v_params        = i_params;

    gl_Position = view_proj * vec4(world, 0.0, 1.0);
}
