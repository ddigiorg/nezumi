// =============================================================================
// GLDevice.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLDevice.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLEnumConvert.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLInfo.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <algorithm>
#include <glad/gl.h>
#include <stdexcept>
#include <utility>

namespace Nezumi::RHI::GL {

namespace {

Logger& require_logger(Logger* logger) {
    if (!logger) throw std::runtime_error("GLDevice: DeviceConfig::logger is null");
    return *logger;
}

// Size of a dimension at a mip level: halved per level, never below 1.
std::uint32_t mip_extent(std::uint32_t size, std::uint32_t level) {
    return std::max(1u, size >> level);
}

// Mip levels in a full chain down to 1x1.
std::uint32_t max_mip_levels(std::uint32_t largest) {
    std::uint32_t levels = 1;
    while (largest > 1) { largest >>= 1; ++levels; }
    return levels;
}

// Uploads go through GL_COPY_WRITE_BUFFER, a binding point nothing else
// uses: GL_ARRAY_BUFFER would work too, but GL_ELEMENT_ARRAY_BUFFER is VAO
// state and GL_UNIFORM_BUFFER is shared with bind groups.
void upload_buffer(GLuint id, std::uint64_t offset, std::uint64_t size, const void* data) {
    glBindBuffer(GL_COPY_WRITE_BUFFER, id);
    glBufferSubData(GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(offset),
                    static_cast<GLsizeiptr>(size), data);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

// Needs GL_KHR_debug (core in 4.3), which a 3.3 driver may not have. The
// object must already exist, i.e. have been bound once after glGen*.
void set_label(GLenum kind, GLuint id, std::string_view name) {
    if (GLAD_GL_KHR_debug && !name.empty()) {
        glObjectLabel(kind, id, static_cast<GLsizei>(name.size()), name.data());
    }
}

} // namespace

// =============================================================================
// Construction & Destruction
// =============================================================================

GLDevice::GLDevice(
    SDL_Window* window,
    Logger* logger,
    const DeviceConfig& config
)
    : logger_          (&require_logger(logger))
    , debug_           (config.enable_debug)
    , frames_in_flight_(config.frames_in_flight)
    , context_         (window, logger, config.enable_debug, config.enable_vsync)
    , info_            (query_device_info())
    , binding_limits_  (query_binding_limits())
    , encoder_         (*logger)
{
    // Rows of uploaded pixel data are tightly packed (GL defaults to 4-byte
    // row alignment, which breaks e.g. R8 textures with odd widths).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (config.srgb_backbuffer) {
        logger_->warn("OpenGL backend: srgb_backbuffer is not supported yet; using RGBA8Unorm");
    }

    recreate_backbuffer_(context_.drawable_extent());

    logger_->info(
        "Device '{}' created: {} {} (debug output: {}, vsync: {})",
        config.name,
        info_.api_name,
        info_.api_version,
        config.enable_debug ? "on" : "off",
        config.enable_vsync ? "on" : "off"
    );
}

GLDevice::~GLDevice() {
    flush_pending_removes_();
    framebuffers_.clear();
    report_leaks_();
}

// =============================================================================
// Buffers
// =============================================================================

BufferHandle GLDevice::create(const BufferConfig& config) {
    if (config.size == 0) {
        logger_->error("create_buffer('{}'): size is 0", config.name);
        return {};
    }

    if (config.usage == BufferUsageFlags::None) {
        logger_->error("create_buffer('{}'): usage is None", config.name);
        return {};
    }

    if (config.initial_data.size() > config.size) {
        logger_->error("create_buffer('{}'): initial_data ({} bytes) is larger than size ({})",
                       config.name, config.initial_data.size(), config.size);
        return {};
    }

    GLBuffer buffer;
    buffer.size  = config.size;
    buffer.usage = config.usage;
    buffer.name  = std::string(config.name);

    glGenBuffers(1, &buffer.id);
    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer.id);   // creates the object

    // GL 3.3 has no immutable storage (glBufferStorage is 4.4). The usage
    // hint does the same job: buffers written after creation are DYNAMIC,
    // the rest STATIC, telling the driver they never change.
    const GLenum hint = has_any(config.usage, BufferUsageFlags::CopyDst) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    const auto   size = static_cast<GLsizeiptr>(config.size);

    if (config.initial_data.empty()) {
        glBufferData(GL_COPY_WRITE_BUFFER, size, nullptr, hint);
    } else if (config.initial_data.size() == config.size) {
        glBufferData(GL_COPY_WRITE_BUFFER, size, config.initial_data.data(), hint);
    } else {
        // Partial initial data: zero-fill the rest and upload in one call.
        std::vector<std::byte> full(config.size);
        std::copy(config.initial_data.begin(), config.initial_data.end(), full.begin());
        glBufferData(GL_COPY_WRITE_BUFFER, size, full.data(), hint);
    }

    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    if (debug_) set_label(GL_BUFFER, buffer.id, buffer.name);
    return buffers_.create(std::move(buffer)).handle;
}

void GLDevice::write_buffer(
    BufferHandle handle,
    std::uint64_t offset,
    std::span<const std::byte> data
) {
    const GLBuffer* buffer = live_(buffers_, handle);

    if (!buffer) {
        logger_->error("write_buffer: handle is null, stale, or destroyed");
        return;
    }

    if (!has_any(buffer->usage, BufferUsageFlags::CopyDst)) {
        logger_->error("write_buffer('{}'): buffer was created without BufferUsageFlags::CopyDst",
                       buffer->name);
        return;
    }

    if (offset + data.size() > buffer->size) {
        logger_->error("write_buffer('{}'): {} bytes at offset {} exceeds size {}",
                       buffer->name, data.size(), offset, buffer->size);
        return;
    }

    if (data.empty()) return;

    if (!in_frame_) {
        upload_buffer(buffer->id, offset, data.size(), data.data());
        return;
    }

    // Catch the bug the upload rule makes easy to write: two writes to the
    // same bytes in one frame, expecting draws in between to see different
    // data. GL would happen to order them; Vulkan/D3D12 wouldn't.
    if (debug_) {
        for (const PendingBufferWrite& w : buffer_writes_) {
            if (w.buffer == handle &&
                offset < w.offset + w.size &&
                w.offset < offset + data.size())
            {
                logger_->warn(
                    "write_buffer('{}'): bytes {}..{} overlap an earlier write this "
                    "frame; every draw this frame sees only the last write",
                    buffer->name, offset, offset + data.size()
                );
                break;
            }
        }
    }

    buffer_writes_.push_back({ handle, offset, staging_.size(), data.size() });
    staging_.insert(staging_.end(), data.begin(), data.end());
}

