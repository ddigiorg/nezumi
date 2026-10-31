// =============================================================================
// Font.hpp
//
// A font loaded with FreeType and rendered once, at load time, into a signed
// distance field (SDF) atlas: one R8 texture registered as a material, plus a
// table of glyph metrics for layout. Covers printable ASCII (32-126).
//
// Atlas values follow FreeType's SDF output: 0.5 is the glyph edge, higher
// is inside, and one atlas pixel of distance is 1 / (2 * sdf_spread).
//
// All metrics are in atlas pixels (at config.pixel_size). Layout scales them
// by text size / pixel_size; see Text.hpp.
//
// Lifetime: a Font owns its material and frees it on destruction. Destroy
// fonts before the RenderCore that owns the MaterialTable. FreeType is only
// used inside the constructor; nothing FreeType leaks into this header.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/Render/Core/Text/FontConfig.hpp"
#include "Nezumi/Render/Core/Text/FontGlyph.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span

namespace Nezumi::Render {

class MaterialTable;

class Font {
public:
    // -------------------------------------------------------------------------
    // Statics
    // -------------------------------------------------------------------------

    static constexpr char FIRST_CHAR = 32;  // ' '
    static constexpr char LAST_CHAR  = 126; // '~'
    static constexpr int  CHAR_COUNT = LAST_CHAR - FIRST_CHAR + 1;

    // -------------------------------------------------------------------------
    // Construction & Destruction
    //
    // Both throw std::runtime_error if the font can't be read, has no glyphs
    // in the ASCII range, or the GPU upload fails.
    // -------------------------------------------------------------------------

    // Loads config.filepath.
    Font(RenderContext& context, const FontConfig& config);

    // From font file bytes already in memory (embedded, or from a pak file);
    // config.filepath is ignored. The bytes are only needed during the call.
    Font(
        RenderContext&           context,
        const FontConfig&          config,
        std::span<const std::byte> data);

    ~Font();

    // Non-copyable and non-movable
    Font(const Font&)            = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&)                 = delete;
    Font& operator=(Font&&)      = delete;

    // -------------------------------------------------------------------------
    // FontGlyphs
    // -------------------------------------------------------------------------

    // nullptr if c is outside 32-126 or the font has no glyph for it.
    const FontGlyph* glyph(char c) const {
        if (c < FIRST_CHAR || c > LAST_CHAR) return nullptr;
        const int i = c - FIRST_CHAR;
        return present_[i] ? &glyphs_[i] : nullptr;
    }

    // glyph(c), or '?' when c is missing (nullptr if '?' is missing too).
    const FontGlyph* glyph_or_fallback(char c) const {
        const FontGlyph* g = glyph(c);
        return g ? g : glyph('?');
    }

    // -------------------------------------------------------------------------
    // Metrics (atlas pixels)
    // -------------------------------------------------------------------------

    float pixel_size()  const { return pixel_size_;  } // em size
    float ascender()    const { return ascender_;    } // baseline -> top of tallest glyphs (> 0)
    float descender()   const { return descender_;   } // baseline -> bottom of descenders (< 0)
    float line_height() const { return line_height_; } // baseline-to-baseline distance
    float sdf_spread()  const { return sdf_spread_;  }

    // -------------------------------------------------------------------------
    // Atlas
    // -------------------------------------------------------------------------

    MaterialID         material()     const { return material_;     }
    RHI::TextureHandle texture()      const { return texture_;      }
    RHI::SamplerHandle sampler()      const { return sampler_;      }
    std::uint32_t      atlas_width()  const { return atlas_width_;  }
    std::uint32_t      atlas_height() const { return atlas_height_; }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    // TODO: Font creation is slow (~1 sec per font).  Might be worth
    //       investigating caching the font atlas in assets instead
    void create_(const FontConfig& config, std::span<const std::byte> data);
    void remove_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RenderContext*   context_  = nullptr;
    MaterialID         material_ = Material::None;
    RHI::TextureHandle texture_  = {};
    RHI::SamplerHandle sampler_  = {};

    std::uint32_t atlas_width_  = 0;
    std::uint32_t atlas_height_ = 0;

    float pixel_size_  = 0.0f;
    float ascender_    = 0.0f;
    float descender_   = 0.0f;
    float line_height_ = 0.0f;
    float sdf_spread_  = 0.0f;

    std::array<FontGlyph, CHAR_COUNT> glyphs_  = {};
    std::array<bool,      CHAR_COUNT> present_ = {};
};

} // namespace Nezumi::Render
