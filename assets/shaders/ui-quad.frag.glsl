// =============================================================================
// ui-quad.frag.glsl
//
// Instance draw UIQuad render objects. One shader for every UI shape, picked
// per instance by params.z (UIQuadShape). The branch is the same for all
// pixels of a quad, so it costs next to nothing.
//
// Every shape is a signed distance (negative inside) turned into coverage
// with a one-pixel antialiased edge, like rectangle-sdf and text-glyph-sdf.
// =============================================================================

#version 330 core

// The UI font's atlas, bound as the material's texture. Only Glyph uses it.
uniform sampler2D u_texture;

in vec2      v_local;
in vec2      v_half;
in vec2      v_tex;
in vec4      v_color;
in vec4      v_border_color;
flat in vec4 v_params;

layout(location = 0) out vec4 o_color;

const int SHAPE_BOX            = 0;
const int SHAPE_GLYPH          = 1;
const int SHAPE_CROSS          = 2;
const int SHAPE_CHECK          = 3;
const int SHAPE_TRIANGLE_DOWN  = 4;
const int SHAPE_TRIANGLE_RIGHT = 5;
const int SHAPE_CIRCLE         = 6;
const int SHAPE_CORNER         = 7;

// Distance to a rounded box centered at the origin.
float sd_round_box(vec2 p, vec2 half_size, float r) {
    vec2 q = abs(p) - half_size + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// Distance to the segment a-b.
float sd_segment(vec2 p, vec2 a, vec2 b) {
    vec2  pa = p - a;
    vec2  ba = b - a;
    float h  = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

// Distance to a triangle (Inigo Quilez, "2D distance functions").
float sd_triangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
    vec2 v0 = p  - p0, v1 = p  - p1, v2 = p  - p2;
    vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0, 1.0);
    vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0, 1.0);
    vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0, 1.0);
    float s  = sign(e0.x * e2.y - e0.y * e2.x);
    vec2  d  = min(min(vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
                       vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
                       vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
    return -sqrt(d.x) * sign(d.y);
}

// Premultiplied color of a shape with distance d (view units), filled with c.
vec4 shade(float d, vec4 c) {
    float aa = max(fwidth(d), 1e-5);
    float a  = clamp(0.5 - d / aa, 0.0, 1.0) * c.a;
    return vec4(c.rgb * a, a);
}

void main() {
    // Sampled outside the branches so its derivatives are always defined.
    float sdf = texture(u_texture, v_tex).r;

    int   shape  = int(v_params.z + 0.5);
    vec2  p      = v_local;
    vec2  h      = v_half;
    float stroke = 0.5 * v_params.y;

    vec4 pm = vec4(0.0);

    if (shape == SHAPE_BOX) {
        float d  = sd_round_box(p, h, v_params.x);
        float aa = max(fwidth(d), 1e-5);

        float outer = clamp(-d / aa, 0.0, 1.0);                  // inside the box
        float inner = clamp(-(d + v_params.y) / aa, 0.0, 1.0);   // inside the border

        // Fill over border in premultiplied space (see rectangle-sdf.frag)
        vec4 fill = vec4(v_color.rgb * v_color.a, v_color.a);
        vec4 edge = vec4(v_border_color.rgb * v_border_color.a, v_border_color.a);
        pm = mix(edge, fill, inner) * outer;
    }
    else if (shape == SHAPE_GLYPH) {
        // SDF value units; > 0 inside (see text-glyph-sdf.frag)
        float d = sdf - 0.5 + v_params.w;
        float w = max(fwidth(d), 1e-5);
        float a = clamp(d / w + 0.5, 0.0, 1.0) * v_color.a;
        pm = vec4(v_color.rgb * a, a);
    }
    else if (shape == SHAPE_CROSS) {
        vec2  e = h - stroke;
        float d = min(sd_segment(p, -e, e),
                      sd_segment(p, vec2(-e.x, e.y), vec2(e.x, -e.y)));
        pm = shade(d - stroke, v_color);
    }
    else if (shape == SHAPE_CHECK) {
        vec2  e = h - stroke;
        vec2  a = vec2(-0.75, 0.05) * e;
        vec2  b = vec2(-0.20, -0.60) * e;
        vec2  c = vec2( 0.80, 0.65) * e;
        float d = min(sd_segment(p, a, b), sd_segment(p, b, c));
        pm = shade(d - stroke, v_color);
    }
    else if (shape == SHAPE_TRIANGLE_DOWN) {
        float d = sd_triangle(p, vec2(-h.x, h.y), vec2(h.x, h.y), vec2(0.0, -h.y));
        pm = shade(d, v_color);
    }
    else if (shape == SHAPE_TRIANGLE_RIGHT) {
        float d = sd_triangle(p, vec2(-h.x, -h.y), vec2(-h.x, h.y), vec2(h.x, 0.0));
        pm = shade(d, v_color);
    }
    else if (shape == SHAPE_CIRCLE) {
        float d = length(p) - min(h.x, h.y);
        pm = shade(d, v_color);
    }
    else if (shape == SHAPE_CORNER) {
        float d = sd_triangle(p, vec2(h.x, h.y), vec2(h.x, -h.y), vec2(-h.x, -h.y));
        pm = shade(d, v_color);
    }

    if (pm.a <= 0.0) discard;
    o_color = vec4(pm.rgb / pm.a, pm.a); // pipeline blend is straight alpha
}