// =============================================================================
// Textures
// =============================================================================

TextureHandle GLDevice::create(const TextureConfig& config) {
    auto fail = [&](auto&& why) -> TextureHandle {
        logger_->error("create_texture('{}'): {}", config.name, why);
        return {};
    };

    if (config.format == Format::Undefined)
        return fail("format is Undefined");

    if (config.width == 0 || config.height == 0)
        return fail("width and height must be nonzero");

    const std::uint32_t layers = config.depth_or_layers;
    switch (config.type) {
        case TextureType::Texture1D:
            if (layers != 1)
                return fail("a Texture1D must have depth_or_layers = 1");
            break;
        case TextureType::Texture2D:
            if (layers != 1)
                return fail("a Texture2D must have depth_or_layers = 1");
            break;
        case TextureType::Texture2DArray:
            if (layers == 0)
                return fail("depth_or_layers must be at least 1");
            if (layers > info_.max_texture_array_layers)
                return fail("too many array layers");
            break;
        case TextureType::TextureCube:
            if (layers != 6)
                return fail("a TextureCube must have depth_or_layers = 6");
            if (config.width != config.height)
                return fail("cube faces must be square");
            break;
        case TextureType::Texture3D:
            if (layers == 0)
                return fail("depth_or_layers must be at least 1");
            break;
    }

    if (config.width > info_.max_texture_size || config.height > info_.max_texture_size) {
        return fail("larger than the device's max_texture_size");
    }

    const std::uint32_t largest = config.type == TextureType::Texture3D
        ? std::max({ config.width, config.height, layers })
        : std::max(config.width, config.height);

    if (config.mip_levels == 0 || config.mip_levels > max_mip_levels(largest)) {
        return fail("mip_levels must be between 1 and a full mip chain");
    }

    const std::uint64_t mip0_size = get_image_size(
        config.format,
        config.width,
        config.height,
        layers
    );

    if (!config.initial_data.empty() && config.initial_data.size() < mip0_size) {
        return fail("initial_data is smaller than mip 0 of every layer");
    }

    GLTexture texture;
    texture.target          = to_gl_texture_target(config.type);
    texture.type            = config.type;
    texture.format          = config.format;
    texture.width           = config.width;
    texture.height          = config.height;
    texture.depth_or_layers = layers;
    texture.mip_levels      = config.mip_levels;
    texture.usage           = config.usage;
    texture.name            = std::string(config.name);

    glGenTextures(1, &texture.id);
    glBindTexture(texture.target, texture.id);   // creates the object

    // GL 3.3 has no immutable storage (glTexStorage is 4.2), so each mip
    // level is allocated separately. The format and type passed with the
    // null data must still be a valid pairing for the internal format.
    const GLenum         internal = to_gl_internal_format(config.format);
    const GLUploadFormat upload   = to_gl_upload_format(config.format);

    for (std::uint32_t level = 0; level < config.mip_levels; ++level) {
        const auto mip = static_cast<GLint>(level);
        const auto w   = static_cast<GLsizei>(mip_extent(config.width,  level));
        const auto h   = static_cast<GLsizei>(mip_extent(config.height, level));
        const auto d   = static_cast<GLsizei>(config.type == TextureType::Texture3D
                                                  ? mip_extent(layers, level) : layers);

        switch (config.type) {
            case TextureType::Texture1D:
                glTexImage1D(GL_TEXTURE_1D, mip, static_cast<GLint>(internal), w, 0,
                             upload.format, upload.type, nullptr);
                break;
            case TextureType::Texture2D:
                glTexImage2D(GL_TEXTURE_2D, mip, static_cast<GLint>(internal), w, h, 0,
                             upload.format, upload.type, nullptr);
                break;
            case TextureType::TextureCube:
                for (GLenum face = 0; face < 6; ++face) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, static_cast<GLint>(internal),
                                 w, h, 0, upload.format, upload.type, nullptr);
                }
                break;
            case TextureType::Texture2DArray:
            case TextureType::Texture3D:
                glTexImage3D(texture.target, mip, static_cast<GLint>(internal), w, h, d, 0,
                             upload.format, upload.type, nullptr);
                break;
        }
    }

    // Without immutable storage GL assumes a full mip chain; capping the
    // level range keeps a texture with fewer levels "complete".
    glTexParameteri(texture.target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(texture.target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(config.mip_levels - 1));

    // GL's default minification filter samples mipmaps, which makes a texture
    // with a single level "incomplete" and read back black when no sampler
    // is bound. Samplers override this; it's a safety net.
    glTexParameteri(
        texture.target,
        GL_TEXTURE_MIN_FILTER,
        config.mip_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
    );

    if (!config.initial_data.empty()) {
        TextureRegion all;                   // mip 0, every layer, tightly packed
        all.width           = config.width;
        all.height          = config.height;
        all.depth_or_layers = layers;
        upload_texture_(texture, all, config.initial_data.data());
    }

    if (debug_) set_label(GL_TEXTURE, texture.id, texture.name);
    return textures_.create(std::move(texture)).handle;
}

