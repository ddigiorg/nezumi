// =============================================================================
// rectangle-sdf.frag.glsl
//
// Instance draw RectangleSDF render objects.
// =============================================================================

#version 460 core

layout(location = 0) in vec2 v_local;
layout(location = 1) in vec2 v_half;
layout(location = 2) in vec4 v_color;
layout(location = 3) in vec4 v_border_color;
layout(location = 4) in vec2 v_params;

layout(location = 0) out vec4 o_color;

// Distance to a rounded box centered at the origin; negative inside.
float sd_round_box(vec2 p, vec2 half_size, float r) {
    vec2 q = abs(p) - half_size + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    float radius = v_params.x;
    float border = v_params.y;

    float d  = sd_round_box(v_local, v_half, radius);
    float aa = max(fwidth(d), 1e-5); // one pixel, in view units

    float outer = clamp(-d / aa, 0.0, 1.0);            // inside the box
    float inner = clamp(-(d + border) / aa, 0.0, 1.0); // inside the border

    // Blend fill over border in premultiplied space so an outline-only
    // rectangle (fill alpha 0) doesn't pick up a fringe of the fill's RGB.
    vec4 fill = vec4(v_color.rgb * v_color.a, v_color.a);
    vec4 edge = vec4(v_border_color.rgb * v_border_color.a, v_border_color.a);
    vec4 pm   = mix(edge, fill, inner) * outer;

    if (pm.a <= 0.0) discard;
    o_color = vec4(pm.rgb / pm.a, pm.a); // pipeline blend is straight alpha
}
