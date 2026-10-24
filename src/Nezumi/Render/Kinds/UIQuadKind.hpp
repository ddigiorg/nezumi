// =============================================================================
// UIQuadKind.hpp
//
// Item kind for UIQuad: boxes, text glyphs and icons for the UI in one
// pipeline. Textured (bind group 1 is the UI font's material), so every
// UIQuad must be drawn with that material, boxes and icons included; they
// just don't sample it. See UIQuad.hpp for why it is one kind.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kinds/UIQuad.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct UIQuadKind {
    using Instance = UIQuad;

    static constexpr std::string_view NAME        = "ui-quad";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/ui-quad.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/ui-quad.frag.glsl";

    // Unit quad as two triangles (TriangleList).
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

    static RHI::RenderPipelineHandle make_pipeline(const RendererContext& ctx);
};

static_assert(KindConcept<UIQuadKind>);

} // namespace Nezumi::Render
