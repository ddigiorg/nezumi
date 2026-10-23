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

#version 330 core

// The font atlas, bound as the material's texture. MaterialTable's bind group
// layout names it, and the GL backend assigns its texture unit from that (see
// GLLimits.hpp).
uniform sampler2D u_texture;

in vec2 v_tex;
in vec4 v_color;
in vec4 v_outline_color;
in vec4 v_params; // outline width, weight, outline offset (uv)

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
    float d_fill    = texture(u_texture, v_tex).r - 0.5 + weight;
    float d_outline = texture(u_texture, v_tex - offset).r - 0.5 + weight + outline_width;

    float fill    = coverage(d_fill)    * v_color.a;
    float outline = coverage(d_outline) * v_outline_color.a;

    // Fill over outline ("over" operator), then back to straight alpha for
    // the pipeline's alpha blend.
    float a = fill + outline * (1.0 - fill);
    if (a <= 0.0) discard;
    vec3 rgb = (v_color.rgb * fill + v_outline_color.rgb * outline * (1.0 - fill)) / a;

    o_color = vec4(rgb, a);
}
