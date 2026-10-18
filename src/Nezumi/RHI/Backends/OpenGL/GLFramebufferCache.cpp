// =============================================================================
// GLFramebufferCache.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLFramebufferCache.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <functional>

namespace Nezumi::RHI::GL {

// -----------------------------------------------------------------------------
// FramebufferKeyHash
// -----------------------------------------------------------------------------

std::size_t FramebufferKeyHash::operator()(const FramebufferKey& key) const noexcept {
    std::size_t h = key.color_count;
    auto mix = [&h](std::uint64_t value) {
        h ^= std::hash<std::uint64_t>{}(value) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    };
    for (std::uint32_t i = 0; i < key.color_count; ++i) mix(key.colors[i].raw());
    mix(key.depth.raw());
    return h;
}

// -----------------------------------------------------------------------------
// GLFramebufferCache
// -----------------------------------------------------------------------------

GLFramebufferCache::~GLFramebufferCache() {
    clear();
}

GLuint GLFramebufferCache::get(
    const FramebufferKey& key,
    const GLTexturePool&  textures,
    Logger& logger
) {
    if (auto it = fbos_.find(key); it != fbos_.end()) return it->second;

    const GLuint fbo = create_(key, textures, logger);
    if (fbo) fbos_.emplace(key, fbo);
    return fbo;
}

void GLFramebufferCache::evict(TextureHandle texture) {
    for (auto it = fbos_.begin(); it != fbos_.end();) {
        const FramebufferKey& key = it->first;

        bool uses = key.depth == texture;
        for (std::uint32_t i = 0; i < key.color_count && !uses; ++i) {
            uses = key.colors[i] == texture;
        }

        if (uses) {
            glDeleteFramebuffers(1, &it->second);
            it = fbos_.erase(it);
        } else {
            ++it;
        }
    }
}

void GLFramebufferCache::clear() {
    for (auto& [key, fbo] : fbos_) glDeleteFramebuffers(1, &fbo);
    fbos_.clear();
}

GLuint GLFramebufferCache::create_(
    const FramebufferKey& key,
    const GLTexturePool&  textures,
    Logger& logger
) const {
    GLuint fbo = 0;
    glCreateFramebuffers(1, &fbo);

    auto fail = [&](const char* why) -> GLuint {
        logger.error("Framebuffer creation failed: {}", why);
        glDeleteFramebuffers(1, &fbo);
        return 0;
    };

    // Color attachments
    GLenum draw_buffers[MAX_COLOR_ATTACHMENTS] = {};
    for (std::uint32_t i = 0; i < key.color_count; ++i) {
        const GLTexture* texture = textures.get(key.colors[i]);
        if (!texture) return fail("color attachment is not a live texture");

        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0 + i, texture->id, 0);
        draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    if (key.color_count > 0) {
        glNamedFramebufferDrawBuffers(fbo, static_cast<GLsizei>(key.color_count), draw_buffers);
    } else {
        glNamedFramebufferDrawBuffer(fbo, GL_NONE);
    }

    // Depth / stencil attachment
    if (key.depth) {
        const GLTexture* texture = textures.get(key.depth);
        if (!texture) return fail("depth attachment is not a live texture");

        const GLenum point = has_stencil(texture->format) ? GL_DEPTH_STENCIL_ATTACHMENT
                                                          : GL_DEPTH_ATTACHMENT;
        glNamedFramebufferTexture(fbo, point, texture->id, 0);
    }

    if (glCheckNamedFramebufferStatus(fbo, GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return fail("attachments form an incomplete framebuffer "
                    "(mismatched sizes, or a format that can't be rendered to)");
    }

    return fbo;
}

} // namespace Nezumi::RHI::GL
