// =============================================================================
// SpriteSheetConfig.hpp
//
// Config of a SpriteSheet. The sampling fields work as in AtlasConfig.
// =============================================================================

#pragma once
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view

namespace Nezumi::Render {

struct SpriteSheetConfig {
    std::string_view name     = "sprite-sheet";

    // The sheet's manifest (sprites.json, written by a packer), relative to
    // the base path. Page images are relative to the manifest's folder.
    // Ignored by the constructor that takes images.
    std::string_view manifest = {};

    RHI::SamplerFilterMode  min_filter     = RHI::SamplerFilterMode::Linear;
    RHI::SamplerFilterMode  mag_filter     = RHI::SamplerFilterMode::Linear;
    RHI::SamplerAddressMode address_mode   = RHI::SamplerAddressMode::ClampToEdge;
    RHI::SamplerFilterMode  mip_filter     = RHI::SamplerFilterMode::Linear;

    // 1 = no mips, 0 = full chain. Sprites need 2^(n-1) px of padding or a
    // 2^(n-1) px grid (see AtlasConfig::mip_levels).
    std::uint32_t mip_levels     = 1;
    float         max_anisotropy = 1.0f;
    float         lod_bias       = 0.0f;
};

} // namespace Nezumi::Render
