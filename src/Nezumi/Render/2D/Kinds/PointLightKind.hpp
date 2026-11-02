// =============================================================================
// PointLightKind.hpp
//
// Kind for PointLight instances: a quad with 4 free corners, shaded with a
// procedural falloff, added (BlendState::ADD) into the target.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindConcept.hpp"
#include "Nezumi/Render/2D/Kinds/PointLight.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include <array>       // array
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct PointLightKind {
    using Instance = PointLight;

    static constexpr std::string_view NAME        = "point-light";
    static constexpr std::string_view VERT_SHADER = "assets/shaders/point-light.vert.glsl";
    static constexpr std::string_view FRAG_SHADER = "assets/shaders/point-light.frag.glsl";

    // Unit quad as two triangles (same as QuadKind)
    static constexpr std::array<float, 12> VERTICES = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    static constexpr std::uint32_t VERTEX_COUNT = 6;

    static std::span<const std::byte> shape_vertices() {
        return std::as_bytes(std::span(VERTICES));
    }

    static RHI::RenderPipelineHandle make_pipeline(const RenderContext& ctx, RHI::Format format);
};

static_assert(KindConcept<PointLightKind>);

} // namespace Nezumi::Render
