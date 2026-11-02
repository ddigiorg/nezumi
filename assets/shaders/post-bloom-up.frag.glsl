// =============================================================================
// post-bloom-up.frag.glsl
//
// 3x3 tent filter over the level below, added (BlendState::ADD) onto this
// level's downsample.
// =============================================================================

#version 330 core

layout(std140) uniform Post {
    vec4 p0; // x: radius, in source texels
    vec4 p1;
    vec4 p2;
    vec4 p3;
};

uniform sampler2D u_tex0; // the level below

in vec2 v_uv;

layout(location = 0) out vec4 o_color;

vec3 tap(vec2 offset) { return texture(u_tex0, v_uv + offset).rgb; }

void main() {
    vec2 r = p0.x / vec2(textureSize(u_tex0, 0));

    vec3 color = tap(vec2(0.0)) * 4.0
               + (tap(vec2(-r.x, 0.0)) + tap(vec2(r.x, 0.0)) + tap(vec2(0.0, -r.y)) + tap(vec2(0.0, r.y))) * 2.0
               + (tap(-r) + tap(r) + tap(vec2(-r.x, r.y)) + tap(vec2(r.x, -r.y)));

    o_color = vec4(color / 16.0, 0.0);
}
