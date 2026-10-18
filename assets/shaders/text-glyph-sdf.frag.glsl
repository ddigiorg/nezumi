// =============================================================================
// text-glyph-sdf.frag.glsl
//
// Instance draw TextGlyphSDF render objects.
//
// The font atlas stores FreeType's distance field: 0.5 at the glyph's edge,
// higher inside. Coverage comes from the distance divided by how much it
// changes across one screen pixel (fwidth), so edges stay one pixel soft at any
// zoom, unlike a fixed smoothstep width.
// =============================================================================

#version 460 core

// The font atlas is group 1 / binding 0: GL texture unit 1 * 16 + 0 = 16.
layout(binding = 16) uniform sampler2D u_font;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec4 v_outline_color;
layout(location = 3) in vec4 v_params; // outline width, weight, outline offset (uv)

layout(location = 0) out vec4 o_color;

float coverage(float d) {
    float w = max(fwidth(d), 1e-5);
    return clamp(d / w + 0.5, 0.0, 1.0);
}

void main() {
    float outline_width = v_params.x;
    float weight        = v_params.y;
    vec2  offset        = v_params.zw;

    // Signed distance in SDF value units; > 0 inside.
    float d_fill    = texture(u_font, v_uv).r - 0.5 + weight;
    float d_outline = texture(u_font, v_uv - offset).r - 0.5 + weight + outline_width;

    float fill    = coverage(d_fill)    * v_color.a;
    float outline = coverage(d_outline) * v_outline_color.a;

    // Fill over outline ("over" operator), then back to straight alpha for
    // the pipeline's alpha blend.
    float a = fill + outline * (1.0 - fill);
    if (a <= 0.0) discard;
    vec3 rgb = (v_color.rgb * fill + v_outline_color.rgb * outline * (1.0 - fill)) / a;

    o_color = vec4(rgb, a);
}
