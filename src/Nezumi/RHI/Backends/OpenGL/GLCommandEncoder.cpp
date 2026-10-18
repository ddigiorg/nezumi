// =============================================================================
// GLCommandEncoder.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLCommandEncoder.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLDevice.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLEnumConvert.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLFramebufferCache.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <array>
#include <cstdint>
#include <type_traits>

namespace Nezumi::RHI::GL {

// =============================================================================
// Replay State
//
// What the replay loop knows about the pass it's inside.
// =============================================================================

struct GLCommandEncoder::ReplayState {
    GLDevice&                   device;
    const cmd::BeginRenderPass* pass        = nullptr;
    GLuint                      fbo         = 0;
    std::uint32_t               pass_height = 0;     // for top-left -> GL bottom-left
    bool                        skipping    = false; // pass had invalid attachments
    bool                        pass_group  = false; // pushed a debug group for the pass

    // The pass's attachment formats, which a bound pipeline must match.
    std::array<Format, MAX_COLOR_ATTACHMENTS> color_formats = {};
    std::uint32_t                             color_count   = 0;
    Format                                    depth_format  = Format::Undefined;

    // Bound state. Reset at the start of every pass (see CommandEncoder.hpp).
    const GLRenderPipeline* pipeline        = nullptr;
    BufferHandle            index_buffer    = {};
    IndexFormat             index_format    = IndexFormat::Uint16;
    std::uint64_t           index_offset    = 0;

    struct VertexBinding {
        BufferHandle  buffer = {};
        std::uint64_t offset = 0;
    };

    std::array<VertexBinding, MAX_VERTEX_BUFFERS> vertex_buffers  = {};

    // Vertex buffers must be (re)attached to the current pipeline's VAO.
    bool vertex_bindings_dirty = true;

    // Layout of the bind group bound at each index (null: none, or disturbed
    // by a pipeline switch), and the layouts of the last pipeline bound.
    // GL doesn't need these; they enforce the RHI's compatibility rule so
    // code that breaks it fails here rather than only on Vulkan.
    std::array<BindGroupLayoutHandle, MAX_BIND_GROUPS> bound_layouts    = {};
    std::array<BindGroupLayoutHandle, MAX_BIND_GROUPS> pipeline_layouts = {};
    bool have_pipeline_layouts = false;

