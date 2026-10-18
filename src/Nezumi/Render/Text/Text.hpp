// =============================================================================
// Text.hpp
//
// Text style and layout. Layout runs on the CPU each time text is drawn:
// it walks the string, places one quad per visible glyph, and handles '\n',
// horizontal alignment and vertical anchoring. The same code measures text,
// so measure_text() always matches what draw() puts on screen.
//
// Coordinates are world space, y up. `position` is where the text anchors:
// by default the left end of the first line's baseline.
//
//   renderer.draw(font, "Score: 42", { 10, 20 }, { .size = 0.75f });
//
//   const TextStyle label = { .size = 0.4f, .align = TextAlign::Center,
//                             .valign = TextVAlign::Bottom,
//                             .outline_width = 2.0f };  // black outline
//   renderer.draw(font, unit.name, unit.pos + glm::vec2(0, 1.2f), label,
//                 { .layer = Layer::Overlay });
//
// Not handled (yet): kerning, UTF-8 / non-ASCII, word wrapping, tabs.
// =============================================================================

#pragma once
#include "Nezumi/Render/Text/Font.hpp"
#include "Nezumi/Render/Text/FontGlyph.hpp"
#include <algorithm>   // count, min, max
#include <cstddef>     // size_t
#include <cstdint>     // uint8_t
#include <glm/glm.hpp> // vec2, vec4
#include <limits>      // numeric_limits
#include <string_view> // string_view

namespace Nezumi::Render {

enum class TextAlign : std::uint8_t {
    Left,    // lines start at position.x
    Center,  // lines are centered on position.x
    Right,   // lines end at position.x
};

enum class TextVAlign : std::uint8_t {
    Baseline, // position.y is the first line's baseline
    Top,      // position.y is the top of the first line (its ascender)
    Middle,   // position.y is the middle of the whole block
    Bottom,   // position.y is the bottom of the last line (its descender)
};

struct TextStyle {
    // Em size in world units (roughly the height of a capital plus some
    // headroom). Glyphs scale by size / font.pixel_size().
    float size = 16.0f;

    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Thicken (> 0) or thin (< 0) every glyph, in font atlas pixels.
    float weight = 0.0f;

    // Outline around each glyph, in font atlas pixels; 0 = none. With an
    // offset, the outline becomes a drop shadow (offset in atlas pixels,
    // y up; e.g. { 2, -2 } = down-right). weight + outline_width and the
    // offset should stay within the font's sdf_spread.
    glm::vec4 outline_color  = { 0.0f, 0.0f, 0.0f, 1.0f };
    float     outline_width  = 0.0f;
    glm::vec2 outline_offset = { 0.0f, 0.0f };

    float      line_spacing = 1.0f;              // x the font's line height
    TextAlign  align        = TextAlign::Left;
    TextVAlign valign       = TextVAlign::Baseline;

    bool has_outline() const {
        return outline_color.a > 0.0f &&
               (outline_width > 0.0f || outline_offset != glm::vec2(0.0f));
    }
};

// -----------------------------------------------------------------------------
// Layout
//
// Calls emit(const Glyph&, glm::vec4 pos_box) for each visible glyph, in
// string order, and returns the block's bounds { min_x, min_y, max_x, max_y }:
// widths from glyph advances, heights from the font's ascender/descender.
// Characters the font lacks are drawn as '?' (or skipped if it has no '?').
// -----------------------------------------------------------------------------

namespace detail {
    inline float line_width(const Font& font, std::string_view line, float scale) {
        float w = 0.0f;
        for (char c : line) {
            if (const FontGlyph* g = font.glyph_or_fallback(c)) w += g->advance * scale;
        }
        return w;
    }
} // namespace detail

template <typename Emit>
glm::vec4 layout_text(const Font& font, std::string_view text, glm::vec2 position,
                      const TextStyle& style, Emit&& emit)
{
    const float scale   = font.pixel_size() > 0.0f ? style.size / font.pixel_size() : 0.0f;
    const float line_h  = font.line_height() * style.line_spacing * scale;
    const float ascent  = font.ascender()  * scale;
    const float descent = font.descender() * scale;   // negative

    const std::size_t lines   = 1 + std::size_t(std::count(text.begin(), text.end(), '\n'));
    const float       extra   = float(lines - 1) * line_h;
    const float       block_h = (ascent - descent) + extra;

    float baseline = position.y;
    switch (style.valign) {
        case TextVAlign::Baseline: baseline = position.y;                          break;
        case TextVAlign::Top:      baseline = position.y - ascent;                 break;
        case TextVAlign::Middle:   baseline = position.y + 0.5f * block_h - ascent; break;
        case TextVAlign::Bottom:   baseline = position.y - descent + extra;         break;
    }
    const float first_baseline = baseline;

    float min_x =  std::numeric_limits<float>::max();
    float max_x = -std::numeric_limits<float>::max();

    std::size_t start = 0;
    for (;;) {
        const std::size_t end  = text.find('\n', start);
        const std::string_view line = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);

        const float width = detail::line_width(font, line, scale);
        float pen = position.x;
        if (style.align == TextAlign::Center) pen -= 0.5f * width;
        if (style.align == TextAlign::Right)  pen -= width;

        min_x = std::min(min_x, pen);
        max_x = std::max(max_x, pen + width);

        for (char c : line) {
            const FontGlyph* g = font.glyph_or_fallback(c);
            if (!g) continue;
            if (g->visible()) {
                const float x0  = pen + g->bearing.x * scale;
                const float top = baseline + g->bearing.y * scale;
                emit(*g, glm::vec4{ x0, top - g->size.y * scale, x0 + g->size.x * scale, top });
            }
            pen += g->advance * scale;
        }

        if (end == std::string_view::npos) break;
        start     = end + 1;
        baseline -= line_h;
    }

    return { min_x, first_baseline - extra + descent, max_x, first_baseline + ascent };
}

// Bounds of `text` drawn at `position` with `style`, without drawing it.
inline glm::vec4 measure_text(const Font& font, std::string_view text, glm::vec2 position,
                              const TextStyle& style)
{
    return layout_text(font, text, position, style, [](const FontGlyph&, const glm::vec4&) {});
}

// Width and height of `text` with `style`.
inline glm::vec2 measure_text(const Font& font, std::string_view text, const TextStyle& style) {
    const glm::vec4 b = measure_text(font, text, { 0.0f, 0.0f }, style);
    return { b.z - b.x, b.w - b.y };
}

} // namespace Nezumi::Render
