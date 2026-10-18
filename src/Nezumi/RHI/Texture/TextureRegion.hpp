// =============================================================================
// TextureRegion.hpp
//
// The destination of a Device::write_texture call.
//
// A default-constructed region means "all of mip 0, layer 0": a zero width,
// height, or depth extends to the edge of the selected mip level.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

struct TextureRegion {
    std::uint32_t mip_level = 0;

    // Origin within the mip level. For array textures, z is the first layer.
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;

    // Size of the region. 0 = to the edge of the mip level.
    std::uint32_t width           = 0;
    std::uint32_t height          = 0;
    std::uint32_t depth_or_layers = 1;

    // Bytes between the starts of consecutive rows in the source data.
    // 0 = tightly packed (width * bytes per pixel).
    std::uint32_t bytes_per_row = 0;
};

} // namespace Nezumi::RHI
