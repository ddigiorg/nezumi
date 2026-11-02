// =============================================================================
// post-composite.frag.glsl
//
// lit = linear(scene) * (ambient + light). The scene is the art as drawn
// (sRGB-encoded); ambient and light are linear.
// =============================================================================

#version 330 core

layout(std140) uniform Post {
    vec4 p0; // rgb: ambient, a: 1 = lighting on
    vec4 p1; // x: 1 = show the light alone
    vec4 p2;
    vec4 p3;
};

uniform sampler2D u_tex0; // scene
uniform sampler2D u_tex1; // light (bilinear: it may be smaller)

in vec2 v_uv;

layout(location = 0) out vec4 o_color;

vec3 srgb_to_linear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}

void main() {
    vec3 scene = srgb_to_linear(clamp(texture(u_tex0, v_uv).rgb, 0.0, 1.0));
    vec3 light = p0.rgb + max(texture(u_tex1, v_uv).rgb, vec3(0.0));
    light = mix(vec3(1.0), light, p0.a);

    vec3 lit = mix(scene * light, light, p1.x);
    o_color = vec4(lit, 1.0);
}
