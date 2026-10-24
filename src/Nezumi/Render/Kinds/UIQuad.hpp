// =============================================================================
// UIQuad.hpp
//
// Per-instance data for the UI: one quad that is either a rounded box, an SDF
// text glyph, or a small vector icon, clipped to a box. The UI module
// (Nezumi/UI) builds these; you normally never fill one in yourself.
//
// Why one kind for everything: the renderer orders draws with the same layer
// and order by kind, then material. With boxes and text as separate kinds, a
// window's text would draw above (or below) every other window's boxes, not
// just its own. With one kind and one material (the UI font's atlas), the
// whole UI keeps submission order and still draws in a single instanced call.
//
// Why the clip box: windows clip their content (scrolled or resized), and
// the renderer has no scissor state. Clipping happens in the vertex shader
// by shrinking the quad, which also shrinks the UVs and local coordinates,
// so a clipped glyph or rounded corner looks exactly like the visible part
// of the unclipped one. No discard, no extra state, no extra draw calls.
// =============================================================================

#pragma once
#include <cstdint>     // uint8_t
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

// What the fragment shader draws inside pos_box (params.z).
enum class UIQuadShape : std::uint8_t {
    Box           = 0, // rounded box with optional border (like RectangleSDF)
    Glyph         = 1, // SDF text glyph from the material's atlas (tex_box)
    Cross         = 2, // X, for close buttons (stroke = border_width)
    Check         = 3, // check mark (stroke = border_width)
    TriangleDown  = 4, // dropdown arrow
    TriangleRight = 5, // collapsed section arrow
    Circle        = 6, // filled circle in the box (toggle knob, radio)
    Corner        = 7, // bottom-right half of the box (resize grip)
};

struct UIQuad {
    glm::vec4 pos_box      = { 0.0f, 0.0f, 0.0f, 0.0f }; // view { min_x, min_y, max_x, max_y }
    glm::vec4 tex_box      = { 0.0f, 0.0f, 0.0f, 0.0f }; // Glyph: atlas { min_u, min_v, max_u, max_v }, v down
    glm::vec4 color        = { 1.0f, 1.0f, 1.0f, 1.0f }; // fill (or glyph / icon color)
    glm::vec4 border_color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Box: border
    glm::vec4 clip_box     = { -1e9f, -1e9f, 1e9f, 1e9f }; // view; nothing outside is drawn

    // x: corner radius (Box)
    // y: border width (Box) or stroke width (Cross, Check), in view units
    // z: UIQuadShape
    // w: glyph weight, in SDF value units (Glyph; see TextGlyphSDF)
    glm::vec4 params       = { 0.0f, 0.0f, 0.0f, 0.0f };
};

static_assert(sizeof(UIQuad) == 96, "UIQuad is 96 bytes");

} // namespace Nezumi::Render
