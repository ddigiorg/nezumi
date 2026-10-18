// =============================================================================
// LineKind.hpp
//
// Kind for Line instances.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kinds/Line.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct LineKind {
    using Instance = Line;

    static constexpr std::string_view NAME        = "line";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/line.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/line.frag.glsl";

    // Unit line is a quad as two triangles: x runs along the line, y across it.
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

static_assert(KindConcept<LineKind>);

} // namespace Nezumi::Render
