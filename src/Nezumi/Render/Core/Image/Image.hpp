// =============================================================================
// Image.hpp
//
// Decoded RGBA8 pixels on the CPU, rows top to bottom (as image files store
// them). Input to SpriteSheet and Font; never flipped.
// =============================================================================

#pragma once
#include <cstddef>     // size_t
#include <cstdint>     // uint8_t, uint32_t
#include <string_view> // string_view
#include <vector>      // vector

namespace Nezumi::Render {

struct Image {

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::uint32_t             width  = 0;
    std::uint32_t             height = 0;
    std::vector<std::uint8_t> pixels;     // width * height * 4, RGBA

    // -------------------------------------------------------------------------
    // At
    //
    // Pointer to the pixel at (x, y), y counted from the top row.
    // -------------------------------------------------------------------------

    std::uint8_t* at(std::uint32_t x, std::uint32_t y) {
        return pixels.data() + 4 * (std::size_t(y) * width + x);
    }

    const std::uint8_t* at(std::uint32_t x, std::uint32_t y) const {
        return pixels.data() + 4 * (std::size_t(y) * width + x);
    }

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    bool empty() const { return width == 0 || height == 0; }

    static Image blank(std::uint32_t width, std::uint32_t height) {
        return Image{
            .width  = width,
            .height = height,
            .pixels = std::vector<std::uint8_t>(std::size_t(width) * height * 4, 0),
        };
    }
};

// -----------------------------------------------------------------------------
// Load Image
//
// Loads any format stb_image supports, converted to RGBA8. Throws on failure.
// -----------------------------------------------------------------------------

Image load_image(std::string_view path);

// -----------------------------------------------------------------------------
// Premultiply Alpha
//
// Multiplies each pixel's RGB by its alpha, in place (rounded to nearest).
// Sprite sheets store premultiplied texels: linear filtering and mip averaging then
// weight each texel's color by its coverage, so transparent pixels can't drag
// their (usually black) RGB into sprite edges as dark fringes.
// -----------------------------------------------------------------------------

void premultiply_alpha(Image& image);

// -----------------------------------------------------------------------------
// Half Size
//
// The next mip level: each output pixel is the 2x2 box average of the input
// pixels it covers (rounded to nearest). Sizes halve and round down, never
// below 1, like GPU mip extents; an odd last row or column is folded into
// its neighbor's average. Expects premultiplied input.
// -----------------------------------------------------------------------------

Image half_size(const Image& image);

} // namespace Nezumi::Render
