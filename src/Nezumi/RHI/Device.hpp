// =============================================================================
// Device.hpp
//
// The abstract connector to the rendering hardware. Creates and removes
// resources, uploads data, and runs the frame loop.
//
// Coordinate Conventions
//
// - Clip space is y-up (as in OpenGL)
// - Depth range is 0..1 unless `DeviceCaps::clip_depth_zero_to_one` otherwise.
// - Viewport and scissor origins are the top-left of the render target.
//
// WebGPU: GPUDevice
// Vulkan: VkDevice
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupHandle.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupConfig.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutConfig.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include "Nezumi/RHI/Buffer/BufferConfig.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Common/Extent2D.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineConfig.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include "Nezumi/RHI/Sampler/SamplerConfig.hpp"
#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include "Nezumi/RHI/Shader/ShaderConfig.hpp"
#include "Nezumi/RHI/Shader/ShaderHandle.hpp"
#include "Nezumi/RHI/Texture/TextureConfig.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include "Nezumi/RHI/Texture/TextureRegion.hpp"
#include "Nezumi/RHI/BackendType.hpp"
#include "Nezumi/RHI/CommandEncoder.hpp"
#include "Nezumi/RHI/DeviceConfig.hpp"
#include "Nezumi/RHI/DeviceInfo.hpp"
#include "Nezumi/RHI/Window.hpp"
#include <cstddef> // byte
#include <cstdint> // uint32_t, uint64_t
#include <memory>  // unique_ptr
#include <span>    // span
class  Logger;
struct SDL_Window;

namespace Nezumi::RHI {

class Device {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    virtual ~Device() = default;

    // Non-copyable
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    virtual BackendType backend() const noexcept = 0;
    virtual const DeviceInfo& info() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Create
    //
    // On engine init only; not intended to be called every frame.
    //
    // Returns a null handle on failure and logs why. With debug = true,
    // invalid configs (see the zero rule in the configs) assert instead.
    // -------------------------------------------------------------------------

    virtual BindGroupHandle       create(const BindGroupConfig&       config) = 0;
    virtual BindGroupLayoutHandle create(const BindGroupLayoutConfig& config) = 0;
    virtual BufferHandle          create(const BufferConfig&          config) = 0;
    virtual RenderPipelineHandle  create(const RenderPipelineConfig&  config) = 0;
    virtual SamplerHandle         create(const SamplerConfig&         config) = 0;
    virtual ShaderHandle          create(const ShaderConfig&          config) = 0;
    virtual TextureHandle         create(const TextureConfig&         config) = 0;

    // -------------------------------------------------------------------------
    // Remove
    //
    // Destruction is deferred: the GPU may still be using the resource for up
    // to frames_in_flight frames, so it's released later. The handle is invalid
    // immediately (its generation no longer matches). Destroying a null handle
    // is a no-op.
    // -------------------------------------------------------------------------

    virtual void remove(BindGroupHandle       group   ) = 0;
    virtual void remove(BindGroupLayoutHandle layout  ) = 0;
    virtual void remove(BufferHandle          buffer  ) = 0;
    virtual void remove(RenderPipelineHandle  pipeline) = 0;
    virtual void remove(SamplerHandle         sampler ) = 0;
    virtual void remove(ShaderHandle          shader  ) = 0;
    virtual void remove(TextureHandle         texture ) = 0;

    // -------------------------------------------------------------------------
    // Uploads (write_buffer / write_texture)
    //
    // Safe to call at any time, including mid-frame and on a buffer the GPU is
    // still reading from an earlier frame; the backend handles the hazard.
    //
    // All writes made during a frame land before ANY of that frame's commands
    // run. So writing the same range twice in one frame, expecting draws in
    // between to see different data, is a bug: every draw sees the last write.
    // (OpenGL happens to order writes between draws, so this bug is invisible
    // there and only appears on Vulkan / D3D12.) To give different draws
    // different data, write to different offsets.
    // -------------------------------------------------------------------------

    // Buffer must have BufferUsage::CopyDst. offset + data.size() <= size.
    virtual void write_buffer(
        BufferHandle buffer,
        std::uint64_t offset,
        std::span<const std::byte> data
    ) = 0;

    // Texture must have TextureUsage::CopyDst.
    virtual void write_texture(
        TextureHandle texture,
        const TextureRegion& region,
        std::span<const std::byte> data
    ) = 0;

    // -------------------------------------------------------------------------
    // Frame loop
    //
    //   auto& enc = device.begin_frame();   // may block on an old frame
    //   ... record ...
    //   device.end_frame();                 // submit + present
    // -------------------------------------------------------------------------

    virtual CommandEncoder& begin_frame() = 0;
    virtual void end_frame() = 0;

    // The swapchain image for the current frame. Only valid between
    // begin_frame() and end_frame(); use it as a ColorAttachment.
    virtual TextureHandle backbuffer() const = 0;

    // Available any time, so pipelines that draw to the backbuffer can be
    // created at load time, before the first frame.
    virtual Format   backbuffer_format() const noexcept = 0;
    virtual Extent2D backbuffer_extent() const noexcept = 0;

    // Call when the window size changes (SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED).
    // Takes effect at the next begin_frame(). A zero extent (minimized) is
    // allowed; begin_frame() then skips presenting until it's nonzero.
    virtual void resize(Extent2D extent) = 0;

    virtual void set_vsync(bool enabled) = 0;

    // Blocks until the GPU has finished all submitted work, then releases all
    // pending destroys. For shutdown and major reloads, not per-frame use.
    virtual void wait_idle() = 0;

    // Increments once per end_frame(). Useful for per-frame ring buffers.
    virtual std::uint64_t frame_index()      const noexcept = 0;
    virtual std::uint32_t frames_in_flight() const noexcept = 0;

protected:
    Device() = default;
};

// -----------------------------------------------------------------------------
// Make Device Factory
// -----------------------------------------------------------------------------

std::unique_ptr<Device> make_device(
    SDL_Window* window,
    Logger* logger,
    const DeviceConfig& config
);

} // namespace Nezumi::RHI
