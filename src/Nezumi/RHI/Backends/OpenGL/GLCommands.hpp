// =============================================================================
// GLCommands.hpp
//
// Set of commands that get recorded during a render pass.
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupHandle.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Buffer/IndexFormat.hpp"
#include "Nezumi/RHI/Common/Color.hpp"
#include "Nezumi/RHI/Common/Rect2D.hpp"
#include "Nezumi/RHI/Common/Viewport.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include "Nezumi/RHI/RenderPass/DepthStencilAttachment.hpp"
#include <cstdint>
#include <variant>

namespace Nezumi::RHI::GL::cmd {

// A string stored in the encoder's string buffer.
struct StringRef {
    std::uint32_t offset = 0;
    std::uint32_t length = 0;
};

struct BeginRenderPass {
    std::uint32_t          first_color   = 0;    // index into the color array
    std::uint32_t          color_count   = 0;
    DepthStencilAttachment depth_stencil = {};
    StringRef              name          = {};
};

struct EndRenderPass {};

struct SetRenderPipeline {
    RenderPipelineHandle pipeline = {};
};

struct SetBindGroup {
    std::uint32_t   group        = 0;
    BindGroupHandle bind_group   = {};
    std::uint32_t   first_offset = 0; // index into the offset array
    std::uint32_t   offset_count = 0;
};

struct SetVertexBuffer {
    std::uint32_t slot   = 0;
    BufferHandle  buffer = {};
    std::uint64_t offset = 0;
};

struct SetIndexBuffer {
    BufferHandle  buffer = {};
    IndexFormat   format = IndexFormat::Uint16;
    std::uint64_t offset = 0;
};

struct SetViewport        { Viewport      viewport = {}; };
struct SetScissor         { Rect2D        rect     = {}; };
struct SetStencilReference{ std::uint32_t value    = 0;  };
struct SetBlendConstant   { Color         color    = {}; };

struct Draw {
    std::uint32_t vertex_count   = 0;
    std::uint32_t instance_count = 0;
    std::uint32_t first_vertex   = 0;
    std::uint32_t first_instance = 0;
};

struct DrawIndexed {
    std::uint32_t index_count    = 0;
    std::uint32_t instance_count = 0;
    std::uint32_t first_index    = 0;
    std::int32_t  vertex_offset  = 0;
    std::uint32_t first_instance = 0;
};

struct PushDebugGroup { StringRef label = {}; };
struct PopDebugGroup  {};

using Command = std::variant<
    BeginRenderPass,
    EndRenderPass,
    SetRenderPipeline,
    SetBindGroup,
    SetVertexBuffer,
    SetIndexBuffer,
    SetViewport,
    SetScissor,
    SetStencilReference,
    SetBlendConstant,
    Draw,
    DrawIndexed,
    PushDebugGroup,
    PopDebugGroup
>;

} // namespace Nezumi::RHI::GL::cmd
