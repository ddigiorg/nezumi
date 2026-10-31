// =============================================================================
// TextKind.hpp
//
// Item kind for SDF text glyphs. Each instance is a unit quad stretched over
// the glyph's box, sampling the font's R8 distance-field atlas (bind group
// 1, the font's material). The fragment shader turns distance into coverage
// with a one-pixel antialiased edge at any zoom, and composites the fill
// over an optional outline / drop shadow.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindConcept.hpp"
#include "Nezumi/Render/2D/Kinds/TextGlyphSDF.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct TextGlyphSDFKind {
    using Instance = TextGlyphSDF;

    static constexpr std::string_view NAME        = "text-glyph-sdf";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/text-glyph-sdf.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/text-glyph-sdf.frag.glsl";

    // Unit quad as two triangles; the shader derives UVs from the corner.
    //
    // v4 +----------+ v2
    //    |          | v3
    //    |          |
    // v0 |          |
    // v5 +----------+ v1
    static constexpr std::array<float, 12> VERTICES = {
        // x,    y,
        0.0f, 0.0f, // v0 triangle 1 bottom-left
        1.0f, 0.0f, // v1 triangle 1 bottom-right
        1.0f, 1.0f, // v2 triangle 1 top-right
        1.0f, 1.0f, // v3 triangle 2 top-right
        0.0f, 1.0f, // v4 triangle 2 top-left
        0.0f, 0.0f, // v5 triangle 2 bottom-left
    };

    static constexpr std::uint32_t VERTEX_COUNT = 6;

    static std::span<const std::byte> shape_vertices() {
        return std::as_bytes(std::span(VERTICES));
    }

    static RHI::RenderPipelineHandle make_pipeline(const RenderContext& ctx);
};

static_assert(KindConcept<TextGlyphSDFKind>);

} // namespace Nezumi::Render
