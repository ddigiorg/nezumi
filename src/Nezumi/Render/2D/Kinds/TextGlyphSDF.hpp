// =============================================================================
// TextGlyphSDF.hpp
//
// Per-instance data for one glyph of SDF text, uploaded as-is (80 bytes).
// Built by Renderer2D::draw(const Font&, ...) from layout_text() and a
// TextStyle; you normally never fill one in yourself.
//
// The style parameters are pre-converted on the CPU into the units the
// shader works in, so the shader needs neither the font's spread nor the
// atlas size:
//   params.x  outline width   (SDF value units: atlas px / (2 * spread))
//   params.y  weight          (SDF value units)
//   params.zw outline offset  (UV units, v down)
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

struct TextGlyphSDF {
    glm::vec4 pos_box       = { 0.0f, 0.0f, 0.0f, 0.0f }; // world { min_x, min_y, max_x, max_y }
    glm::vec4 tex_box       = { 0.0f, 0.0f, 0.0f, 0.0f }; // atlas { min_u, min_v, max_u, max_v }, v down
    glm::vec4 color         = { 1.0f, 1.0f, 1.0f, 1.0f }; // fill; alpha 0 = outline only
    glm::vec4 outline_color = { 0.0f, 0.0f, 0.0f, 0.0f }; // alpha 0 = no outline
    glm::vec4 params        = { 0.0f, 0.0f, 0.0f, 0.0f }; // see above
};

static_assert(sizeof(TextGlyphSDF) == 80, "TextGlyphSDF is 80 bytes");

} // namespace Nezumi::Render
