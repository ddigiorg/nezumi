// =============================================================================
// RenderPipelineConfig.hpp
//
// Config of a RenderPipeline.
//
// WebGPU: GPURenderPipelineDescriptor
// Vulkan: VkGraphicsPipelineCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include "Nezumi/RHI/Pipeline/ColorTargetState.hpp"
#include "Nezumi/RHI/Pipeline/DepthStencilState.hpp"
#include "Nezumi/RHI/Pipeline/PrimitiveState.hpp"
#include "Nezumi/RHI/Pipeline/VertexBufferLayout.hpp"
#include "Nezumi/RHI/Shader/ShaderHandle.hpp"
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct RenderPipelineConfig {
    std::string_view name            = "";
    ShaderHandle     vertex_shader   = {};
    ShaderHandle     fragment_shader = {};

    std::span<const VertexBufferLayout>    vertex_buffers     = {};
    std::span<const BindGroupLayoutHandle> bind_group_layouts = {}; // index = group

    PrimitiveState                    primitive     = {};
    DepthStencilState                 depth_stencil = {};
    std::span<const ColorTargetState> color_targets = {};
};

} // namespace Nezumi::RHI
