// =============================================================================
// ArrowKind.hpp
//
// Kind for arrow instances.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kinds/Arrow.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct ArrowKind {
    using Instance = Arrow;

    static constexpr std::string_view NAME        = "arrow";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/arrow.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/arrow.frag.glsl";

    // Unit arrow as three lines (LineList).
    //
    //        v2 -
    //            -
    // v0 ----p----> v1
    //            -
    //        v3 -
    static constexpr std::array<float, 12> VERTICES = {
        //   x,      y,
        -0.50f,  0.00f, // v0 shaft base
         0.50f,  0.00f, // v1 shaft tip
         0.00f,  0.25f, // v2 left  barb corner
         0.50f,  0.00f, // v1 left  barb tip
         0.00f, -0.25f, // v3 right barb corner
         0.50f,  0.00f, // v1 right barb tip
    };

    static constexpr std::uint32_t VERTEX_COUNT = 6;

    static std::span<const std::byte> shape_vertices() {
        return std::as_bytes(std::span(VERTICES));
    }

    static RHI::RenderPipelineHandle make_pipeline(const RendererContext& ctx);
};

static_assert(KindConcept<ArrowKind>);

} // namespace Nezumi::Render
