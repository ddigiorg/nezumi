// =============================================================================
// SpriteAtlasConfig.hpp
//
// Config of a sprite atlas.
//
// Sampling:
//
// - Use `Nearest` for pixel art (crisp at any zoom)
// - Use `Linear` for smooth art, with a 1-2 pixel gap between sprites in the
//   image so neighbours don't bleed in.
// =============================================================================

#pragma once
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec2
#include <string_view> // string_view

namespace Nezumi::Render {

struct AtlasConfig {
    std::string_view name     = "atlas";
    std::string_view filepath = {};

    // How the texture is sampled when it is minified (zoomed out)
    RHI::SamplerFilterMode min_filter = RHI::SamplerFilterMode::Linear;

    // How the texture is sampled when it is magnified (zoomed in )
    RHI::SamplerFilterMode mag_filter = RHI::SamplerFilterMode::Linear;

    // Must be 1 for now; anything else throws.
    //
    // TODO: to support mip_levels > 1:
    //  1. Generate the chain on the CPU: each level is the previous one
    //     halved (2x2 box average, never below 1x1), down to mip_levels.
    //     Average in premultiplied alpha, or transparent pixels' RGB bleeds
    //     dark fringes into sprite edges at smaller levels.
    //  2. Create the texture with mip_levels set and usage
    //     Sampled | CopyDst (write_texture rejects textures without CopyDst).
    //     initial_data still fills level 0.
    //  3. Upload levels 1..n-1 with device.write_texture(texture,
    //     { .mip_level = i }, level_pixels) (a zero width/height region
    //     covers the whole level).
    //  4. Add mip_filter to this config (Linear blends between levels) and
    //     pass it to the sampler; min_filter still picks within a level.
    //  5. Sprites must not share 2^k x 2^k blocks at the levels you keep, or
    //     the averaging blends neighbours: put sprite boxes on a 2^(n-1) pixel
    //     grid (the test atlas's 64-pixel grid allows up to 7 levels), or pad
    //     between them by 2^(n-1) pixels.
    //  (Alternative: add a generate_mipmaps(texture) call to the RHI that maps
    //  to glGenerateTextureMipmap, replacing steps 1 and 3; step 5 still
    //  applies.)
    std::uint32_t mip_levels = 1;

    // std::span<const SpritePixelBox> sprites = {};
};

} // namespace Nezumi::Render
