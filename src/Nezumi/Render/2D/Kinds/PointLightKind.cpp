// =============================================================================
// PointLightKind.cpp
// =============================================================================

#include "Nezumi/Render/2D/Kinds/PointLightKind.hpp"

#include "Nezumi/Render/Core/Shader/ShaderLibrary.hpp"
#include "Nezumi/Render/Core/View/ViewBindings.hpp"
#include "Nezumi/RHI/Device.hpp"
#include <cstddef> // offsetof

namespace Nezumi::Render {

RHI::RenderPipelineHandle PointLightKind::make_pipeline(const RenderContext& ctx, RHI::Format format) {

    // Setup shaders
    const RHI::ShaderHandle vert_shader = ctx.shaders->get(
        /*.path   = */VERT_SHADER,
        /*.stage  = */RHI::ShaderStageFlags::Vertex,
        /*.format = */RHI::ShaderFormat::Glsl
    );
    if (!vert_shader) return {};

    const RHI::ShaderHandle frag_shader = ctx.shaders->get(
        /*.path   = */FRAG_SHADER,
        /*.stage  = */RHI::ShaderStageFlags::Fragment,
        /*.format = */RHI::ShaderFormat::Glsl
    );
    if (!frag_shader) return {};

    // Setup shape attributes: unit quad
    const std::array<RHI::VertexAttribute, 1> shape_attributes = {
        RHI::VertexAttribute{
            .location = 0,
            .format   = RHI::VertexFormat::Float32x2,
            .offset   = 0
        },
    };

    // Setup instance attributes: one PointLight per instance. The 4 corners
    // go in as two vec4s (corners 0-1 and 2-3).
    const std::array<RHI::VertexAttribute, 4> instance_attributes = {
        RHI::VertexAttribute{
            .location = 1,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(PointLight, corners),
        },
        RHI::VertexAttribute{
            .location = 2,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(PointLight, corners) + 2 * sizeof(glm::vec2),
        },
        RHI::VertexAttribute{
            .location = 3,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(PointLight, color),
        },
        RHI::VertexAttribute{
            .location = 4,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(PointLight, falloff),
        },
    };

    // Setup vertex buffers
    const std::array<RHI::VertexBufferLayout, 2> vertex_buffers = {
        RHI::VertexBufferLayout{
            .stride     = 2 * sizeof(float),
            .step_mode  = RHI::VertexStepMode::Vertex,
            .attributes = shape_attributes,
        },
        RHI::VertexBufferLayout{
            .stride     = sizeof(PointLight),
            .step_mode  = RHI::VertexStepMode::Instance,
            .attributes = instance_attributes,
        },
    };

    // Setup bind group layouts: views only, no material
    const std::array<RHI::BindGroupLayoutHandle, 1> bind_group_layouts = {
        ctx.views->layout(),
    };

    // Setup primitive
    const RHI::PrimitiveState primitive = {
        .topology = RHI::PrimitiveTopology::TriangleList
    };

    // Setup color targets: lights add up
    const std::array<RHI::ColorTargetState, 1> color_targets = {
        RHI::ColorTargetState{
            .format = format,
            .blend  = RHI::BlendState::ADD(),
        },
    };

    // Create render pipeline
    return ctx.device->create(
        RHI::RenderPipelineConfig{
            .name               = NAME,
            .vertex_shader      = vert_shader,
            .fragment_shader    = frag_shader,
            .vertex_buffers     = vertex_buffers,
            .bind_group_layouts = bind_group_layouts,
            .primitive          = primitive,
            .color_targets      = color_targets,
        }
    );
}

} // namespace Nezumi::Render
