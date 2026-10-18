// =============================================================================
// SpriteAtlas.cpp
// =============================================================================

#include "Nezumi/Render/Atlas/Atlas.hpp"

#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/Render/Material/MaterialTable.hpp"
#include <span>      // span
#include <stdexcept> // runtime_error
#include <string>    // string, to_string

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Atlas::Atlas(
    RendererContext& context,
    const AtlasConfig& config)
        : Atlas(
            /*.context = */context,
            /*.config  = */config,
            /*.image   = */load_image(BasePath::get(config.filepath).string()))
{}

Atlas::Atlas(
    RendererContext&   context,
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

    // See TODO note in SpriteAtlasConfig::mip_levels.
    if (config.mip_levels != 1) {
        throw std::runtime_error(
            "SpriteAtlas '" + name + "': only 1 mip level is supported for now"
        );
    }

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

    // Setup texture
    //
    // Uploaded at creation, rows top to bottom and unflipped: row 0 is v = 0
    // on every backend. Static after this, so Sampled only (no CopyDst).
    // RGBA8Unorm, not Srgb: matches the old renderer's GL_RGBA8, so colors
    // look the same as before.
    texture_ = device.create(
        RHI::TextureConfig{
            .name         = name,
            .type         = RHI::TextureType::Texture2D,
            .format       = RHI::Format::RGBA8Unorm,
            .width        = image.width,
            .height       = image.height,
            .mip_levels   = config.mip_levels, // 1; see TODO in SpriteAtlasConfig::mip_levels.
            .usage        = RHI::TextureUsageFlags::Sampled,
            .initial_data = std::as_bytes(std::span(image.pixels)),
        }
    );
    if (!texture_) {
        remove_();
        throw std::runtime_error(
            "SpriteAtlas: texture '" + name + "' creation failed (see log)"
        );
    }

    // Setup sampler
    sampler_ = device.create(
        RHI::SamplerConfig{
            .name       = name,
            .min_filter = config.min_filter,
            .mag_filter = config.mag_filter,
            .mip_filter = RHI::SamplerFilterMode::Nearest,
            .address_u  = RHI::SamplerAddressMode::ClampToEdge,
            .address_v  = RHI::SamplerAddressMode::ClampToEdge,
            .address_w  = RHI::SamplerAddressMode::ClampToEdge,
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
    // sprites_.clear();
}

} // namespace Nezumi::Render
