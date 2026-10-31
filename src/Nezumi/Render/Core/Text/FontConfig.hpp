// =============================================================================
// FontConfig.hpp
//
// Config of a font.
// =============================================================================

#pragma once
#include <cstdint>     // uint32_t
#include <string_view> // string_view

namespace Nezumi::Render {

struct FontConfig {
    std::string_view name     = "font";
    std::string_view filepath = {};   // any format FreeType reads (.ttf, .otf, ...)

    // Em size, in pixels, that glyphs are rendered at into the atlas. Text
    // can be drawn at any size from one atlas; this sets how much detail the
    // distance field holds. 48-64 is plenty for UI and world labels.
    std::uint32_t pixel_size = 64;

    // How far, in atlas pixels, the distance field reaches on each side of a
    // glyph's edge (FreeType allows 2-32). Outlines, shadow offsets and
    // weight changes must stay within it: weight + outline_width < spread.
    std::uint32_t sdf_spread = 8;
};

} // namespace Nezumi::Render
