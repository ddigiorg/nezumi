// =============================================================================
// point-light.vert.glsl
//
// Instance draw PointLight render objects: a quad with 4 free corners (the
// light's square, projected), with a local position for the falloff.
// =============================================================================

#version 330 core

layout(std140) uniform Frame {
    mat4 view_proj;
};

layout(location = 0) in vec2 v_pos;       // vertex   {x, y} in 0..1
layout(location = 1) in vec4 i_corners01; // instance {x0, y0, x1, y1}: bottom-left, bottom-right
layout(location = 2) in vec4 i_corners23; // instance {x2, y2, x3, y3}: top-right, top-left
layout(location = 3) in vec4 i_color;     // instance {r, g, b, intensity} (linear)
layout(location = 4) in vec4 i_falloff;   // instance {core, softness, inner, unused}

out vec2 v_local;   // -1..1 across the light's square
out vec3 v_color;   // rgb * intensity
out vec3 v_falloff; // core, softness, inner

void main() {
    // Bilinear blend of the corners: exact for parallelograms (projected squares)
    vec2 bottom = mix(i_corners01.xy, i_corners01.zw, v_pos.x);
    vec2 top    = mix(i_corners23.zw, i_corners23.xy, v_pos.x);
    vec2 world  = mix(bottom, top, v_pos.y);

    v_local   = v_pos * 2.0 - 1.0;
    v_color   = i_color.rgb * i_color.a;
    v_falloff = i_falloff.xyz;

    gl_Position = view_proj * vec4(world, 0.0, 1.0);
}
