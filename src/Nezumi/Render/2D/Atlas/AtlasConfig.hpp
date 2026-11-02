// =============================================================================
// SpriteAtlasConfig.hpp
//
// Config of a sprite atlas.
//
// Sampling:
//
// - Use `Nearest` for pixel art (crisp at any zoom)
// - Use `Linear` for smooth art, with a 1-2 pixel gap between sprites in the
//   image so neighbours don't bleed in (more with mips; see mip_levels).
//
// Texels are stored premultiplied by alpha (see premultiply_alpha in
// Image.hpp). Author images with straight alpha, as image editors save them;
// the atlas converts them on load.
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

    // What sampling past the image's edge reads. ClampToEdge for sprite
    // sheets; Repeat for a seamless texture sampled across many tiles (e.g.
    // ground), so filtering and mips wrap instead of smearing the edge.
    RHI::SamplerAddressMode address_mode = RHI::SamplerAddressMode::ClampToEdge;

    // How the sampler blends between mip levels (Linear = trilinear)
    RHI::SamplerFilterMode mip_filter = RHI::SamplerFilterMode::Linear;

    // Number of mip levels, generated on the CPU from the image (2x2 box
    // filter in premultiplied alpha). 1 = no mips; 0 = the full chain, down
    // to 1x1. Mips stop zoomed-out sprites from shimmering, and keep the GPU
    // from reading full-size texels for small on-screen sprites.
    //
    // Level k averages 2^k x 2^k pixel blocks, and linear filtering at that
    // level reads one more texel on each side. So for n levels, put sprite
    // boxes on a 2^(n-1) pixel grid, or leave 2^(n-1) pixels of transparent
    // padding between sprites; otherwise neighbors bleed into each other at
    // the smaller levels. 4 levels (8 px) covers zooming out to 1/8 size.
    std::uint32_t mip_levels = 1;

    // Anisotropic filtering (1 = off). Helps textures squashed more along one
    // axis than the other, e.g. square ground textures drawn as 2:1 iso
    // diamonds. Clamped to what the device supports.
    float max_anisotropy = 1.0f;

    // Added to the mip level the GPU picks (negative = sharper, at the cost
    // of some shimmer). Trilinear filtering blends toward the next smaller
    // level as soon as a texture is minified at all, which can look soft;
    // -0.5 is a common middle ground for detailed ground textures.
    float lod_bias = 0.0f;

    // std::span<const SpritePixelBox> sprites = {};
};

} // namespace Nezumi::Render
