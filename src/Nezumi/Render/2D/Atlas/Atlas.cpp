// =============================================================================
// SpriteAtlas.cpp
// =============================================================================

#include "Nezumi/Render/2D/Atlas/Atlas.hpp"

#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/Render/Core/Material/MaterialTable.hpp"
#include <algorithm> // max, min
#include <span>      // span
#include <stdexcept> // runtime_error
#include <string>    // string, to_string
#include <vector>    // vector

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Atlas::Atlas(
    RenderContext& context,
    const AtlasConfig& config)
        : Atlas(
            /*.context = */context,
            /*.config  = */config,
            /*.image   = */load_image(BasePath::get(config.filepath).string()))
{}

Atlas::Atlas(
    RenderContext&   context,
    const AtlasConfig& config,
    const Image&       image)
        : context_(&context)
{
    create_(config, image);
}

Atlas::~Atlas() {
    remove_();
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void Atlas::create_(const AtlasConfig& config, const Image& image) {
    RHI::Device& device = *context_->device;
    const std::string name = std::string(config.name);

    // Ensure image is not empty
    if (image.empty()) {
        throw std::runtime_error(
            "SpriteAtlas '" + name + "': empty image"
        );
    }

    // Setup image
    width_  = image.width;
    height_ = image.height;

    // Setup sprite table
    // sprites_.reserve(config.sprites.size());
    // for (std::size_t i = 0; i < config.sprites.size(); ++i) {
    //     sprites_.push_back(make_region(config.sprites[i], i, config, width_, height_));
    // }

    // Number of mip levels: 0 = the full chain, otherwise capped at it
    std::uint32_t full_chain = 1;
    for (std::uint32_t s = std::max(width_, height_); s > 1; s /= 2) ++full_chain;
    mip_levels_ = (config.mip_levels == 0) ? full_chain : std::min(config.mip_levels, full_chain);

    // Setup mip chain
    //
    // Premultiplied first (see AtlasConfig.hpp), so filtering and the box
    // averages below weight color by coverage. Rows stay top to bottom and
    // unflipped: row 0 is v = 0 on every backend.
    std::vector<Image> levels;
    levels.reserve(mip_levels_);
    levels.push_back(image);
    premultiply_alpha(levels.back());
    for (std::uint32_t level = 1; level < mip_levels_; ++level) {
        levels.push_back(half_size(levels.back()));
    }

    // Setup texture
    //
    // Mip 0 is uploaded at creation; the smaller levels need CopyDst for
    // write_texture. Static after this. RGBA8Unorm, not Srgb: matches the old
    // renderer's GL_RGBA8, so colors look the same as before.
    const RHI::TextureUsageFlags usage = (mip_levels_ > 1)
        ? RHI::TextureUsageFlags::Sampled | RHI::TextureUsageFlags::CopyDst
        : RHI::TextureUsageFlags::Sampled;

    texture_ = device.create(
        RHI::TextureConfig{
            .name         = name,
            .type         = RHI::TextureType::Texture2DArray, // one layer; every material texture is an array
            .format       = RHI::Format::RGBA8Unorm,
            .width        = width_,
            .height       = height_,
            .mip_levels   = mip_levels_,
            .usage        = usage,
            .initial_data = std::as_bytes(std::span(levels[0].pixels)),
        }
    );
    if (!texture_) {
        remove_();
        throw std::runtime_error(
            "SpriteAtlas: texture '" + name + "' creation failed (see log)"
        );
    }

    for (std::uint32_t level = 1; level < mip_levels_; ++level) {
        device.write_texture(
            texture_,
            RHI::TextureRegion{ .mip_level = level }, // whole level
            std::as_bytes(std::span(levels[level].pixels))
        );
    }

    // Setup sampler
    sampler_ = device.create(
        RHI::SamplerConfig{
            .name           = name,
            .min_filter     = config.min_filter,
            .mag_filter     = config.mag_filter,
            .mip_filter     = config.mip_filter,
            .address_u      = config.address_mode,
            .address_v      = config.address_mode,
            .address_w      = config.address_mode,
            .max_anisotropy = config.max_anisotropy,
            .lod_bias       = config.lod_bias,
        }
    );
    if (!sampler_) {
        remove_();
        throw std::runtime_error(
            "SpriteAtlas: sampler '" + name + "' creation failed (see log)"
        );
    }

    // Setup material in material table
    try {
        material_ = context_->materials->create(name, texture_, sampler_);
    } catch (...) {
        remove_();
        throw;
    }

    // for (SpriteRegion& s : sprites_) s.material = material_;
}

void Atlas::remove_() {
    if (context_) {
        RHI::Device& device = *context_->device;
        context_->materials->remove(material_);
        device.remove(sampler_);
        device.remove(texture_);
    }
    context_  = nullptr;
    material_ = Material::None;
    texture_  = {};
    sampler_  = {};
    mip_levels_ = 1;
    // sprites_.clear();
}

} // namespace Nezumi::Render
