// =============================================================================
// GLCommandEncoder.hpp
//
// Records commands during the frame and replays them as OpenGL calls in
// GLDevice::end_frame(). Recording instead of calling GL immediately is what
// lets the GL backend honor the RHI's upload rule (every write in a frame
// lands before any of that frame's commands run; see Device.hpp).
//
// Commands are small structs in a std::variant. Anything variable-length
// (color attachments, dynamic offsets, strings) is copied into side arrays
// and referenced by index, which keeps each command small and honors the
// borrow-only rule for configs: nothing recorded points into caller memory.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Backends/OpenGL/GLCommands.hpp"
#include "Nezumi/RHI/CommandEncoder.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Nezumi {
    class Logger;
}

namespace Nezumi::RHI::GL {

class GLDevice;

class GLCommandEncoder final : public CommandEncoder {
public:
    explicit GLCommandEncoder(Logger& logger);

    // -------------------------------------------------------------------------
    // Recording
    // -------------------------------------------------------------------------

    void begin_render_pass(const RenderPassConfig& config) override;

    void end_render_pass() override;

    void set_render_pipeline(RenderPipelineHandle pipeline) override;

    void set_bind_group(
        std::uint32_t group,
        BindGroupHandle bind_group,
        std::span<const std::uint32_t> dynamic_offsets) override;

    void set_vertex_buffer(
        std::uint32_t slot,
        BufferHandle buffer,
        std::uint64_t offset) override;

    void set_index_buffer(
        BufferHandle buffer,
        IndexFormat format,
        std::uint64_t offset) override;

    void set_viewport(const Viewport& viewport) override;

    void set_scissor(const Rect2D& rect) override;

    void set_stencil_reference(std::uint32_t value) override;

    void set_blend_constant(const Color& color) override;

    void draw(
        std::uint32_t vertex_count,
        std::uint32_t instance_count,
        std::uint32_t first_vertex,
        std::uint32_t first_instance) override;

    void draw_indexed(
        std::uint32_t index_count,
        std::uint32_t instance_count,
        std::uint32_t first_index,
        std::int32_t  vertex_offset,
        std::uint32_t first_instance) override;

    void push_debug_group(std::string_view label) override;

    void pop_debug_group() override;

    // -------------------------------------------------------------------------
    // Backend (called by GLDevice)
    // -------------------------------------------------------------------------

    // Discard everything recorded. Called at begin_frame.
    void reset();

    // Close anything left open (and log it as an error). Called at end_frame.
    void finish();

    // Replay the recorded commands as GL calls. Requires a current context.
    void execute(GLDevice& device);

private:
    struct ReplayState;

    bool require_pass_(const char* function);

    bool prepare_draw_(ReplayState& state, const char* draw, std::uint32_t first_instance);

    cmd::StringRef store_string_(std::string_view text);

    std::string_view view_(cmd::StringRef ref) const;

    void not_implemented_(const char* what);

    void replay_(const cmd::BeginRenderPass&     c, ReplayState& state);
    void replay_(const cmd::EndRenderPass&       c, ReplayState& state);
    void replay_(const cmd::SetRenderPipeline&   c, ReplayState& state);
    void replay_(const cmd::SetBindGroup&        c, ReplayState& state);
    void replay_(const cmd::SetVertexBuffer&     c, ReplayState& state);
    void replay_(const cmd::SetIndexBuffer&      c, ReplayState& state);
    void replay_(const cmd::SetViewport&         c, ReplayState& state);
    void replay_(const cmd::SetScissor&          c, ReplayState& state);
    void replay_(const cmd::SetStencilReference& c, ReplayState& state);
    void replay_(const cmd::SetBlendConstant&    c, ReplayState& state);
    void replay_(const cmd::Draw&                c, ReplayState& state);
    void replay_(const cmd::DrawIndexed&         c, ReplayState& state);
    void replay_(const cmd::PushDebugGroup&      c, ReplayState& state);
    void replay_(const cmd::PopDebugGroup&       c, ReplayState& state);

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Logger& logger_;

    std::vector<cmd::Command>    commands_;
    std::vector<ColorAttachment> colors_;
    std::vector<std::uint32_t>   dynamic_offsets_;
    std::string                  strings_;

    // Recording-time validation
    bool          in_pass_     = false;
    std::uint32_t debug_depth_ = 0;

    // Features that log "not implemented" once, not every frame.
    std::unordered_set<std::string_view> warned_;
};

} // namespace Nezumi::RHI::GL
