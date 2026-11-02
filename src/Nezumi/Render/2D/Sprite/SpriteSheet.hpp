// =============================================================================
// SpriteSheet.hpp
//
// Sprite pages as one Texture2DArray (one layer per page), with a table of
// named sprites and animations. One texture, one sampler, one material: every
// sprite on every page batches with every other.
//
// Loaded from a manifest a packer writes (format below), or made from images
// you already have (no named sprites; address them with tex_box()). Texels
// are stored premultiplied (see SpriteSheetConfig.hpp).
//
// Texel convention: v = 0 is the TOP row of a page, as stored in the file.
// Pages are never flipped; the sprite shader maps a quad's bottom edge to
// the sprite's bottom row.
//
//   SpriteSheet& units = *renderer.create_sprite_sheet({
//       .name = "units", .manifest = "assets/graphics/units.json", .mip_levels = 4 }).object;
//
//   const AnimationID walk = units.animation("knight/walk");   // once, at load
//   ...
//   const SpriteSheet::Frame f = units.frame(walk, heading, seconds);
//   renderer.draw_sprite(units.sprite_at(f, feet, view_units_per_pixel),
//                        { .material = units.material() });
//
// ## Manifest (version 1)
//
//   {
//     "version": 1,
//     "page_size": [2048, 2048],
//     "pages": ["units-0.png", "units-1.png"],          // relative to the manifest
//     "sprites": [
//       { "name": "knight/walk/0/0", "page": 0,
//         "box":    [x0, y0, x1, y1],                    // pixels on the page, y down
//         "offset": [ox, oy],                            // optional, default [0, 0]
//         "pivot":  [px, py] },                          // optional, default bottom center
//       ...
//     ],
//     "animations": [                                    // optional
//       { "name": "knight/walk", "first": 0, "frames": 16,
//         "directions": 9, "headings": 16, "fps": 12, "loop": true },
//       ...
//     ]
//   }
//
// offset and pivot are in the untrimmed frame (see SpriteRegion.hpp). Every
// page must be page_size. "headings" defaults to "directions" (no mirroring).
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kinds/Sprite.hpp"
#include "Nezumi/Render/2D/Sprite/Animation.hpp"
#include "Nezumi/Render/2D/Sprite/SpriteID.hpp"
#include "Nezumi/Render/2D/Sprite/SpriteRegion.hpp"
#include "Nezumi/Render/2D/Sprite/SpriteSheetConfig.hpp"
#include "Nezumi/Render/Core/Image/Image.hpp"
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstddef>       // size_t
#include <cstdint>       // uint32_t
#include <functional>    // equal_to
#include <glm/glm.hpp>   // vec2, vec4
#include <span>          // span
#include <string>        // string
#include <string_view>   // string_view
#include <unordered_map> // unordered_map
#include <vector>        // vector

namespace Nezumi::Render {

class SpriteSheet {
public:

    // -------------------------------------------------------------------------
    // Frame
    //
    // The sprite to draw for an animation at a heading and time.
    // -------------------------------------------------------------------------

    struct Frame {
        SpriteID sprite   = 0;
        bool     mirrored = false; // draw flipped left to right
    };

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Loads config.manifest and its pages. Throws on any error.
    SpriteSheet(RenderContext& context, const SpriteSheetConfig& config);

    // Uses images you already have, one per layer, all the same size;
    // config.manifest is ignored. No named sprites: use tex_box().
    SpriteSheet(
        RenderContext&          context,
        const SpriteSheetConfig& config,
        std::span<const Image>   pages);

    ~SpriteSheet();

    // Non-copyable and non-movable
    SpriteSheet(const SpriteSheet&)            = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;
    SpriteSheet(SpriteSheet&&)                 = delete;
    SpriteSheet& operator=(SpriteSheet&&)      = delete;

    // -------------------------------------------------------------------------
    // Lookup (by name: at load time, not per frame)
    // -------------------------------------------------------------------------

    // Throw if the name isn't in the sheet
    SpriteID    sprite   (std::string_view name) const;
    AnimationID animation(std::string_view name) const;

    // Null if the name isn't in the sheet
    const SpriteID*    find_sprite   (std::string_view name) const;
    const AnimationID* find_animation(std::string_view name) const;

    // -------------------------------------------------------------------------
    // Tables
    // -------------------------------------------------------------------------

    const SpriteRegion& region(SpriteID id)    const { return regions_[id];    }
    const Animation&    anim  (AnimationID id) const { return animations_[id]; }

    std::size_t sprite_count()    const { return regions_.size();    }
    std::size_t animation_count() const { return animations_.size(); }

    // -------------------------------------------------------------------------
    // Animation
    // -------------------------------------------------------------------------

    // heading in [0, anim.headings) (wrapped); seconds since the animation
    // started. Looping animations wrap; others hold their last frame.
    Frame frame(AnimationID id, std::uint32_t heading, float seconds) const;

    // -------------------------------------------------------------------------
    // Drawing
    // -------------------------------------------------------------------------

    // A Sprite whose pivot lands on `anchor` (view space, y up), at `scale`
    // view units per sprite pixel. Mirroring flips it about the anchor.
    Sprite sprite_at(SpriteID id, glm::vec2 anchor, float scale, bool mirrored = false) const;

    Sprite sprite_at(Frame f, glm::vec2 anchor, float scale) const {
        return sprite_at(f.sprite, anchor, scale, f.mirrored);
    }

    // Page pixels { x0, y0, x1, y1 } (y down) to UVs, for sheets without
    // named sprites
    glm::vec4 tex_box(glm::vec4 pixel_box) const {
        const float w = float(page_width_);
        const float h = float(page_height_);
        return pixel_box / glm::vec4{ w, h, w, h };
    }

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    MaterialID         material()    const { return material_;    }
    RHI::TextureHandle texture()     const { return texture_;     }
    RHI::SamplerHandle sampler()     const { return sampler_;     }
    std::uint32_t      page_width()  const { return page_width_;  }
    std::uint32_t      page_height() const { return page_height_; }
    std::uint32_t      pages()       const { return pages_;       }
    std::uint32_t      mip_levels()  const { return mip_levels_;  }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    void load_manifest_(const SpriteSheetConfig& config, std::vector<Image>& pages);
    void create_(const SpriteSheetConfig& config, std::span<const Image> pages);
    void remove_();

    // Transparent hash, so lookups by string_view don't allocate
    struct NameHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
    };
    using NameMap = std::unordered_map<std::string, std::uint32_t, NameHash, std::equal_to<>>;

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RenderContext*     context_     = nullptr;
    std::string        name_;
    MaterialID         material_    = Material::None;
    RHI::TextureHandle texture_     = {};
    RHI::SamplerHandle sampler_     = {};
    std::uint32_t      page_width_  = 0;
    std::uint32_t      page_height_ = 0;
    std::uint32_t      pages_       = 0;
    std::uint32_t      mip_levels_  = 1;

    std::vector<SpriteRegion> regions_;
    std::vector<Animation>    animations_;
    NameMap                   sprite_names_;
    NameMap                   animation_names_;
};

} // namespace Nezumi::Render
