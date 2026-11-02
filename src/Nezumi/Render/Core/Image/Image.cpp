// =============================================================================
// Image.cpp
//
// STB_IMAGE_IMPLEMENTATION must be defined in exactly one .cpp in the program.
// =============================================================================

#include "Nezumi/Render/Core/Image/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm> // max, min
#include <cstring>   // memcpy
#include <stdexcept> // runtime_error
#include <string>    // string

namespace Nezumi::Render {

Image load_image(std::string_view path) {
    const std::string file(path); // stb wants a null-terminated string

    // Rows stay top to bottom as stored in the file: row 0 is v = 0 (see
    // SpriteSheet.hpp). stb's flip flag is global state, so set it explicitly.
    stbi_set_flip_vertically_on_load(false);

    int w = 0;
    int h = 0;
    int channels_in_file = 0;

    stbi_uc* data = stbi_load(
        /*.filename = */file.c_str(),
        /*.x        = */&w,
        /*.y        = */&h,
        /*.comp     = */&channels_in_file,
        /*.desired  = */4
    );
    if (!data) {
        throw std::runtime_error(
            "load_image('" + file + "'): " + stbi_failure_reason()
        );
    }

    Image image = Image::blank(
        /*.width  = */std::uint32_t(w),
        /*.height = */std::uint32_t(h)
    );

    std::memcpy(
        /*.dst = */image.pixels.data(),
        /*.src = */data,
        /*.n   = */image.pixels.size()
    );

    stbi_image_free(data);

    return image;
}

void premultiply_alpha(Image& image) {
    std::uint8_t*     p   = image.pixels.data();
    const std::size_t end = image.pixels.size();
    for (std::size_t i = 0; i < end; i += 4) {
        const unsigned a = p[i + 3];
        if (a == 255) continue;
        // x * a / 255, rounded to nearest, exact for all 8-bit inputs
        for (std::size_t c = 0; c < 3; ++c) {
            const unsigned t = unsigned(p[i + c]) * a + 128;
            p[i + c] = std::uint8_t((t + (t >> 8)) >> 8);
        }
    }
}

Image half_size(const Image& image) {
    const std::uint32_t w = std::max(image.width  / 2, 1u);
    const std::uint32_t h = std::max(image.height / 2, 1u);
    Image out = Image::blank(w, h);

    for (std::uint32_t y = 0; y < h; ++y) {
        // Source rows this output row averages: 2y and 2y+1, plus 2y+2 for
        // the last row of an odd height (so no input row is dropped)
        const std::uint32_t y0 = std::min(2 * y, image.height - 1);
        const std::uint32_t y1 = (y + 1 == h) ? image.height : std::min(2 * y + 2, image.height);

        for (std::uint32_t x = 0; x < w; ++x) {
            const std::uint32_t x0 = std::min(2 * x, image.width - 1);
            const std::uint32_t x1 = (x + 1 == w) ? image.width : std::min(2 * x + 2, image.width);

            unsigned sum[4] = { 0, 0, 0, 0 };
            for (std::uint32_t sy = y0; sy < std::max(y1, y0 + 1); ++sy) {
                for (std::uint32_t sx = x0; sx < std::max(x1, x0 + 1); ++sx) {
                    const std::uint8_t* s = image.at(sx, sy);
                    sum[0] += s[0]; sum[1] += s[1]; sum[2] += s[2]; sum[3] += s[3];
                }
            }

            const unsigned n = (std::max(y1, y0 + 1) - y0) * (std::max(x1, x0 + 1) - x0);
            std::uint8_t*  d = out.at(x, y);
            for (int c = 0; c < 4; ++c) d[c] = std::uint8_t((sum[c] + n / 2) / n);
        }
    }

    return out;
}

} // namespace Nezumi::Render
