// =============================================================================
// SpriteAtlas.hpp
//
// A pre-made sprite atlas: one image, uploaded as one texture, sampled with
// one sampler, registered as one material (bind group 1), with a table of
// sprites inside it. All sprites in an atlas batch together, since they
// share the material.
//
// Texel convention: v = 0 is the TOP row of the image, as stored in the file.
// The image is never flipped; the sprite shader maps a quad's bottom edge to
// the sprite's bottom row.
// =============================================================================

#pragma once
#include "Nezumi/Render/Atlas/AtlasConfig.hpp"
#include "Nezumi/Render/Atlas/Image.hpp"
#include "Nezumi/Render/Material/MaterialID.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

class Atlas {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Loads config.filepath.
    Atlas(RendererContext& context, const AtlasConfig& config);

    // Uses an image you already have (generated, or loaded some other way);
    // config.filepath is ignored.
    Atlas(
        RendererContext&   context,
        const AtlasConfig& config,
        const Image&       image);

    ~Atlas();

    // Non-copyable and non-movable
    Atlas(const Atlas&)            = delete;
    Atlas& operator=(const Atlas&) = delete;
    Atlas(Atlas&&)                 = delete;
    Atlas& operator=(Atlas&&)      = delete;

    // -------------------------------------------------------------------------
    // Sprites
    // -------------------------------------------------------------------------

    // Image pixels { x0, y0, x1, y1 } (y down, row 0 at the top, as in an
    // image editor) to the UVs Sprite::tex_box expects.
    glm::vec4 tex_box(glm::vec4 pixel_box) const {
        const float w = float(width_);
        const float h = float(height_);
        return pixel_box / glm::vec4{ w, h, w, h };
    }

    // const SpriteRegion& operator[](SpriteID id) const { return sprites_[id]; }

    // std::size_t sprite_count() const { return sprites_.size(); }

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    MaterialID         material() const { return material_; }
    RHI::TextureHandle texture()  const { return texture_;  }
    RHI::SamplerHandle sampler()  const { return sampler_;  }
    std::uint32_t      width()    const { return width_;    }
    std::uint32_t      height()   const { return height_;   }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    void create_(const AtlasConfig& config, const Image& image);
    void remove_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RendererContext*   context_  = nullptr;
    MaterialID         material_ = Material::None;
    RHI::TextureHandle texture_  = {};
    RHI::SamplerHandle sampler_  = {};
    std::uint32_t      width_    = 0;
    std::uint32_t      height_   = 0;

    // std::vector<SpriteRegion> sprites_;
};

} // namespace Nezumi::Render
