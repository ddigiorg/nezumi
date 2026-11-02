// =============================================================================
// point-light.frag.glsl
//
// Smooth radial falloff (see PointLight.hpp). Zero at the rim, so lights
// never leave a hard edge however they overlap.
// =============================================================================

#version 330 core

in vec2 v_local;
in vec3 v_color;
in vec3 v_falloff; // core, softness, inner

layout(location = 0) out vec4 o_color;

void main() {
    float d = length(v_local);
    if (d >= 1.0) discard;

    // Full brightness out to `inner`, then the falloff over the rest
    float inner = clamp(v_falloff.z, 0.0, 0.99);
    d = max(d - inner, 0.0) / (1.0 - inner);

    float d2   = d * d;
    float win  = 1.0 - d2 * d2;
    float hot  = (win * win) / (1.0 + max(v_falloff.x, 0.0) * d2);
    float soft = (1.0 - d2) * (1.0 - d2);
    float i    = mix(hot, soft, clamp(v_falloff.y, 0.0, 1.0));

    o_color = vec4(v_color * i, 0.0);
}