    void reset_bindings() {
        pipeline = nullptr;
        vertex_buffers.fill({});
        index_buffer = {};
        index_offset = 0;
        vertex_bindings_dirty = true;
        bound_layouts.fill({});
        pipeline_layouts.fill({});
        have_pipeline_layouts = false;
    }
};

// =============================================================================
// Construction
// =============================================================================

GLCommandEncoder::GLCommandEncoder(Logger& logger)
    : logger_(logger)
{}

// =============================================================================
// Recording
// =============================================================================

void GLCommandEncoder::begin_render_pass(const RenderPassConfig& config) {
    if (in_pass_) {
        logger_.error("begin_render_pass('{}'): a render pass is already open", config.name);
        return;
    }

    if (config.color_attachments.size() > MAX_COLOR_ATTACHMENTS) {
        logger_.error("begin_render_pass('{}'): {} color attachments (max {})",
                      config.name, config.color_attachments.size(), MAX_COLOR_ATTACHMENTS);
        return;
    }

    if (config.color_attachments.empty() && !config.depth_stencil.texture) {
        logger_.error("begin_render_pass('{}'): no attachments", config.name);
        return;
    }

    cmd::BeginRenderPass c;
    c.first_color   = static_cast<std::uint32_t>(colors_.size());
    c.color_count   = static_cast<std::uint32_t>(config.color_attachments.size());
    c.depth_stencil = config.depth_stencil;
    c.name          = store_string_(config.name);

    colors_.insert(
        colors_.end(),
        config.color_attachments.begin(),
        config.color_attachments.end()
    );

    commands_.emplace_back(c);
    in_pass_ = true;
}

void GLCommandEncoder::end_render_pass() {
    if (!in_pass_) {
        logger_.error("end_render_pass: no render pass is open");
        return;
    }

    commands_.emplace_back(cmd::EndRenderPass{});
    in_pass_ = false;
}

void GLCommandEncoder::set_render_pipeline(RenderPipelineHandle pipeline) {
    if (!require_pass_("set_pipeline")) return;
    commands_.emplace_back(cmd::SetRenderPipeline{ pipeline });
}

void GLCommandEncoder::set_bind_group(
    std::uint32_t group,
    BindGroupHandle bind_group,
    std::span<const std::uint32_t> dynamic_offsets)
{
    if (!require_pass_("set_bind_group")) return;

    cmd::SetBindGroup c;
    c.group        = group;
    c.bind_group   = bind_group;
    c.first_offset = static_cast<std::uint32_t>(dynamic_offsets_.size());
    c.offset_count = static_cast<std::uint32_t>(dynamic_offsets.size());

    dynamic_offsets_.insert(
        dynamic_offsets_.end(),
        dynamic_offsets.begin(),
        dynamic_offsets.end()
    );

    commands_.emplace_back(c);
}

void GLCommandEncoder::set_vertex_buffer(
    std::uint32_t slot,
    BufferHandle  buffer,
    std::uint64_t offset
) {
    if (!require_pass_("set_vertex_buffer")) return;
    commands_.emplace_back(cmd::SetVertexBuffer{ slot, buffer, offset });
}

void GLCommandEncoder::set_index_buffer(
    BufferHandle  buffer,
    IndexFormat   format,
    std::uint64_t offset
) {
    if (!require_pass_("set_index_buffer")) return;
    commands_.emplace_back(cmd::SetIndexBuffer{ buffer, format, offset });
}

void GLCommandEncoder::set_viewport(const Viewport& viewport) {
    if (!require_pass_("set_viewport")) return;
    commands_.emplace_back(cmd::SetViewport{ viewport });
}

void GLCommandEncoder::set_scissor(const Rect2D& rect) {
    if (!require_pass_("set_scissor")) return;
    commands_.emplace_back(cmd::SetScissor{ rect });
}

void GLCommandEncoder::set_stencil_reference(std::uint32_t value) {
    if (!require_pass_("set_stencil_reference")) return;
    commands_.emplace_back(cmd::SetStencilReference{ value });
}

void GLCommandEncoder::set_blend_constant(const Color& color) {
    if (!require_pass_("set_blend_constant")) return;
    commands_.emplace_back(cmd::SetBlendConstant{ color });
}

void GLCommandEncoder::draw(
    std::uint32_t vertex_count,
    std::uint32_t instance_count,
    std::uint32_t first_vertex,
    std::uint32_t first_instance
) {
    if (!require_pass_("draw")) return;
    commands_.emplace_back(cmd::Draw{
        vertex_count, instance_count, first_vertex, first_instance
    });
}

void GLCommandEncoder::draw_indexed(
    std::uint32_t index_count,
    std::uint32_t instance_count,
    std::uint32_t first_index,
    std::int32_t  vertex_offset,
    std::uint32_t first_instance
) {
    if (!require_pass_("draw_indexed")) return;
    commands_.emplace_back(cmd::DrawIndexed{
        index_count, instance_count, first_index, vertex_offset, first_instance
    });
}

void GLCommandEncoder::push_debug_group(std::string_view label) {
    commands_.emplace_back(cmd::PushDebugGroup{ store_string_(label) });
    debug_depth_++;
}

void GLCommandEncoder::pop_debug_group() {
    if (debug_depth_ == 0) {
        logger_.error("pop_debug_group: no debug group is open");
        return;
    }

    commands_.emplace_back(cmd::PopDebugGroup{});
    debug_depth_--;
}

// =============================================================================
// Backend
// =============================================================================

void GLCommandEncoder::reset() {
    commands_.clear();          // clear() keeps capacity, so steady-state
    colors_.clear();            // frames don't allocate
    dynamic_offsets_.clear();
    strings_.clear();
    in_pass_ = false;
    debug_depth_ = 0;
}

void GLCommandEncoder::finish() {
    if (in_pass_) {
        logger_.error("end_frame: a render pass was left open; closing it");
        end_render_pass();
    }

    if (debug_depth_ > 0) {
        logger_.error("end_frame: {} debug group(s) left open; closing them", debug_depth_);
        while (debug_depth_ > 0) pop_debug_group();
    }
}

void GLCommandEncoder::execute(GLDevice& device) {
    ReplayState state{ device };

    for (const cmd::Command& command : commands_) {
        std::visit([&](const auto& c) {
            using C = std::decay_t<decltype(c)>;

            // Inside a pass whose attachments were invalid, skip everything
            // except the commands that keep passes and debug groups balanced.
            constexpr bool always_replay =
                std::is_same_v<C, cmd::BeginRenderPass> || std::is_same_v<C, cmd::EndRenderPass> ||
                std::is_same_v<C, cmd::PushDebugGroup>  || std::is_same_v<C, cmd::PopDebugGroup>;

            if constexpr (!always_replay) {
                if (state.skipping) return;
            }

            replay_(c, state);
        }, command);
    }
}

// =============================================================================
// Replay: render passes
// =============================================================================

void GLCommandEncoder::replay_(
    const cmd::BeginRenderPass& c,
    ReplayState& state
) {
    state.pass       = &c;
    state.skipping   = false;
    state.pass_group = false;

    FramebufferKey key;
    key.color_count = c.color_count;
    for (std::uint32_t i = 0; i < c.color_count; ++i) {
        key.colors[i] = colors_[c.first_color + i].texture;
    }
    key.depth = c.depth_stencil.texture;

    const GLuint fbo = state.device.framebuffers().get(key, state.device.textures(), logger_);

    // The pass covers its first attachment.
    const GLTexture* target = state.device.textures().get(
        c.color_count > 0 ? key.colors[0] : key.depth);

    if (!fbo || !target) {
        logger_.error("Render pass '{}' skipped: invalid attachments", view_(c.name));
        state.skipping = true;
        return;
    }

    state.fbo         = fbo;
    state.pass_height = target->height;
    state.reset_bindings();

    state.color_count = c.color_count;
    for (std::uint32_t i = 0; i < c.color_count; ++i) {
        const GLTexture* color = state.device.textures().get(key.colors[i]);
        state.color_formats[i] = color ? color->format : Format::Undefined;
    }
    const GLTexture* depth_texture = state.device.textures().get(key.depth);
    state.depth_format = depth_texture ? depth_texture->format : Format::Undefined;
    const auto width  = static_cast<GLsizei>(target->width);
    const auto height = static_cast<GLsizei>(target->height);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    if (state.device.debug() && c.name.length > 0) {
        glPushDebugGroup(
            GL_DEBUG_SOURCE_APPLICATION, 0,
            static_cast<GLsizei>(c.name.length), view_(c.name).data()
        );
        state.pass_group = true;
    }

    // Every pass starts with viewport and scissor covering the whole target
    // (see CommandEncoder.hpp). The scissor test stays enabled; a full-size
    // scissor rect is the same as no scissor.
    glViewport(0, 0, width, height);
    glDepthRangef(0.0f, 1.0f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, width, height);

    // Load ops. Clears obey the color, depth, and stencil write masks, which
    // pipelines change, so each clear first opens its mask fully.
    GLenum  discard[MAX_COLOR_ATTACHMENTS + 2];
    GLsizei discard_count = 0;

    for (std::uint32_t i = 0; i < c.color_count; ++i) {
        const ColorAttachment& a = colors_[c.first_color + i];
        switch (a.load_op) {
            case LoadOp::Load:
                break;
            case LoadOp::Clear: {
                const GLfloat rgba[4] = {
                    a.clear_color.r, a.clear_color.g,
                    a.clear_color.b, a.clear_color.a
                };
                glColorMaski(i, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glClearNamedFramebufferfv(fbo, GL_COLOR, static_cast<GLint>(i), rgba);
                break;
            }
            case LoadOp::DontCare:
                discard[discard_count++] = GL_COLOR_ATTACHMENT0 + i;
                break;
        }
    }

    if (const GLTexture* depth = state.device.textures().get(key.depth)) {
        const DepthStencilAttachment& a = c.depth_stencil;

        if (is_depth_format(depth->format)) {
            switch (a.depth_load_op) {
                case LoadOp::Load:
                    break;
                case LoadOp::Clear:
                    glDepthMask(GL_TRUE);
                    glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &a.clear_depth);
                    break;
                case LoadOp::DontCare:
                    discard[discard_count++] = GL_DEPTH_ATTACHMENT;
                    break;
            }
        }

        if (has_stencil(depth->format)) {
            switch (a.stencil_load_op) {
                case LoadOp::Load:
                    break;
                case LoadOp::Clear: {
                    const GLint stencil = a.clear_stencil;
                    glStencilMask(0xFF);
                    glClearNamedFramebufferiv(fbo, GL_STENCIL, 0, &stencil);
                    break;
                }
                case LoadOp::DontCare:
                    discard[discard_count++] = GL_STENCIL_ATTACHMENT;
                    break;
            }
        }
    }

    if (discard_count > 0) {
        glInvalidateNamedFramebufferData(fbo, discard_count, discard);
    }

    // The clears above changed write masks without going through the cache.
    state.device.state_cache().write_masks_changed();
}

void GLCommandEncoder::replay_(const cmd::EndRenderPass&, ReplayState& state) {
    // Store ops: DontCare tells the driver the contents needn't survive.
    if (state.pass && !state.skipping) {
        const cmd::BeginRenderPass& c = *state.pass;

        GLenum  discard[MAX_COLOR_ATTACHMENTS + 2];
        GLsizei discard_count = 0;

        for (std::uint32_t i = 0; i < c.color_count; ++i) {
            if (colors_[c.first_color + i].store_op == StoreOp::DontCare) {
                discard[discard_count++] = GL_COLOR_ATTACHMENT0 + i;
            }
        }

        if (const GLTexture* depth = state.device.textures().get(c.depth_stencil.texture)) {
            if (is_depth_format(depth->format) && c.depth_stencil.depth_store_op == StoreOp::DontCare) {
                discard[discard_count++] = GL_DEPTH_ATTACHMENT;
            }
            if (has_stencil(depth->format) && c.depth_stencil.stencil_store_op == StoreOp::DontCare) {
                discard[discard_count++] = GL_STENCIL_ATTACHMENT;
            }
        }

        if (discard_count > 0) {
            glInvalidateNamedFramebufferData(state.fbo, discard_count, discard);
        }
    }

    if (state.pass_group) glPopDebugGroup();

    state.pass       = nullptr;
    state.fbo        = 0;
    state.skipping   = false;
    state.pass_group = false;
    state.reset_bindings();
}

// =============================================================================
// Replay: pipelines and vertex input
// =============================================================================

void GLCommandEncoder::replay_(const cmd::SetRenderPipeline& c, ReplayState& state) {
    const GLRenderPipeline* pipeline = state.device.pipelines().get(c.pipeline);
    if (!pipeline) {
        logger_.error("set_pipeline: handle is null, stale, or destroyed");
        state.pipeline = nullptr;
        return;
    }

    // A pipeline bakes in its attachment formats; using it with a pass whose
    // attachments differ is an error on Vulkan and D3D12, so it is here too.
    bool matches = pipeline->state.color_target_count == state.color_count
                && pipeline->depth_format == state.depth_format;
    for (std::uint32_t i = 0; matches && i < state.color_count; ++i) {
        matches = pipeline->color_formats[i] == state.color_formats[i];
    }

    if (!matches) {
        logger_.error("set_pipeline('{}'): its attachment formats don't match render pass '{}'; "
                      "draws are skipped until another pipeline is set",
                      pipeline->name, state.pass ? view_(state.pass->name) : std::string_view());
        state.pipeline = nullptr;
        return;
    }

    // Compatibility rule (CommandEncoder.hpp): a bound group survives only if
    // the new pipeline's layout matches at its index and every lower index.
    if (state.have_pipeline_layouts) {
        std::uint32_t first_different = MAX_BIND_GROUPS;
        for (std::uint32_t i = 0; i < MAX_BIND_GROUPS; ++i) {
            if (state.pipeline_layouts[i] != pipeline->bind_group_layouts[i]) {
                first_different = i;
                break;
            }
        }
        for (std::uint32_t i = first_different; i < MAX_BIND_GROUPS; ++i) {
            state.bound_layouts[i] = {};
        }
    }
    state.pipeline_layouts      = pipeline->bind_group_layouts;
    state.have_pipeline_layouts = true;

    if (pipeline == state.pipeline) return;

    glUseProgram(pipeline->program);
    glBindVertexArray(pipeline->vao);
    state.device.state_cache().apply(pipeline->state);

    state.pipeline              = pipeline;
    state.vertex_bindings_dirty = true;   // buffers attach to the new VAO
}

void GLCommandEncoder::replay_(const cmd::SetVertexBuffer& c, ReplayState& state) {
    if (c.slot >= MAX_VERTEX_BUFFERS) {
        logger_.error("set_vertex_buffer: slot {} exceeds the maximum of {}", c.slot, MAX_VERTEX_BUFFERS);
        return;
    }
    state.vertex_buffers[c.slot] = { c.buffer, c.offset };
    state.vertex_bindings_dirty  = true;
}

void GLCommandEncoder::replay_(const cmd::SetIndexBuffer& c, ReplayState& state) {
    state.index_buffer = c.buffer;
    state.index_format = c.format;
    state.index_offset = c.offset;
}

// Binds each resource of the group at its GL binding point (GLLimits.hpp).
// On any error the group index is left unbound, so draws that need it are
// skipped with a message rather than using a half-bound group.
void GLCommandEncoder::replay_(const cmd::SetBindGroup& c, ReplayState& state) {
    if (c.group >= MAX_BIND_GROUPS) {
        logger_.error("set_bind_group: group index {} exceeds the maximum of {}", c.group, MAX_BIND_GROUPS - 1);
        return;
    }
    state.bound_layouts[c.group] = {};

    const GLBindGroup* group = state.device.bind_groups().get(c.bind_group);
    if (!group) {
        logger_.error("set_bind_group({}): handle is null, stale, or destroyed", c.group);
        return;
    }

    if (c.offset_count != group->dynamic_offset_count) {
        logger_.error("set_bind_group('{}'): {} dynamic offsets given, its layout needs {}",
                      group->name, c.offset_count, group->dynamic_offset_count);
        return;
    }

    GLuint sampler = 0;
    if (group->sampler) {
        const GLSampler* s = state.device.samplers().get(group->sampler);
        if (!s) {
            logger_.error("set_bind_group('{}'): its sampler has been destroyed", group->name);
            return;
        }
        sampler = s->id;
    }

    const std::uint32_t* dynamic = dynamic_offsets_.data() + c.first_offset;
    std::uint32_t        next    = 0;

    for (const GLBindGroupEntry& entry : group->entries) {
        const GLuint point = gl_binding(c.group, entry.binding);

        switch (entry.type) {
            case BindType::UniformBuffer:
            case BindType::StorageBuffer:
            case BindType::ReadOnlyStorageBuffer: {
                const bool      uniform = entry.type == BindType::UniformBuffer;
                const GLBuffer* buffer  = state.device.buffers().get(entry.buffer);
                if (!buffer) {
                    logger_.error("set_bind_group('{}'): binding {} refers to a destroyed buffer",
                                  group->name, entry.binding);
                    return;
                }

                std::uint64_t offset = entry.offset;
                if (entry.has_dynamic_offset) {
                    const std::uint32_t extra     = dynamic[next++];
                    const std::uint32_t alignment = uniform
                        ? state.device.info().uniform_buffer_offset_alignment
                        : state.device.binding_limits().storage_buffer_offset_alignment;
                    if (alignment && extra % alignment != 0) {
                        logger_.error("set_bind_group('{}'): dynamic offset {} isn't a multiple of {}",
                                      group->name, extra, alignment);
                        return;
                    }
                    offset += extra;
                    if (offset + entry.size > buffer->size) {
                        logger_.error("set_bind_group('{}'): dynamic offset {} puts binding {} past "
                                      "the end of its buffer", group->name, extra, entry.binding);
                        return;
                    }
                }

                glBindBufferRange(uniform ? GL_UNIFORM_BUFFER : GL_SHADER_STORAGE_BUFFER, point,
                                  buffer->id, static_cast<GLintptr>(offset),
                                  static_cast<GLsizeiptr>(entry.size));
                break;
            }

            case BindType::SampledTexture:
            case BindType::StorageTexture: {
                const GLTexture* texture = state.device.textures().get(entry.texture);
                if (!texture) {
                    logger_.error("set_bind_group('{}'): binding {} refers to a destroyed texture",
                                  group->name, entry.binding);
                    return;
                }

                if (entry.type == BindType::SampledTexture) {
                    glBindTextureUnit(point, texture->id);
                    glBindSampler(point, sampler);      // 0 = the texture's own state
                } else {
                    const bool layered = texture->type != TextureType::Texture2D;
                    glBindImageTexture(point, texture->id, 0, layered ? GL_TRUE : GL_FALSE, 0,
                                       GL_READ_WRITE, to_gl_internal_format(texture->format));
                }
                break;
            }

            case BindType::Sampler:
                break;   // not stored as an entry; applied with the textures above
        }
    }

    state.bound_layouts[c.group] = group->layout;
}

void GLCommandEncoder::replay_(
    const cmd::SetStencilReference& c,
    ReplayState& state
) {
    state.device.state_cache().set_stencil_reference(
        static_cast<GLint>(c.value)
    );
}

// Checks a draw can happen and attaches the bound vertex buffers to the
// current pipeline's VAO if anything changed since the last draw. False (and
// logged) if there's no valid pipeline, a bind group the pipeline expects
// isn't bound, or a vertex buffer slot the pipeline reads is empty.
bool GLCommandEncoder::prepare_draw_(ReplayState& state, const char* draw) {
    if (!state.pipeline) {
        logger_.error("{}: no valid pipeline is set", draw);
        return false;
    }

    const GLRenderPipeline& pipeline = *state.pipeline;
    for (std::uint32_t group = 0; group < pipeline.bind_group_count; ++group) {
        if (state.bound_layouts[group] != pipeline.bind_group_layouts[group]) {
            logger_.error("{}: pipeline '{}' expects a bind group at index {} with the layout it was "
                          "created with, and none is bound (or a pipeline switch disturbed it)",
                          draw, pipeline.name, group);
            return false;
        }
    }

    if (!state.vertex_bindings_dirty) return true;
    for (std::uint32_t slot = 0; slot < pipeline.vertex_buffer_count; ++slot) {
        const auto&     binding = state.vertex_buffers[slot];
        const GLBuffer* buffer  = state.device.buffers().get(binding.buffer);
        if (!buffer) {
            logger_.error("{}: pipeline '{}' reads vertex buffer slot {}, which isn't set",
                          draw, pipeline.name, slot);
            return false;
        }
        glVertexArrayVertexBuffer(pipeline.vao, slot, buffer->id,
                                  static_cast<GLintptr>(binding.offset), pipeline.strides[slot]);
    }

    state.vertex_bindings_dirty = false;
    return true;
}

// =============================================================================
// Replay: draws
//
// Note for shaders: with first_instance, per-instance attributes start at
// that instance, but GLSL's gl_InstanceID still starts at 0 (Vulkan's
// gl_InstanceIndex includes the base). Prefer instance attributes over
// gl_InstanceID so shaders behave the same on every backend.
// =============================================================================

void GLCommandEncoder::replay_(const cmd::Draw& c, ReplayState& state) {
    if (!prepare_draw_(state, "draw")) return;

    glDrawArraysInstancedBaseInstance(
        state.pipeline->topology,
        static_cast<GLint>(c.first_vertex),
        static_cast<GLsizei>(c.vertex_count),
        static_cast<GLsizei>(c.instance_count),
        c.first_instance
    );
}

void GLCommandEncoder::replay_(const cmd::DrawIndexed& c, ReplayState& state) {
    if (!prepare_draw_(state, "draw_indexed")) return;

    const GLBuffer* indices = state.device.buffers().get(state.index_buffer);
    if (!indices) {
        logger_.error("draw_indexed: no index buffer is set");
        return;
    }

    // The index buffer binding is part of the VAO; setting it every indexed
    // draw is cheap and always correct after a pipeline switch.
    glVertexArrayElementBuffer(state.pipeline->vao, indices->id);

    const std::uint64_t byte_offset = state.index_offset
                                    + std::uint64_t{c.first_index}
                                    * index_size(state.index_format);

    glDrawElementsInstancedBaseVertexBaseInstance(
        state.pipeline->topology,
        static_cast<GLsizei>(c.index_count),
        to_gl_index_type(state.index_format),
        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(byte_offset)),
        static_cast<GLsizei>(c.instance_count),
        c.vertex_offset,
        c.first_instance
    );
}

