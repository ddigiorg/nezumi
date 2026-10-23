// =============================================================================
// Image.cpp
//
// STB_IMAGE_IMPLEMENTATION must be defined in exactly one .cpp in the program.
// =============================================================================

#include "Nezumi/Render/Atlas/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>   // memcpy
#include <stdexcept> // runtime_error
#include <string>    // string

namespace Nezumi::Render {

Image load_image(std::string_view path) {
    const std::string file(path); // stb wants a null-terminated string

    // Rows stay top to bottom as stored in the file: row 0 is v = 0 (see
    // Atlas.hpp). stb's flip flag is global state, so set it explicitly.
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

} // namespace Nezumi::Render
