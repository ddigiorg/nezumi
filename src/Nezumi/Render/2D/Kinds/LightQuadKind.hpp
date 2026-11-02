// =============================================================================
// LightQuadKind.hpp
//
// Kind for light "cookies": a textured Quad (4 free corners) whose texture is
// the light's shape (a lantern cone, a window, a rune), added into the
// target with its alpha as the mask (BlendState::ADD: the quad shader
// outputs premultiplied color, so the mask is already applied). Same instance
// and shaders as QuadKind; tint is the light's linear color and may exceed 1.
//
//   renderer.draw_light_quad(Quad{ .corners = cone, .tex_box = cookie,
//                                  .tint = { 2.0f, 1.4f, 0.8f, 1.0f } },
//                            { .view = View::Lights, .material = atlas->material() });
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindConcept.hpp"
#include "Nezumi/Render/2D/Kinds/Quad.hpp"
#include "Nezumi/Render/2D/Kinds/QuadKind.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::Render {

struct LightQuadKind {
    using Instance = Quad;

    static constexpr std::string_view NAME = "light-quad";

    static constexpr std::uint32_t VERTEX_COUNT = QuadKind::VERTEX_COUNT;

    static std::span<const std::byte> shape_vertices() {
        return QuadKind::shape_vertices();
    }

    static RHI::RenderPipelineHandle make_pipeline(const RenderContext& ctx, RHI::Format format) {
        return QuadKind::make_pipeline_with(ctx, format, RHI::BlendState::ADD(), NAME);
    }
};

static_assert(KindConcept<LightQuadKind>);

} // namespace Nezumi::Render