void GLDevice::write_texture(
    TextureHandle handle,
    const TextureRegion& region,
    std::span<const std::byte> data
) {
    const GLTexture* texture = live_(textures_, handle);

    if (!texture) {
        logger_->error("write_texture: handle is null, stale, or destroyed");
        return;
    }

    if (!has_any(texture->usage, TextureUsageFlags::CopyDst)) {
        logger_->error("write_texture('{}'): texture was created without TextureUsage::CopyDst",
                       texture->name);
        return;
    }

    TextureRegion resolved;
    const std::size_t size = resolve_region_(*texture, region, data, resolved);
    if (size == 0) return;

    if (!in_frame_) {
        upload_texture_(*texture, resolved, data.data());
        return;
    }

    texture_writes_.push_back({ handle, resolved, staging_.size(), size });
    staging_.insert(staging_.end(), data.begin(), data.begin() + static_cast<std::ptrdiff_t>(size));
}

std::size_t GLDevice::resolve_region_(
    const GLTexture&           texture,
    const TextureRegion&       in,
    std::span<const std::byte> data,
    TextureRegion&             out
) const {
    auto fail = [&](auto&& why) -> std::size_t {
        logger_->error("write_texture('{}'): {}", texture.name, why);
        return 0;
    };

    if (in.mip_level >= texture.mip_levels) return fail("mip_level out of range");

    // Array layers don't shrink with mip level; 3D depth does.
    const std::uint32_t mip_w = mip_extent(texture.width,  in.mip_level);
    const std::uint32_t mip_h = mip_extent(texture.height, in.mip_level);
    const std::uint32_t mip_d = texture.type == TextureType::Texture3D
        ? mip_extent(texture.depth_or_layers, in.mip_level)
        : texture.depth_or_layers;

    if (in.x >= mip_w || in.y >= mip_h || in.z >= mip_d) {
        return fail("region origin is outside the mip level");
    }

    out        = in;
    out.width  = in.width  ? in.width  : mip_w - in.x;
    out.height = in.height ? in.height : mip_h - in.y;
    out.depth_or_layers = in.depth_or_layers ? in.depth_or_layers : mip_d - in.z;

    if (in.x + out.width > mip_w  ||
        in.y + out.height > mip_h ||
        in.z + out.depth_or_layers > mip_d)
    {
        return fail("region extends past the edge of the mip level");
    }

    const std::uint32_t bpp       = get_format_info(texture.format).bytes_per_pixel;
    const std::uint64_t tight_row = get_row_size(texture.format, out.width);
    if (in.bytes_per_row != 0) {
        if (in.bytes_per_row % bpp != 0)
            return fail("bytes_per_row isn't a whole number of pixels");
        if (in.bytes_per_row < tight_row)
            return fail("bytes_per_row is smaller than one row");
    }

    const std::uint64_t pitch    = in.bytes_per_row ? in.bytes_per_row : tight_row;
    const std::uint64_t required = pitch * out.height * out.depth_or_layers;
    if (data.size() < required) {
        logger_->error("write_texture('{}'): data is {} bytes, region needs {}",
                       texture.name, data.size(), required);
        return 0;
    }

    return static_cast<std::size_t>(required);
}

void GLDevice::upload_texture_(
    const GLTexture& texture,
    const TextureRegion& region,
    const std::byte* data
) const {
    const GLUploadFormat upload = to_gl_upload_format(texture.format);

    // GL_UNPACK_ROW_LENGTH is in pixels; 0 means tightly packed.
    const std::uint32_t bpp        = get_format_info(texture.format).bytes_per_pixel;
    const auto          row_length = static_cast<GLint>(region.bytes_per_row / bpp);
    if (row_length) glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);

    const auto mip = static_cast<GLint>(region.mip_level);
    const auto x   = static_cast<GLint>(region.x);
    const auto y   = static_cast<GLint>(region.y);
    const auto z   = static_cast<GLint>(region.z);
    const auto w   = static_cast<GLsizei>(region.width);
    const auto h   = static_cast<GLsizei>(region.height);
    const auto d   = static_cast<GLsizei>(region.depth_or_layers);

    glBindTexture(texture.target, texture.id);

    switch (texture.type) {
        case TextureType::Texture1D:
            glTexSubImage1D(GL_TEXTURE_1D, mip, x, w, upload.format, upload.type, data);
            break;
        case TextureType::Texture2D:
            glTexSubImage2D(GL_TEXTURE_2D, mip, x, y, w, h, upload.format, upload.type, data);
            break;
        case TextureType::TextureCube: {
            // Without DSA each face is its own 2D target; z selects the first.
            const std::uint64_t pitch = region.bytes_per_row ? region.bytes_per_row
                                                             : get_row_size(texture.format, region.width);
            for (GLsizei face = 0; face < d; ++face) {
                glTexSubImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + z + face),
                                mip, x, y, w, h, upload.format, upload.type,
                                data + pitch * region.height * static_cast<std::uint64_t>(face));
            }
            break;
        }
        case TextureType::Texture2DArray:
        case TextureType::Texture3D:
            glTexSubImage3D(texture.target, mip, x, y, z, w, h, d, upload.format, upload.type, data);
            break;
    }

    if (row_length) glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

// =============================================================================
// Samplers
// =============================================================================

SamplerHandle GLDevice::create(const SamplerConfig& config) {
    GLSampler sampler;
    sampler.name = std::string(config.name);

    glGenSamplers(1, &sampler.id);

    glSamplerParameteri(
        sampler.id,
        GL_TEXTURE_MIN_FILTER,
        static_cast<GLint>(to_gl_min_filter(config.min_filter, config.mip_filter))
    );

    glSamplerParameteri(
        sampler.id,
        GL_TEXTURE_MAG_FILTER,
        static_cast<GLint>(to_gl_mag_filter(config.mag_filter))
    );

    glSamplerParameteri(
        sampler.id,
        GL_TEXTURE_WRAP_S,
        static_cast<GLint>(to_gl_address_mode(config.address_u))
    );

    glSamplerParameteri(
        sampler.id,
        GL_TEXTURE_WRAP_T,
        static_cast<GLint>(to_gl_address_mode(config.address_v))
    );

    glSamplerParameteri(
        sampler.id,
        GL_TEXTURE_WRAP_R,
        static_cast<GLint>(to_gl_address_mode(config.address_w))
    );

    glSamplerParameterf(
        sampler.id,
        GL_TEXTURE_MIN_LOD,
        config.min_lod
    );

    glSamplerParameterf(sampler.id, GL_TEXTURE_MAX_LOD, config.max_lod);

    if (config.max_anisotropy > 1.0f) {
        if (info_.max_sampler_anisotropy > 1.0f) {
            glSamplerParameterf(sampler.id, GL_TEXTURE_MAX_ANISOTROPY,
                                std::min(config.max_anisotropy, info_.max_sampler_anisotropy));
        } else {
            logger_->warn("create_sampler('{}'): anisotropic filtering isn't supported; ignoring",
                          config.name);
        }
    }

    if (debug_) set_label(GL_SAMPLER, sampler.id, sampler.name);
    return samplers_.create(std::move(sampler)).handle;
}

