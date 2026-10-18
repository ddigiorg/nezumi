// =============================================================================
// GLDevice.hpp
//
// The OpenGL implementation of RHI::Device.
//
// FRAME FLOW
//   begin_frame  - resize the backbuffer if the window changed; reset encoder
//   (recording)  - the caller records commands, writes data, destroys things
//   end_frame    - apply the frame's queued writes, replay the recording as
//                  GL calls, present, then delete what was destroyed
//
// UPLOADS
//   Outside a frame, write_buffer / write_texture go straight to the GPU.
//   Inside a frame, the data is copied into a staging buffer and applied just
//   before replay, so every write lands before any of the frame's commands
//   (the rule in Device.hpp).
//
// DESTRUCTION
//   Outside a frame, destroy() deletes immediately; the driver itself keeps
//   GL objects alive until the GPU is done with them. Inside a frame, the
//   resource is marked pending: its handle is rejected by every public call
//   from then on, but commands recorded earlier in the frame can still use
//   it during replay. It's deleted after the replay.
//
// The backbuffer is an ordinary texture in the pool, so render passes treat
// it like any other attachment. The window's own framebuffer only receives
// the final copy (see GLContext's prepare_window).
// =============================================================================

#pragma once
#include "Nezumi/RHI/Backends/OpenGL/GLBindGroup.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLBindGroupLayout.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLBuffer.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLCommandEncoder.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLContext.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLFramebufferCache.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLInfo.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLRenderPipeline.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLSampler.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLShader.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLStateCache.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLTexture.hpp"
#include "Nezumi/RHI/Device.hpp"
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace Nezumi::RHI::GL {

class GLDevice final : public Device {
public:
    // Throws std::runtime_error on failure; create_device() catches it.
    explicit GLDevice(
        SDL_Window* window,
        Logger* Logger,
        const DeviceConfig& config
    );


    ~GLDevice() override;

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    BackendType backend() const noexcept override {
        return BackendType::OpenGL;
    }

    const DeviceInfo& info() const noexcept override { return info_; }

    // -------------------------------------------------------------------------
    // Creation
    // -------------------------------------------------------------------------

    BufferHandle          create(const BufferConfig&          config) override;
    TextureHandle         create(const TextureConfig&         config) override;
    SamplerHandle         create(const SamplerConfig&         config) override;
    ShaderHandle          create(const ShaderConfig&          config) override;
    BindGroupLayoutHandle create(const BindGroupLayoutConfig& config) override;
    BindGroupHandle       create(const BindGroupConfig&       config) override;
    RenderPipelineHandle  create(const RenderPipelineConfig&  config) override;

    // -------------------------------------------------------------------------
    // Destruction
    // -------------------------------------------------------------------------

    void remove(BufferHandle          buffer)   override;
    void remove(TextureHandle         texture)  override;
    void remove(SamplerHandle         sampler)  override;
    void remove(ShaderHandle          shader)   override;
    void remove(BindGroupLayoutHandle layout)   override;
    void remove(BindGroupHandle       group)    override;
    void remove(RenderPipelineHandle  pipeline) override;

    // -------------------------------------------------------------------------
    // Uploads
    // -------------------------------------------------------------------------

    void write_buffer(
        BufferHandle handle,
        std::uint64_t offset,
        std::span<const std::byte> data
    ) override;

    void write_texture(
        TextureHandle handle,
        const TextureRegion& region,
        std::span<const std::byte> data
    ) override;

    // -------------------------------------------------------------------------
    // Device: frame loop
    // -------------------------------------------------------------------------

    CommandEncoder& begin_frame() override;

    void end_frame() override;

    TextureHandle backbuffer()        const          override { return backbuffer_;        }
    Format        backbuffer_format() const noexcept override { return BACKBUFFER_FORMAT;  }
    Extent2D      backbuffer_extent() const noexcept override { return backbuffer_extent_; }

    void resize(Extent2D extent) override;
    void set_vsync(bool enabled) override;
    void wait_idle() override;

    std::uint64_t frame_index()      const noexcept override { return frame_index_; }
    std::uint32_t frames_in_flight() const noexcept override { return frames_in_flight_; }

    // -------------------------------------------------------------------------
    // Backend-internal (used by GLCommandEncoder during replay). These pools
    // include resources pending destruction, which replay may still use.
    // -------------------------------------------------------------------------

    bool debug() const noexcept { return debug_; }

    const GLBindingLimits& binding_limits() const noexcept { return binding_limits_; }
    const GLBindGroupPool& bind_groups()    const noexcept { return bind_groups_;    }
    const GLBufferPool&    buffers()        const noexcept { return buffers_;        }
    const GLPipelinePool&  pipelines()      const noexcept { return pipelines_;      }
    const GLSamplerPool&   samplers()       const noexcept { return samplers_;       }
    const GLShaderPool&    shaders()        const noexcept { return shaders_;        }
    const GLTexturePool&   textures()       const noexcept { return textures_;       }

    GLFramebufferCache& framebuffers() noexcept { return framebuffers_; }
    GLStateCache&       state_cache()  noexcept { return state_cache_; }

private:
    static constexpr Format BACKBUFFER_FORMAT = Format::RGBA8Unorm;

    // A write made during a frame, applied just before replay.
    struct PendingBufferWrite {
        BufferHandle  buffer      = {};
        std::uint64_t offset      = 0;
        std::size_t   data_offset = 0;   // into staging_
        std::size_t   size        = 0;
    };

    struct PendingTextureWrite {
        TextureHandle texture     = {};
        TextureRegion region      = {};  // already validated and resolved
        std::size_t   data_offset = 0;   // into staging_
        std::size_t   size        = 0;
    };

    // A resource removed during a frame, deleted after replay.
    using PendingRemove = std::variant<
        BufferHandle, TextureHandle, SamplerHandle,
        ShaderHandle, RenderPipelineHandle,
        BindGroupLayoutHandle, BindGroupHandle
    >;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    // The resource, or nullptr if the handle is stale or pending destruction.
    template <typename T, typename Tag>
    [[nodiscard]] T* live_(SlotMap<T, Tag>& pool, Handle<Tag> handle);

    // Shared logic for the public remove() overloads.
    template <typename T, typename Tag>
    void remove_(SlotMap<T, Tag>& pool, Handle<Tag> handle, const char* what);

    void remove_now_(BufferHandle          handle);
    void remove_now_(TextureHandle         handle);
    void remove_now_(SamplerHandle         handle);
    void remove_now_(ShaderHandle          handle);
    void remove_now_(RenderPipelineHandle  handle);
    void remove_now_(BindGroupLayoutHandle handle);
    void remove_now_(BindGroupHandle       handle);
    void flush_pending_removes_();

    // Validates `in` against the texture, resolves "0 = to the edge" sizes,
    // and returns the bytes of `data` the upload reads (0 = invalid, logged).
    std::size_t resolve_region_(
        const GLTexture&           texture,
        const TextureRegion&       in,
        std::span<const std::byte> data,
        TextureRegion&             out
    ) const;

    void upload_texture_(
        const GLTexture& texture,
        const TextureRegion& region,
        const std::byte* data
    ) const;

    void apply_pending_writes_();

    void recreate_backbuffer_(Extent2D extent);
    void present_();
    void report_leaks_();
    void not_implemented_(const char* what);

    // -------------------------------------------------------------------------
    // Members
    //
    // Order matters: members are destroyed in reverse, and everything below
    // context_ makes GL calls when destroyed, so context_ comes first.
    // -------------------------------------------------------------------------

    Logger*       logger_;
    bool          debug_;
    std::uint32_t frames_in_flight_;

    GLContext             context_;
    DeviceInfo            info_;
    GLBindingLimits       binding_limits_;
    GLBindGroupLayoutPool bind_group_layouts_;
    GLBindGroupPool       bind_groups_;
    GLBufferPool          buffers_;
    GLPipelinePool        pipelines_;
    GLSamplerPool         samplers_;
    GLShaderPool          shaders_;
    GLTexturePool         textures_;
    GLFramebufferCache    framebuffers_;
    GLStateCache          state_cache_;
    GLCommandEncoder      encoder_;

    // Deferred work for the current frame
    std::vector<std::byte>           staging_;
    std::vector<PendingBufferWrite>  buffer_writes_;
    std::vector<PendingTextureWrite> texture_writes_;
    std::vector<PendingRemove>       pending_removes_;

    TextureHandle backbuffer_        = {};
    Extent2D      backbuffer_extent_ = {};
    std::uint64_t frame_index_       = 0;
    bool          in_frame_          = false;
};

} // namespace Nezumi::RHI::GL
