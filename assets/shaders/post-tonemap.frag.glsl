// =============================================================================
// post-tonemap.frag.glsl
//
// final = encode(tonemap((lit + bloom) * exposure))
// =============================================================================

#version 330 core

layout(std140) uniform Post {
    vec4 p0; // x: exposure, y: bloom strength, z: tonemap (0 clamp, 1 shoulder, 2 ACES), w: 1 = bloom alone
    vec4 p1; // x: shoulder, y: 1 = encode sRGB in the shader
    vec4 p2;
    vec4 p3;
};

uniform sampler2D u_tex0; // lit (linear HDR)
uniform sampler2D u_tex1; // bloom level 0 (half size; the sum of every level)

in vec2 v_uv;

layout(location = 0) out vec4 o_color;

vec3 linear_to_srgb(vec3 c) {
    vec3 lo = c * 12.92;
    vec3 hi = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(lo, hi, step(vec3(0.0031308), c));
}

// Identity below k, then an exponential roll-off that reaches 1 at infinity
vec3 shoulder(vec3 x, float k) {
    vec3 over = max(x - k, 0.0);
    vec3 roll = k + (1.0 - k) * (1.0 - exp(-over / (1.0 - k)));
    return mix(x, roll, step(vec3(k), x));
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 lit   = max(texture(u_tex0, v_uv).rgb, vec3(0.0));
    vec3 bloom = max(texture(u_tex1, v_uv).rgb, vec3(0.0)) * p0.y;

    vec3 hdr = mix(lit + bloom, bloom, p0.w) * p0.x;

    vec3 ldr;
    if      (p0.z > 1.5) ldr = aces(hdr);
    else if (p0.z > 0.5) ldr = shoulder(hdr, p1.x);
    else                 ldr = hdr;
    ldr = clamp(ldr, 0.0, 1.0);

    if (p1.y > 0.5) ldr = linear_to_srgb(ldr);
    o_color = vec4(ldr, 1.0);
}
