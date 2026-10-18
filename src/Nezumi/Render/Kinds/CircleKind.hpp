// =============================================================================
// CircleKind.hpp
//
// Kind for circle instances.
//
// Each instance is a unit quad that the vertex shader sizes to the circle's
// bounding square; the fragment shader keeps the pixels whose distance from the
// center lies between radius - width and radius, with a one-pixel antialiased
// edge on both sides.
//
// Fill rate: the quad covers the whole bounding square, so a thin ring still
// costs as many fragments as a disk (most are discarded). Fine for UI and
// markers; thousands of large rings would be the case to optimize.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kinds/Circle.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct CircleKind {
    using Instance = Circle;

    static constexpr std::string_view NAME        = "circle";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/circle.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/circle.frag.glsl";

    // Unit quad as two triangles.
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

static_assert(KindConcept<CircleKind>);

} // namespace Nezumi::Render
