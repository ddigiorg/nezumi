// =============================================================================
// RectangleUnfilledKind.cpp
// =============================================================================

#include "Nezumi/Render/2D/Kinds/RectangleUnfilledKind.hpp"

#include "Nezumi/Render/Core/Shader/ShaderLibrary.hpp"
#include "Nezumi/Render/Core/View/ViewBindings.hpp"
#include "Nezumi/RHI/Device.hpp"
#include <cstddef> // offsetof

namespace Nezumi::Render {

RHI::RenderPipelineHandle RectangleUnfilledKind::make_pipeline(const RenderContext& ctx) {

    // Setup vertex shader
    const RHI::ShaderHandle vert_shader = ctx.shaders->get(
        /*.path   = */VERT_SHADER,
        /*.stage  = */RHI::ShaderStageFlags::Vertex,
        /*.format = */RHI::ShaderFormat::Glsl
    );
    if (!vert_shader) return {};

    // Setup fragment shader
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

    // Setup instance attributes: one Rectangle per instance
    const std::array<RHI::VertexAttribute, 2> instance_attributes = {
        RHI::VertexAttribute{
            .location = 1,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(Rectangle, pos_box),
        },
        RHI::VertexAttribute{
            .location = 2,
            .format   = RHI::VertexFormat::Float32x4,
            .offset   = offsetof(Rectangle, color),
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
            .stride     = sizeof(Rectangle),
            .step_mode  = RHI::VertexStepMode::Instance,
            .attributes = instance_attributes,
        },
    };

    // Setup bind group layouts
    const std::array<RHI::BindGroupLayoutHandle, 1> bind_group_layouts = {
        ctx.views->layout(),
    };

    // Setup primitive
    const RHI::PrimitiveState primitive = {
        .topology = RHI::PrimitiveTopology::LineStrip,
    };

    // Setup color targets
    const std::array<RHI::ColorTargetState, 1> color_targets = {
        RHI::ColorTargetState{
            .format = ctx.device->backbuffer_format(),
            .blend  = RHI::BlendState::ALPHA_BLEND(),
        },
    };

    // Create render pipeline
    RHI::RenderPipelineHandle pipeline = ctx.device->create(
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

    return pipeline;
}

} // namespace Nezumi::Render
