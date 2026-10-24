// =============================================================================
// ui-quad.vert.glsl
//
// Instance draw UIQuad render objects (boxes, glyphs and icons for the UI).
//
// Clipping: each corner is clamped into the clip box, so the quad shrinks to
// its visible part (or to nothing). Local position and UVs come from the
// clamped corner, so whatever survives is shaded exactly as it would be
// unclipped.
// =============================================================================

#version 330 core

layout(std140) uniform Frame {
    mat4 u_view_proj;
};

layout(location = 0) in vec2 v_pos;          // vertex (unit quad)
layout(location = 1) in vec4 i_pos_box;      // instance { min_x, min_y, max_x, max_y }
layout(location = 2) in vec4 i_tex_box;      // instance { min_u, min_v, max_u, max_v }, v down
layout(location = 3) in vec4 i_color;        // instance fill
layout(location = 4) in vec4 i_border_color; // instance border
layout(location = 5) in vec4 i_clip_box;     // instance { min_x, min_y, max_x, max_y }
layout(location = 6) in vec4 i_params;       // instance { radius, border, shape, weight }

out vec2      v_local;  // position relative to box center
out vec2      v_half;   // half size
out vec2      v_tex;
out vec4      v_color;
out vec4      v_border_color;
flat out vec4 v_params; // { radius, border } clamped, shape, weight

void main() {
    vec2 size    = i_pos_box.zw - i_pos_box.xy;
    vec2 corner  = mix(i_pos_box.xy, i_pos_box.zw, v_pos);
    vec2 clipped = clamp(corner, i_clip_box.xy, i_clip_box.zw);
    vec2 t       = (clipped - i_pos_box.xy) / max(abs(size), vec2(1e-6));

    v_half  = 0.5 * abs(size);
    v_local = (t - 0.5) * size;

    // View y goes up, atlas v goes down: the quad's bottom samples max v.
    v_tex = vec2(mix(i_tex_box.x, i_tex_box.z, t.x),
                 mix(i_tex_box.w, i_tex_box.y, t.y));

    v_color        = i_color;
    v_border_color = i_border_color;

    // Clamp once per vertex instead of once per pixel.
    float max_r = min(v_half.x, v_half.y);
    v_params    = vec4(clamp(i_params.x, 0.0, max_r),
                       clamp(i_params.y, 0.0, max_r),
                       i_params.z,
                       i_params.w);

    gl_Position = u_view_proj * vec4(clipped, 0.0, 1.0);
}
