// =============================================================================
// post-bloom-down.frag.glsl
//
// 13-tap downsample (Jimenez 2014): 5 overlapping 2x2 boxes, weighted
// 0.5 for the center box and 0.125 for each corner box. Into the first
// level it also applies the soft threshold, and Karis-averages each box
// (weights 1 / (1 + luma)) so single bright pixels don't flicker.
// =============================================================================

#version 330 core

layout(std140) uniform Post {
    vec4 p0; // x: threshold, y: knee, z: 1 = first level
    vec4 p1;
    vec4 p2;
    vec4 p3;
};

uniform sampler2D u_tex0; // the level above (the lit image for level 0)

in vec2 v_uv;

layout(location = 0) out vec4 o_color;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

vec3 prefilter(vec3 c) {
    float threshold = p0.x;
    float knee      = p0.y;
    float br   = max(c.r, max(c.g, c.b));
    float soft = clamp(br - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-4);
    return c * (max(soft, br - threshold) / max(br, 1e-4));
}

vec3 box(vec3 a, vec3 b, vec3 c, vec3 d) {
    if (p0.z < 0.5) return (a + b + c + d) * 0.25;
    a = prefilter(a); b = prefilter(b); c = prefilter(c); d = prefilter(d);
    float wa = 1.0 / (1.0 + luma(a));
    float wb = 1.0 / (1.0 + luma(b));
    float wc = 1.0 / (1.0 + luma(c));
    float wd = 1.0 / (1.0 + luma(d));
    return (a * wa + b * wb + c * wc + d * wd) / (wa + wb + wc + wd);
}

vec3 tap(vec2 offset) {
    return max(texture(u_tex0, v_uv + offset).rgb, vec3(0.0));
}

void main() {
    vec2 t = 1.0 / vec2(textureSize(u_tex0, 0));

    vec3 a = tap(t * vec2(-2.0,  2.0));
    vec3 b = tap(t * vec2( 0.0,  2.0));
    vec3 c = tap(t * vec2( 2.0,  2.0));
    vec3 d = tap(t * vec2(-2.0,  0.0));
    vec3 e = tap(t * vec2( 0.0,  0.0));
    vec3 f = tap(t * vec2( 2.0,  0.0));
    vec3 g = tap(t * vec2(-2.0, -2.0));
    vec3 h = tap(t * vec2( 0.0, -2.0));
    vec3 i = tap(t * vec2( 2.0, -2.0));
    vec3 j = tap(t * vec2(-1.0,  1.0));
    vec3 k = tap(t * vec2( 1.0,  1.0));
    vec3 l = tap(t * vec2(-1.0, -1.0));
    vec3 m = tap(t * vec2( 1.0, -1.0));

    vec3 color = box(j, k, l, m) * 0.5
               + box(a, b, d, e) * 0.125
               + box(b, c, e, f) * 0.125
               + box(d, e, g, h) * 0.125
               + box(e, f, h, i) * 0.125;

    o_color = vec4(color, 1.0);
}