// =============================================================================
// Shaders
// =============================================================================

ShaderHandle GLDevice::create(const ShaderConfig& config) {
    auto fail = [&](auto&& why) -> ShaderHandle {
        logger_->error("create_shader('{}'): {}", config.name, why);
        return {};
    };

    const GLenum stage = to_gl_shader_stage(config.stage);
    if (stage == GL_NONE)
        return fail("stage must be exactly one of Vertex, Fragment (compute needs OpenGL 4.3)");
    if (config.code.empty())
        return fail("code is empty");

    switch (config.format) {
        case ShaderFormat::Glsl:
            break;
        case ShaderFormat::Spirv:
            return fail("SPIR-V isn't supported by the OpenGL backend yet; use ShaderFormat::Glsl");
    }
    if (config.entry_point != "main") return fail("GLSL shaders must use 'main' as the entry point");

    GLShader shader;
    shader.stage = config.stage;
    shader.name  = std::string(config.name);
    shader.id    = glCreateShader(stage);

    const auto* source = reinterpret_cast<const GLchar*>(config.code.data());
    const auto  length = static_cast<GLint>(config.code.size());
    glShaderSource(shader.id, 1, &source, &length);
    glCompileShader(shader.id);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint capacity = 0;
        glGetShaderiv(shader.id, GL_INFO_LOG_LENGTH, &capacity);
        std::string log(static_cast<std::size_t>(std::max(capacity, 1)), '\0');
        GLsizei written = 0;
        glGetShaderInfoLog(shader.id, capacity, &written, log.data());
        log.resize(static_cast<std::size_t>(written));
        while (!log.empty() && (log.back() == '\n' || log.back() == ' ')) log.pop_back();

        glDeleteShader(shader.id);
        return fail("compilation failed:\n" + log);
    }

    if (debug_) set_label(GL_SHADER, shader.id, shader.name);
    return shaders_.create(std::move(shader)).handle;
}

// =============================================================================
// Pipelines
// =============================================================================