// =============================================================================
// Replay: dynamic state
//
// The RHI puts the viewport and scissor origin at the top-left of the target;
// GL puts it at the bottom-left, so y is flipped against the pass height.
// =============================================================================

void GLCommandEncoder::replay_(const cmd::SetViewport& c, ReplayState& state) {
    const Viewport& v = c.viewport;
    const float y = static_cast<float>(state.pass_height) - (v.y + v.height);

    glViewportIndexedf(0, v.x, y, v.width, v.height);
    glDepthRangef(v.min_depth, v.max_depth);
}

void GLCommandEncoder::replay_(const cmd::SetScissor& c, ReplayState& state) {
    const Rect2D& r = c.rect;
    const GLint   y = static_cast<GLint>(state.pass_height) - r.offset.y
                    - static_cast<GLint>(r.extent.height);

    glScissor(
        r.offset.x, y,
        static_cast<GLsizei>(r.extent.width),
        static_cast<GLsizei>(r.extent.height)
    );
}

void GLCommandEncoder::replay_(const cmd::SetBlendConstant& c, ReplayState&) {
    glBlendColor(c.color.r, c.color.g, c.color.b, c.color.a);
}


// =============================================================================
// Replay: debug groups (no-ops unless the device was created with debug)
// =============================================================================

void GLCommandEncoder::replay_(
    const cmd::PushDebugGroup& c,
    ReplayState& state)
{
    if (!state.device.debug()) return;
    glPushDebugGroup(
        GL_DEBUG_SOURCE_APPLICATION, 0,
        static_cast<GLsizei>(c.label.length),
        view_(c.label).data()
    );
}

void GLCommandEncoder::replay_(const cmd::PopDebugGroup&, ReplayState& state) {
    if (!state.device.debug()) return;
    glPopDebugGroup();
}

// =============================================================================
// Helpers
// =============================================================================

bool GLCommandEncoder::require_pass_(const char* function) {
    if (in_pass_) return true;
    logger_.error("{}: must be called inside a render pass", function);
    return false;
}

cmd::StringRef GLCommandEncoder::store_string_(std::string_view text) {
    const cmd::StringRef ref{
        static_cast<std::uint32_t>(strings_.size()),
        static_cast<std::uint32_t>(text.size())
    };
    strings_.append(text);
    return ref;
}

std::string_view GLCommandEncoder::view_(cmd::StringRef ref) const {
    return std::string_view(strings_).substr(ref.offset, ref.length);
}

void GLCommandEncoder::not_implemented_(const char* what) {
    if (warned_.insert(what).second) {
        logger_.warn("OpenGL backend: {} is not implemented yet", what);
    }
}

} // namespace Nezumi::RHI::GL
