// =============================================================================
// RectangleUnfilledKind.hpp
//
// Kind for unfilled (i.e. stroked or wireframe) Rectangle instances.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kinds/Rectangle.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct RectangleUnfilledKind {
    using Instance = Rectangle;

    static constexpr std::string_view NAME        = "rectangle-lined";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/rectangle.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/rectangle.frag.glsl";

    // Unit quad as four closed corners (LineStrip).
    //
    // v3 +----------+ v2
    //    |          |
    //    |          |
    // v4 |          |
    // v0 +----------+ v1
    static constexpr std::array<float, 10> VERTICES = {
        // x,    y,
        0.0f, 0.0f, // bottom-left
        1.0f, 0.0f, // bottom-right
        1.0f, 1.0f, // top-right
        0.0f, 1.0f, // top-left
        0.0f, 0.0f, // back to the start
    };

    static constexpr std::uint32_t VERTEX_COUNT = 6;

    static std::span<const std::byte> shape_vertices() {
        return std::as_bytes(std::span(VERTICES));
    }

    static RHI::RenderPipelineHandle make_pipeline(const RendererContext& ctx);
};

static_assert(KindConcept<RectangleUnfilledKind>);

} // namespace Nezumi::Render
