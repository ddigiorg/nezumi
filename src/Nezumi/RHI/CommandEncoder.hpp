// =============================================================================
// CommandEncoder.hpp
//
// The interface used to record batches of commands that will eventually be
// executed asynchronously by the GPU.  Records GPU work only for one frame.
//
// Obtained from Device::begin_frame() and valid until Device::end_frame().
//
// Higher-level render code records into an encoder:
//
//   enc.begin_render_pass(pass_config);
//   enc.set_bind_group   (0, frame_group);      // camera, once per pass
//   enc.set_pipeline     (sprite_pipeline);
//   enc.set_bind_group   (1, sheet_group);
//   enc.set_vertex_buffer(0, quad);
//   enc.set_vertex_buffer(1, instances);
//   enc.draw             (4, instance_count);
//   enc.end_render_pass  ();
//
// Rules:
//
// - Everything except begin/end_render_pass and debug groups must be called
//   inside a render pass.
// - begin_render_pass resets all bound state (pipeline, bind groups, vertex and
//   index buffers) and sets the viewport and scissor to cover the whole first
//   attachment.
// - A bind group stays bound across set_pipeline if the new pipeline uses the
//   same BindGroupLayout for that group index AND for every lower index. This
//   is why group 0 (per frame) can be bound once per pass: give every pipeline
//   the same frame layout at group 0.
//
// Default arguments live only on this interface. Backends must not repeat
// them in their overrides (defaults on virtuals bind to the static type).
//
// WebGPU: GPUCommandEncoder
// Vulkan: VkCommandBuffer
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupHandle.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Buffer/IndexFormat.hpp"
#include "Nezumi/RHI/Common/Color.hpp"
#include "Nezumi/RHI/Common/Rect2D.hpp"
#include "Nezumi/RHI/Common/Viewport.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include "Nezumi/RHI/RenderPass/RenderPassConfig.hpp"
#include <cstdint>     // int32_t, uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

class CommandEncoder {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    virtual ~CommandEncoder() = default;

    CommandEncoder(const CommandEncoder&)            = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    // -------------------------------------------------------------------------
    // Render passes
    // -------------------------------------------------------------------------

    virtual void begin_render_pass(const RenderPassConfig& config) = 0;
    virtual void end_render_pass() = 0;

    // -------------------------------------------------------------------------
    // Bound state
    // -------------------------------------------------------------------------

    virtual void set_render_pipeline(RenderPipelineHandle pipeline) = 0;

    // One dynamic offset per has_dynamic_offset entry in the group's layout,
    // in binding order. Each must be a multiple of
    // DeviceCaps::uniform_buffer_offset_alignment.
    virtual void set_bind_group(
        std::uint32_t                  group,
        BindGroupHandle                bind_group,
        std::span<const std::uint32_t> dynamic_offsets = {}
    ) = 0;

    // `slot` is the index into PipelineConfig::vertex_buffers.
    virtual void set_vertex_buffer(
        std::uint32_t slot,
        BufferHandle  buffer,
        std::uint64_t offset = 0
    ) = 0;

    virtual void set_index_buffer(
        BufferHandle  buffer,
        IndexFormat   format,
        std::uint64_t offset = 0
    ) = 0;

    // -------------------------------------------------------------------------
    // Dynamic state
    // -------------------------------------------------------------------------

    virtual void set_viewport(const Viewport& viewport)     = 0;
    virtual void set_scissor(const Rect2D& rect)            = 0;
    virtual void set_stencil_reference(std::uint32_t value) = 0;
    virtual void set_blend_constant(const Color& color)     = 0;

    // -------------------------------------------------------------------------
    // Draws
    //
    // first_instance != 0 requires DeviceCaps::supports_base_instance.
    // -------------------------------------------------------------------------

    virtual void draw(
        std::uint32_t vertex_count,
        std::uint32_t instance_count = 1,
        std::uint32_t first_vertex   = 0,
        std::uint32_t first_instance = 0
    ) = 0;

    virtual void draw_indexed(
        std::uint32_t index_count,
        std::uint32_t instance_count = 1,
        std::uint32_t first_index    = 0,
        std::int32_t  vertex_offset  = 0,
        std::uint32_t first_instance = 0
    ) = 0;

    // -------------------------------------------------------------------------
    // Debug markers
    //
    // Named, nestable regions that show up in RenderDoc / Nsight captures.
    // Wrapping each Layer in one makes captures far easier to read. No-ops
    // unless the device was created with debug = true.
    // -------------------------------------------------------------------------

    virtual void push_debug_group(std::string_view label) = 0;
    virtual void pop_debug_group() = 0;

protected:
    CommandEncoder() = default;
};

} // namespace Nezumi::RHI