RenderPipelineHandle GLDevice::create(const RenderPipelineConfig& config) {
    auto fail = [&](auto&& why) -> RenderPipelineHandle {
        logger_->error("create_pipeline('{}'): {}", config.name, why);
        return {};
    };

    // ---- Validate everything before creating any GL objects ----------------
    const GLShader* vs = live_(shaders_, config.vertex_shader);
    const GLShader* fs = live_(shaders_, config.fragment_shader);

    if (!vs || vs->stage != ShaderStageFlags::Vertex)
        return fail("vertex_shader is missing, destroyed, or not a vertex shader");

    if (!fs || fs->stage != ShaderStageFlags::Fragment)
        return fail("fragment_shader is missing, destroyed, or not a fragment shader");

    if (config.vertex_buffers.size() > std::min(MAX_VERTEX_BUFFERS, info_.max_vertex_buffers)) {
        return fail("too many vertex buffers");
    }

    for (const VertexBufferLayout& layout : config.vertex_buffers) {
        for (const VertexAttribute& attribute : layout.attributes) {
            if (attribute.location >= info_.max_vertex_attributes) {
                return fail("vertex attribute location exceeds the device limit");
            }
        }
    }

    if (config.color_targets.size() > MAX_COLOR_ATTACHMENTS)
        return fail("too many color targets");

    for (const ColorTargetState& target : config.color_targets) {
        if (target.format == Format::Undefined) return fail("a color target's format is Undefined");
        if (is_depth_format(target.format))     return fail("a color target has a depth format");
    }

    // GL 3.3 has one blend function and equation for all draw buffers
    // (glBlendFuncSeparatei is 4.0); only enabling blending is per target.
    for (const ColorTargetState& target : config.color_targets) {
        const BlendState& a = target.blend;
        const BlendState& b = config.color_targets[0].blend;
        if (a.enabled && b.enabled &&
            (a.color.src != b.color.src || a.color.dst != b.color.dst || a.color.op != b.color.op ||
             a.alpha.src != b.alpha.src || a.alpha.dst != b.alpha.dst || a.alpha.op != b.alpha.op))
        {
            return fail("color targets use different blend functions, which OpenGL 3.3 can't do");
        }
    }

    const DepthStencilState& ds = config.depth_stencil;

    if (config.color_targets.empty() && ds.format == Format::Undefined) {
        return fail("no color targets and no depth format");
    }

    if ((ds.depth_test || ds.depth_write || ds.stencil_test) && ds.format == Format::Undefined) {
        return fail("depth or stencil testing is enabled but depth_stencil.format is Undefined");
    }

    if (ds.format != Format::Undefined && !is_depth_format(ds.format)) {
        return fail("depth_stencil.format isn't a depth format");
    }

    if (ds.stencil_test && !has_stencil(ds.format)) {
        return fail("stencil testing is enabled but the depth format has no stencil");
    }

    if (config.bind_group_layouts.size() > MAX_BIND_GROUPS)
        return fail("too many bind group layouts");

    for (std::uint32_t group = 0; group < config.bind_group_layouts.size(); ++group) {
        const GLBindGroupLayout* layout = live_(bind_group_layouts_, config.bind_group_layouts[group]);

        if (!layout) {
            logger_->error("create_pipeline('{}'): bind_group_layouts[{}] is missing or destroyed",
                           config.name, group);
            return {};
        }

        // The group index decides the GL binding points (group * 16 + binding),
        // so driver limits can only be checked here, not when the layout is made.
        for (const BindGroupLayoutEntry& entry : layout->entries) {
            const std::uint32_t point = gl_binding(group, entry.binding);
            std::uint32_t       limit = 0;
            switch (entry.type) {
                case BindType::UniformBuffer:         limit = binding_limits_.uniform_buffers; break;
                case BindType::SampledTexture:        limit = binding_limits_.texture_units;   break;
                case BindType::StorageBuffer:
                case BindType::ReadOnlyStorageBuffer:
                case BindType::StorageTexture:                    // rejected by the layout
                case BindType::Sampler:               continue;   // shares its textures' units
            }

            if (point >= limit) {
                logger_->error("create_pipeline('{}'): group {} binding {} maps to GL binding {}, "
                               "beyond this driver's limit of {} for that resource type",
                               config.name, group, entry.binding, point, limit);
                return {};
            }
        }
    }

    // ---- Program ------------------------------------------------------------
    GLRenderPipeline pipeline;
    pipeline.name    = std::string(config.name);
    pipeline.program = glCreateProgram();

    glAttachShader(pipeline.program, vs->id);
    glAttachShader(pipeline.program, fs->id);
    glLinkProgram(pipeline.program);
    glDetachShader(pipeline.program, vs->id);
    glDetachShader(pipeline.program, fs->id);

    GLint linked = GL_FALSE;
    glGetProgramiv(pipeline.program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint capacity = 0;
        glGetProgramiv(pipeline.program, GL_INFO_LOG_LENGTH, &capacity);
        std::string log(static_cast<std::size_t>(std::max(capacity, 1)), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(pipeline.program, capacity, &written, log.data());
        log.resize(static_cast<std::size_t>(written));
        while (!log.empty() && (log.back() == '\n' || log.back() == ' ')) log.pop_back();

        glDeleteProgram(pipeline.program);
        return fail("linking failed:\n" + log);
    }

    // ---- Bindings -----------------------------------------------------------
    // GLSL 3.30 can't declare binding points, so each uniform block and
    // sampler is found by its layout entry's name and pointed at its GL
    // binding (GLLimits.hpp). Names the program doesn't use (another stage's,
    // or optimized away) are skipped.
    glUseProgram(pipeline.program);
    for (std::uint32_t group = 0; group < config.bind_group_layouts.size(); ++group) {
        const GLBindGroupLayout* layout = live_(bind_group_layouts_, config.bind_group_layouts[group]);

        for (std::size_t i = 0; i < layout->entries.size(); ++i) {
            const BindGroupLayoutEntry& entry = layout->entries[i];
            const char*                 name  = layout->names[i].c_str();
            const GLuint                point = gl_binding(group, entry.binding);

            if (entry.type == BindType::UniformBuffer) {
                const GLuint index = glGetUniformBlockIndex(pipeline.program, name);
                if (index != GL_INVALID_INDEX) glUniformBlockBinding(pipeline.program, index, point);
            } else if (entry.type == BindType::SampledTexture) {
                const GLint location = glGetUniformLocation(pipeline.program, name);
                if (location >= 0) glUniform1i(location, static_cast<GLint>(point));
            }
        }
    }
    glUseProgram(0);

    // ---- Vertex layout ------------------------------------------------------
    // The VAO records which attributes are enabled and their divisors. The
    // formats are kept in the pipeline and pointed at buffers at draw time,
    // since GL 3.3 sets format and buffer in the same call.
    glGenVertexArrays(1, &pipeline.vao);
    glBindVertexArray(pipeline.vao);
    for (std::uint32_t slot = 0; slot < config.vertex_buffers.size(); ++slot) {
        const VertexBufferLayout& layout = config.vertex_buffers[slot];
        const bool per_instance = layout.step_mode == VertexStepMode::Instance;

        GLVertexSlot& out   = pipeline.vertex_slots[slot];
        out.stride          = static_cast<GLsizei>(layout.stride);
        out.per_instance    = per_instance;
        out.first_attribute = static_cast<std::uint32_t>(pipeline.attributes.size());
        out.attribute_count = static_cast<std::uint32_t>(layout.attributes.size());

        for (const VertexAttribute& attribute : layout.attributes) {
            const GLVertexFormat format = to_gl_vertex_format(attribute.format);

            glEnableVertexAttribArray(attribute.location);
            glVertexAttribDivisor(attribute.location, per_instance ? 1 : 0);

            pipeline.attributes.push_back({
                attribute.location,
                format.size,
                format.type,
                format.normalized,
                format.integer,
                attribute.offset
            });
        }
    }
    glBindVertexArray(0);

    pipeline.vertex_buffer_count = static_cast<std::uint32_t>(config.vertex_buffers.size());

    // ---- Fixed-function state -----------------------------------------------
    pipeline.topology = to_gl_primitive_topology(config.primitive.topology);

    GLRenderPipelineState& state = pipeline.state;
    state.raster.cull_enabled = config.primitive.cull_mode != CullMode::None;
    state.raster.cull_face    = to_gl_cull_face(config.primitive.cull_mode);
    state.raster.front_face   = to_gl_front_face(config.primitive.front_face);

    auto stencil_face = [](const StencilOpState& face) {
        return GLStencilFace{
            to_gl_compare_op(face.compare),
            to_gl_stencil_op(face.fail),
            to_gl_stencil_op(face.depth_fail),
            to_gl_stencil_op(face.pass)
        };
    };

    state.depth_stencil.depth_test   = ds.depth_test;
    state.depth_stencil.depth_write  = ds.depth_write ? GL_TRUE : GL_FALSE;
    state.depth_stencil.depth_func   = to_gl_compare_op(ds.depth_compare);
    state.depth_stencil.stencil_test = ds.stencil_test;
    state.depth_stencil.front        = stencil_face(ds.stencil_front);
    state.depth_stencil.back         = stencil_face(ds.stencil_back);
    state.depth_stencil.read_mask    = ds.stencil_read_mask;
    state.depth_stencil.write_mask   = ds.stencil_write_mask;

    state.color_target_count = static_cast<std::uint32_t>(config.color_targets.size());

    for (std::uint32_t i = 0; i < config.color_targets.size(); ++i) {
        const ColorTargetState& target = config.color_targets[i];

        state.blend[i] = GLBlendTarget{
            target.blend.enabled,
            to_gl_blend_factor(target.blend.color.src), to_gl_blend_factor(target.blend.color.dst),
            to_gl_blend_op(target.blend.color.op),
            to_gl_blend_factor(target.blend.alpha.src), to_gl_blend_factor(target.blend.alpha.dst),
            to_gl_blend_op(target.blend.alpha.op),
        };

        auto bit = [&](ColorWriteFlags channel) -> GLboolean {
            return has_any(target.write_mask, channel) ? GL_TRUE : GL_FALSE;
        };
        state.color_mask[i] = GLColorMask{
            bit(ColorWriteFlags::Red),
            bit(ColorWriteFlags::Green),
            bit(ColorWriteFlags::Blue),
            bit(ColorWriteFlags::Alpha)
        };

        pipeline.color_formats[i] = target.format;
    }

    pipeline.depth_format = ds.format;

    for (std::uint32_t i = 0; i < config.bind_group_layouts.size(); ++i) {
        pipeline.bind_group_layouts[i] = config.bind_group_layouts[i];
    }

    pipeline.bind_group_count = static_cast<std::uint32_t>(config.bind_group_layouts.size());

    if (debug_) {
        set_label(GL_PROGRAM,      pipeline.program, pipeline.name);
        set_label(GL_VERTEX_ARRAY, pipeline.vao,     pipeline.name);
    }
    return pipelines_.create(std::move(pipeline)).handle;
}

// =============================================================================
// Bind group layouts
// =============================================================================

BindGroupLayoutHandle GLDevice::create(const BindGroupLayoutConfig& config) {
    GLBindGroupLayout layout;
    layout.name = std::string(config.name);
    layout.entries.assign(config.entries.begin(), config.entries.end());

    std::sort(layout.entries.begin(), layout.entries.end(),
              [](const auto& a, const auto& b) { return a.binding < b.binding; });

    std::uint32_t samplers = 0;
    std::uint32_t textures = 0;

    for (std::size_t i = 0; i < layout.entries.size(); ++i) {
        const BindGroupLayoutEntry& entry = layout.entries[i];

        if (entry.binding >= BINDINGS_PER_GROUP) {
            logger_->error("create_bind_group_layout('{}'): binding {} exceeds the maximum of {}",
                           config.name, entry.binding, BINDINGS_PER_GROUP - 1);
            return {};
        }

        if (i > 0 && layout.entries[i - 1].binding == entry.binding) {
            logger_->error("create_bind_group_layout('{}'): binding {} appears twice",
                           config.name, entry.binding);
            return {};
        }

        if (entry.stage == ShaderStageFlags::None) {
            logger_->error("create_bind_group_layout('{}'): binding {} has no visibility",
                           config.name, entry.binding);
            return {};
        }

        bool is_buffer = false;
        switch (entry.type) {
            case BindType::UniformBuffer:  is_buffer = true; break;
            case BindType::SampledTexture: ++textures;       break;
            case BindType::Sampler:        ++samplers;       break;
            case BindType::StorageBuffer:
            case BindType::ReadOnlyStorageBuffer:
            case BindType::StorageTexture:
                logger_->error("create_bind_group_layout('{}'): binding {} is a storage buffer or "
                               "storage texture, which needs OpenGL 4.3", config.name, entry.binding);
                return {};
        }

        // See BindGroupLayoutEntry::name.
        const bool named = entry.type == BindType::UniformBuffer || entry.type == BindType::SampledTexture;
        if (named && entry.name.empty()) {
            logger_->error("create_bind_group_layout('{}'): binding {} needs a name (its GLSL uniform "
                           "block or sampler name) on OpenGL 3.3", config.name, entry.binding);
            return {};
        }

        if (entry.has_dynamic_offset) {
            if (!is_buffer) {
                logger_->error("create_bind_group_layout('{}'): binding {} has a dynamic offset "
                               "but isn't a buffer", config.name, entry.binding);
                return {};
            }
            ++layout.dynamic_offset_count;
        }
    }

    // See the sampler rule in GLLimits.hpp.
    if (samplers > 1 && textures > 0) {
        logger_->error("create_bind_group_layout('{}'): a group with textures may have at most one "
                       "sampler on OpenGL (it applies to every texture in the group)", config.name);
        return {};
    }

    // Own the names: the config's string_views don't outlive this call.
    for (BindGroupLayoutEntry& entry : layout.entries) {
        layout.names.emplace_back(entry.name);
        entry.name = {};
    }

    return bind_group_layouts_.create(std::move(layout)).handle;
}

// =============================================================================
// Bind groups
// =============================================================================

BindGroupHandle GLDevice::create(const BindGroupConfig& config) {
    const GLBindGroupLayout* layout = live_(bind_group_layouts_, config.layout);
    if (!layout) {
        logger_->error("create_bind_group('{}'): layout is missing or destroyed", config.name);
        return {};
    }

    if (config.entries.size() != layout->entries.size()) {
        logger_->error("create_bind_group('{}'): has {} entries, its layout expects {}",
                       config.name, config.entries.size(), layout->entries.size());
        return {};
    }

    GLBindGroup group;
    group.layout               = config.layout;
    group.dynamic_offset_count = layout->dynamic_offset_count;
    group.name                 = std::string(config.name);

    for (const BindGroupLayoutEntry& expected : layout->entries) {
        const BindGroupEntry* entry = nullptr;

        for (const BindGroupEntry& candidate : config.entries) {
            if (candidate.binding == expected.binding) { entry = &candidate; break; }
        }

        if (!entry) {
            logger_->error("create_bind_group('{}'): no entry for binding {}", config.name, expected.binding);
            return {};
        }

        auto bad = [&](const char* why) -> BindGroupHandle {
            logger_->error("create_bind_group('{}'): binding {}: {}", config.name, expected.binding, why);
            return {};
        };

        switch (expected.type) {
            case BindType::UniformBuffer:
            case BindType::StorageBuffer:
            case BindType::ReadOnlyStorageBuffer: {
                const bool uniform = expected.type == BindType::UniformBuffer;

                const GLBuffer* buffer = live_(buffers_, entry->buffer);
                if (!buffer)
                    return bad("buffer is missing or destroyed");

                if (!has_any(buffer->usage, uniform ? BufferUsageFlags::Uniform : BufferUsageFlags::Storage)) {
                    return bad(uniform ? "buffer lacks BufferUsageFlags::Uniform" : "buffer lacks BufferUsageFlags::Storage");
                }

                // Storage buffers never get here: their layouts are rejected.
                const std::uint32_t alignment = info_.uniform_buffer_offset_alignment;

                if (alignment && entry->offset % alignment != 0) {
                    return bad("offset isn't a multiple of the device's buffer offset alignment");
                }

                if (entry->offset >= buffer->size)
                    return bad("offset is past the end of the buffer");

                const std::uint64_t size = entry->size ? entry->size : buffer->size - entry->offset;

                if (entry->offset + size > buffer->size)
                    return bad("offset + size is past the end of the buffer");

                if (uniform && size > info_.max_uniform_buffer_range) {
                    return bad("size exceeds the device's max_uniform_buffer_range");
                }

                group.entries.push_back({
                    expected.binding,
                    expected.type,
                    expected.has_dynamic_offset,
                    entry->buffer,
                    entry->offset,
                    size,
                    {}
                });
                break;
            }
            case BindType::SampledTexture:
            case BindType::StorageTexture: {
                const bool sampled = expected.type == BindType::SampledTexture;

                const GLTexture* texture = live_(textures_, entry->texture);
                if (!texture)
                    return bad("texture is missing or destroyed");

                if (!has_any(texture->usage, sampled ? TextureUsageFlags::Sampled : TextureUsageFlags::Storage)) {
                    return bad(sampled ? "texture lacks TextureUsage::Sampled" : "texture lacks TextureUsage::Storage");
                }

                GLBindGroupEntry resolved;
                resolved.binding = expected.binding;
                resolved.type    = expected.type;
                resolved.texture = entry->texture;
                group.entries.push_back(resolved);
                break;
            }
            case BindType::Sampler: {
                if (!live_(samplers_, entry->sampler))
                    return bad("sampler is missing or destroyed");
                group.sampler = entry->sampler;
                break;
            }
        }
    }

    return bind_groups_.create(std::move(group)).handle;
}

// =============================================================================
// Remove
// =============================================================================

template <typename T, typename Tag>
T* GLDevice::live_(SlotMap<T, Tag>& pool, Handle<Tag> id) {
    T* resource = pool.get(id);
    return (resource && !resource->pending_destroy) ? resource : nullptr;
}

template <typename T, typename Tag>
void GLDevice::remove_(SlotMap<T, Tag>& pool, Handle<Tag> id, const char* what) {
    if (!id) return;

    T* resource = live_(pool, id);
    if (!resource) {
        logger_->error("destroy: {} handle is stale or already destroyed", what);
        return;
    }

    if (in_frame_) {
        // Commands recorded earlier this frame may still use it during replay.
        resource->pending_destroy = true;
        pending_removes_.emplace_back(id);
    } else {
        remove_now_(id);
    }
}

void GLDevice::remove(BufferHandle handle) {
    remove_(buffers_, handle, "buffer");
}

void GLDevice::remove(SamplerHandle handle) {
    remove_(samplers_, handle, "sampler");
}

void GLDevice::remove(ShaderHandle handle) {
    remove_(shaders_, handle, "shader");
}

void GLDevice::remove(RenderPipelineHandle handle) {
    remove_(pipelines_, handle, "pipeline");
}

void GLDevice::remove(BindGroupLayoutHandle handle) {
    remove_(bind_group_layouts_, handle, "bind group layout");
}

void GLDevice::remove(BindGroupHandle handle) {
    remove_(bind_groups_, handle, "bind group");
}

void GLDevice::remove(TextureHandle handle) {
    if (handle && handle == backbuffer_) {
        logger_->error("destroy: the backbuffer is owned by the device");
        return;
    }
    remove_(textures_, handle, "texture");
}

void GLDevice::remove_now_(BufferHandle handle) {
    if (GLBuffer* buffer = buffers_.get(handle)) {
        glDeleteBuffers(1, &buffer->id);
        buffers_.remove(handle);
    }
}

void GLDevice::remove_now_(TextureHandle handle) {
    if (GLTexture* texture = textures_.get(handle)) {
        framebuffers_.evict(handle);
        glDeleteTextures(1, &texture->id);
        textures_.remove(handle);
    }
}

void GLDevice::remove_now_(SamplerHandle handle) {
    if (GLSampler* sampler = samplers_.get(handle)) {
        glDeleteSamplers(1, &sampler->id);
        samplers_.remove(handle);
    }
}

void GLDevice::remove_now_(ShaderHandle handle) {
    if (GLShader* shader = shaders_.get(handle)) {
        glDeleteShader(shader->id);
        shaders_.remove(handle);
    }
}

void GLDevice::remove_now_(RenderPipelineHandle handle) {
    if (GLRenderPipeline* pipeline = pipelines_.get(handle)) {
        glDeleteProgram(pipeline->program);
        glDeleteVertexArrays(1, &pipeline->vao);
        pipelines_.remove(handle);
    }
}

void GLDevice::remove_now_(BindGroupLayoutHandle handle) {
    bind_group_layouts_.remove(handle); // no GL object
}

void GLDevice::remove_now_(BindGroupHandle handle) {
    bind_groups_.remove(handle); // no GL object
}

void GLDevice::flush_pending_removes_() {
    for (const PendingRemove& pending : pending_removes_) {
        std::visit([this](auto id) { remove_now_(id); }, pending);
    }
    pending_removes_.clear();
}

void GLDevice::report_leaks_() {
    buffers_.for_each_handle([&](BufferHandle, GLBuffer& buffer) {
        logger_->warn("Leaked buffer '{}'", buffer.name);
        glDeleteBuffers(1, &buffer.id);
    });

    samplers_.for_each_handle([&](SamplerHandle, GLSampler& sampler) {
        logger_->warn("Leaked sampler '{}'", sampler.name);
        glDeleteSamplers(1, &sampler.id);
    });

    textures_.for_each_handle([&](TextureHandle handle, GLTexture& texture) {
        if (handle != backbuffer_)
            logger_->warn("Leaked texture '{}'", texture.name);
        glDeleteTextures(1, &texture.id);
    });

    pipelines_.for_each_handle([&](RenderPipelineHandle, GLRenderPipeline& pipeline) {
        logger_->warn("Leaked pipeline '{}'", pipeline.name);
        glDeleteProgram(pipeline.program);
        glDeleteVertexArrays(1, &pipeline.vao);
    });

    shaders_.for_each_handle([&](ShaderHandle, GLShader& shader) {
        logger_->warn("Leaked shader '{}'", shader.name);
        glDeleteShader(shader.id);
    });

    bind_groups_.for_each_handle([&](BindGroupHandle, GLBindGroup& group) {
        logger_->warn("Leaked bind group '{}'", group.name);
    });

    bind_group_layouts_.for_each_handle([&](BindGroupLayoutHandle, GLBindGroupLayout& layout) {
        logger_->warn("Leaked bind group layout '{}'", layout.name);
    });

    bind_groups_.clear();
    bind_group_layouts_.clear();
    buffers_.clear();
    pipelines_.clear();
    samplers_.clear();
    shaders_.clear();
    textures_.clear();
}

// =============================================================================
// Frame loop
// =============================================================================

CommandEncoder& GLDevice::begin_frame() {
    if (in_frame_) {
        logger_->error("begin_frame: previous frame was never ended; discarding it");
        buffer_writes_.clear();
        texture_writes_.clear();
        staging_.clear();
        flush_pending_removes_();
    }
    in_frame_ = true;

    // Follow the window's pixel size. Polling each frame also catches changes
    // that don't produce a resize event, like moving to a display with a
    // different scale. A zero size (minimized) keeps the old backbuffer.
    const Extent2D drawable = context_.drawable_extent();
    if (drawable != backbuffer_extent_ && drawable.width > 0 && drawable.height > 0) {
        recreate_backbuffer_(drawable);
    }

    encoder_.reset();
    return encoder_;
}

void GLDevice::end_frame() {
    if (!in_frame_) {
        logger_->error("end_frame: called without begin_frame");
        return;
    }

    encoder_.finish();
    apply_pending_writes_();       // writes land before any of the frame's commands
    encoder_.execute(*this);

    const Extent2D drawable = context_.drawable_extent();
    if (drawable.width > 0 && drawable.height > 0) {
        present_();
    }

    in_frame_ = false;
    flush_pending_removes_();     // after replay, which may have used them
    ++frame_index_;
}

void GLDevice::apply_pending_writes_() {
    // Writes to resources destroyed and deleted since are skipped: their
    // handles no longer resolve.
    for (const PendingBufferWrite& w : buffer_writes_) {
        if (const GLBuffer* buffer = buffers_.get(w.buffer)) {
            upload_buffer(buffer->id, w.offset, w.size, staging_.data() + w.data_offset);
        }
    }
    for (const PendingTextureWrite& w : texture_writes_) {
        if (const GLTexture* texture = textures_.get(w.texture)) {
            upload_texture_(*texture, w.region, staging_.data() + w.data_offset);
        }
    }

    // clear() keeps capacity, so steady-state frames don't allocate.
    buffer_writes_.clear();
    texture_writes_.clear();
    staging_.clear();
}

void GLDevice::present_() {
    FramebufferKey key;
    key.colors[0]   = backbuffer_;
    key.color_count = 1;

    const GLuint source = framebuffers_.get(key, textures_, *logger_);
    if (!source) return;

    const Extent2D window = context_.drawable_extent();

    // Blits obey the scissor test, which render passes leave enabled. The
    // color mask is opened too, as cheap insurance against a pipeline's mask
    // on draw buffer 0 lingering; the cache is told.
    glDisable(GL_SCISSOR_TEST);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    state_cache_.write_masks_changed();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // 0 = the window's framebuffer
    glBlitFramebuffer(
        0,
        0,
        static_cast<GLint>(backbuffer_extent_.width),
        static_cast<GLint>(backbuffer_extent_.height),
        0,
        0,
        static_cast<GLint>(window.width),
        static_cast<GLint>(window.height),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    context_.swap_buffers();
}

void GLDevice::recreate_backbuffer_(Extent2D extent) {
    if (backbuffer_) remove_now_(backbuffer_);

    backbuffer_ = create({
        .name   = "Backbuffer",
        .format = BACKBUFFER_FORMAT,
        .width  = extent.width,
        .height = extent.height,
        .usage  = TextureUsageFlags::ColorTarget | TextureUsageFlags::CopySrc,
    });
    backbuffer_extent_ = extent;
}

void GLDevice::resize(Extent2D /*extent*/) {
    // Nothing to do: begin_frame() follows the window's pixel size directly.
}

void GLDevice::set_vsync(bool enabled) {
    context_.set_vsync(enabled);
}

void GLDevice::wait_idle() {
    glFinish();
}

} // namespace Nezumi::RHI::GL
