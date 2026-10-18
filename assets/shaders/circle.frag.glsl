// =============================================================================
// circle.frag.glsl
//
// Instance draw Circle render objects.
//
// The ring between radius - width and radius, with one-pixel antialiased edges.
//
// width >= radius puts the inner edge at or below zero, which leaves it filled.
// =============================================================================

#version 460 core

layout(location = 0) in vec2 v_local;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec2 v_params; // { radius, width }

layout(location = 0) out vec4 o_color;

void main() {
    float radius = v_params.x;
    float inner  = radius - v_params.y;   // <= 0: filled

    float d  = length(v_local);           // distance from the center
    float aa = max(fwidth(d), 1e-5);      // one pixel, in world units

    float outside_inner = inner > 0.0 ? clamp(0.5 + (d - inner) / aa, 0.0, 1.0) : 1.0;
    float inside_outer  = clamp(0.5 - (d - radius) / aa, 0.0, 1.0);
    float coverage      = inside_outer * outside_inner;
    if (coverage <= 0.0) discard;

    o_color = vec4(v_color.rgb, v_color.a * coverage);
}
