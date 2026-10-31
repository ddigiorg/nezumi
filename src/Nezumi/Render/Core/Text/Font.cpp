// =============================================================================
// Font.cpp
//
// Needs FreeType 2.11+ (for FT_RENDER_MODE_SDF).
// =============================================================================

#include "Nezumi/Render/Core/Text/Font.hpp"

#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/Core/ReadFile.hpp"
#include "Nezumi/Render/Core/Material/MaterialTable.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

#include <algorithm> // max, sort
#include <cmath>     // ceil, sqrt
#include <cstring>   // memcpy
#include <stdexcept> // runtime_error
#include <string>    // string, to_string
#include <utility>   // exchange, move
#include <vector>    // vector

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

namespace {

// Empty pixels between glyphs in the atlas. The SDF already fades to 0 at
// each bitmap's border, so one pixel keeps linear filtering from ever
// blending two glyphs.
constexpr std::uint32_t PADDING = 1;

// FreeType objects, released however the build exits.
struct FreeType {
    FT_Library library = nullptr;
    FT_Face    face    = nullptr;
    ~FreeType() {
        if (face)    FT_Done_Face(face);
        if (library) FT_Done_FreeType(library);
    }
};

// One rendered glyph waiting to be packed.
struct Bitmap {
    int                       index = 0; // c - FIRST_CHAR
    std::uint32_t             width = 0, height = 0;
    std::vector<std::uint8_t> pixels;    // tightly packed rows, top to bottom
    std::uint32_t             x = 0, y = 0; // atlas position, set by packing
};

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Font::Font(RenderContext& context, const FontConfig& config)
    : Font(
        /*.context   = */context,
        /*.config    = */config,
        /*.file_data = */read_file(BasePath::get(config.filepath).string()))
{}

Font::Font(
    RenderContext&           context,
    const FontConfig&          config,
    std::span<const std::byte> data)
        : context_(&context)
{
    create_(config, data);
}

Font::~Font() {
    remove_();
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void Font::create_(const FontConfig& config, std::span<const std::byte> data) {
    const std::string name = std::string(config.name);

    // -------------------------------------------------------------------------
    // Validate config
    // -------------------------------------------------------------------------

    if (config.pixel_size == 0) {
        throw std::runtime_error("Font: '" + name + "': pixel_size is 0");
    }

    if (config.sdf_spread < 2 || config.sdf_spread > 32) {
        throw std::runtime_error("Font: '" + name + "': sdf_spread must be 2-32");
    }

    if (data.empty()) {
        throw std::runtime_error("Font: '" + name + "': font data is empty");
    }

    // -------------------------------------------------------------------------
    // Setup FreeType
    // -------------------------------------------------------------------------

    FreeType ft;
    FT_Error ft_error;

    // Initialize FreeType
    ft_error = FT_Init_FreeType(&ft.library);
    if (ft_error) {
        throw std::runtime_error(
            "Font: '" + name + "': FreeType failed to initialize"
        );
    }

    // Setup spread properties
    FT_Int spread = static_cast<FT_Int>(config.sdf_spread);
    ft_error = FT_Property_Set(
        /*.library       = */ft.library,
        /*.module_name   = */"sdf",
        /*.property_name = */"spread",
        /*.value         = */&spread
    );
    if (ft_error) {
        throw std::runtime_error(
            "Font: '" + name + "': Failed to initialize sdf spread property"
        );
    }
    ft_error = FT_Property_Set(
        /*.library       = */ft.library,
        /*.module_name   = */"bsdf",
        /*.property_name = */"spread",
        /*.value         = */&spread
    );
    if (ft_error) {
        throw std::runtime_error(
            "Font: '" + name + "': Failed to initialize bsdf spread property"
        );
    }

    // Setup memory face
    ft_error = FT_New_Memory_Face(
        /*.library    = */ft.library,
        /*.file_base  = */reinterpret_cast<const FT_Byte*>(data.data()),
        /*.file_size  = */static_cast<FT_Long>(data.size()),
        /*.face_index = */0,
        /*.aface      = */&ft.face
    );
    if (ft_error)
    {
        throw std::runtime_error(
            "Font: '" + name + "': FreeType can't read the font data"
        );
    }

    // Setup pixel sizes
    ft_error = FT_Set_Pixel_Sizes(
        /*.face         = */ft.face,
        /*.pixel_width  = */0,
        /*.pixel_height = */config.pixel_size
    );
    if (ft_error) {
        throw std::runtime_error(
            "Font: '" + name + "': pixel_size " +
            std::to_string(config.pixel_size)   +
            " isn't available in this font"
        );
    }

    // -------------------------------------------------------------------------
    // Render every glyph
    // -------------------------------------------------------------------------

    std::vector<Bitmap> bitmaps;

    for (int c = FIRST_CHAR; c <= LAST_CHAR; ++c) {

        const int i = c - FIRST_CHAR;
        if (FT_Get_Char_Index(ft.face, FT_ULong(c)) == 0)        continue; // not in this font
        if (FT_Load_Char(ft.face, FT_ULong(c), FT_LOAD_DEFAULT)) continue;

        FT_GlyphSlot slot = ft.face->glyph;
        FontGlyph& g = glyphs_[i];
        g.advance  = static_cast<float>(slot->advance.x) / 64.0f;
        present_[i] = true;

        // Glyphs with no outline (space) only advance the pen.
        const bool has_shape = slot->format != FT_GLYPH_FORMAT_OUTLINE ||
                               slot->outline.n_points > 0;
        if (!has_shape
            || FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL) // fast coverage bitmap (~0.5 ms for all glyphs)
            || FT_Render_Glyph(slot, FT_RENDER_MODE_SDF)    // bitmap -> "bsdf" renderer instead of "sdf"
        ) continue;

        const FT_Bitmap& bmp = slot->bitmap;
        if (bmp.width == 0 || bmp.rows == 0) continue;

        g.size    = { float(bmp.width),         float(bmp.rows)         };
        g.bearing = { float(slot->bitmap_left), float(slot->bitmap_top) };

        Bitmap b;
        b.index  = i;
        b.width  = bmp.width;
        b.height = bmp.rows;
        b.pixels.resize(std::size_t(bmp.width) * bmp.rows);
        for (uint32_t row = 0; row < bmp.rows; ++row) {
            // pitch can be padded (or negative for bottom-up bitmaps)
            const unsigned char* src = bmp.buffer + std::ptrdiff_t(row) * bmp.pitch;
            std::memcpy(b.pixels.data() + std::size_t(row) * bmp.width, src, bmp.width);
        }
        bitmaps.push_back(std::move(b));
    }

    if (bitmaps.empty()) {
        throw std::runtime_error(
            "Font: '" + name + "': No printable ASCII glyphs"
        );
    }

    const FT_Size_Metrics& m = ft.face->size->metrics;
    pixel_size_  = static_cast<float>(config.pixel_size);
    ascender_    = static_cast<float>(m.ascender)  / 64.0f;
    descender_   = static_cast<float>(m.descender) / 64.0f;
    line_height_ = static_cast<float>(m.height)    / 64.0f;
    sdf_spread_  = static_cast<float>(config.sdf_spread);

    // -------------------------------------------------------------------------
    // Pack into rows (tallest first)
    // -------------------------------------------------------------------------

    std::size_t area = 0;
    std::uint32_t widest = 0;
    for (const Bitmap& b : bitmaps) {
        area  += std::size_t(b.width + PADDING) * (b.height + PADDING);
        widest = std::max(widest, b.width + PADDING);
    }
    // Roughly square, with some slack for row waste; rounded up to 4.
    std::uint32_t width = std::max(
        widest,
        std::uint32_t(std::ceil(std::sqrt(double(area) * 1.15)))
    );
    width = (width + 3u) & ~3u;

    std::vector<Bitmap*> order;
    for (Bitmap& b : bitmaps) order.push_back(&b);
    std::stable_sort(
        /*.first = */order.begin(),
        /*.last  = */order.end(),
        /*.comp  = */[](const Bitmap* a, const Bitmap* b) {
            return a->height > b->height;
        }
    );

    std::uint32_t x = PADDING, y = PADDING, row_h = 0;
    for (Bitmap* b : order) {
        if (x + b->width + PADDING > width) { // row full
            x = PADDING;
            y += row_h + PADDING;
            row_h = 0;
        }
        b->x = x;
        b->y = y;
        x += b->width + PADDING;
        row_h = std::max(row_h, b->height);
    }
    const std::uint32_t height = (y + row_h + PADDING + 3u) & ~3u;

    // -------------------------------------------------------------------------
    // Compose the glyph atlas and fill in UVs
    // -------------------------------------------------------------------------

    std::vector<std::uint8_t> atlas(std::size_t(width) * height, 0);
    for (const Bitmap& b : bitmaps) {
        for (std::uint32_t row = 0; row < b.height; ++row) {
            std::memcpy(
                /*.dst = */atlas.data() + std::size_t(b.y + row) * width + b.x,
                /*.src = */b.pixels.data() + std::size_t(row) * b.width,
                /*.n   = */b.width
            );
        }
        glyphs_[b.index].tex_box = {
            /*.min_u = */float(b.x)            / float(width ),
            /*.min_v = */float(b.y)            / float(height),
            /*.max_u = */float(b.x + b.width ) / float(width ),
            /*.max_v = */float(b.y + b.height) / float(height),
        };
    }

    atlas_width_  = width;
    atlas_height_ = height;

    // -------------------------------------------------------------------------
    // Handle GPU objects
    // -------------------------------------------------------------------------

    RHI::Device& device = *context_->device;

    // Setup texture
    //
    // One channel, uploaded at creation, rows top to bottom. One mip level:
    // SDF text stays sharp when minified without mips (see TODO(mipmaps) in
    // SpriteAtlasConfig.hpp if very small text ever shimmers).
    texture_ = device.create(
        RHI::TextureConfig{
            .name         = name,
            .type         = RHI::TextureType::Texture2D,
            .format       = RHI::Format::R8Unorm,
            .width        = width,
            .height       = height,
            .mip_levels   = 1,
            .usage        = RHI::TextureUsageFlags::Sampled,
            .initial_data = std::as_bytes(std::span(atlas)),
        }
    );
    if (!texture_) {
        remove_();
        throw std::runtime_error(
            "Font: '" + name + "': texture creation failed (see log)"
        );
    }

    // Setup sampler
    //
    // Linear filtering is what makes a distance field work: it interpolates
    // distances between texels, so edges stay smooth at any scale.
    sampler_ = device.create(
        RHI::SamplerConfig{
            .name       = name,
            .min_filter = RHI::SamplerFilterMode::Linear,
            .mag_filter = RHI::SamplerFilterMode::Linear,
            .mip_filter = RHI::SamplerFilterMode::Nearest,
            .address_u  = RHI::SamplerAddressMode::ClampToEdge,
            .address_v  = RHI::SamplerAddressMode::ClampToEdge,
            .address_w  = RHI::SamplerAddressMode::ClampToEdge,
        }
    );
    if (!sampler_) {
        remove_();
        throw std::runtime_error(
            "Font: '" + name + "': sampler creation failed (see log)"
        );
    }

    // Setup material in material table
    try {
        material_ = context_->materials->create(name, texture_, sampler_);
    } catch (...) {
        remove_();
        throw;
    }
}

void Font::remove_() {
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
}

} // namespace Nezumi::Render
