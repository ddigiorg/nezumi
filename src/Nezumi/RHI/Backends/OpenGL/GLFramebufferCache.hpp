// =============================================================================
// GLFramebufferCache.hpp
//
// OpenGL renders into framebuffer objects (FBOs), each a fixed combination
// of attached textures. The RHI has no framebuffer objects: a render pass
// just names its attachments. This cache creates an FBO the first time a
// combination is used and reuses it after that.
//
// Entries referencing a texture must be evicted when that texture is
// destroyed, or the FBO would point at a deleted texture.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLTexture.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <unordered_map>

namespace Nezumi {
    class Logger;
}

namespace Nezumi::RHI::GL {

// -----------------------------------------------------------------------------
// FramebufferKey
// -----------------------------------------------------------------------------

struct FramebufferKey {
    std::array<TextureHandle, MAX_COLOR_ATTACHMENTS> colors = {};
    std::uint32_t color_count = 0;
    TextureHandle depth = {};   // null = none

    bool operator==(const FramebufferKey&) const noexcept = default;
};

struct FramebufferKeyHash {
    std::size_t operator()(const FramebufferKey& key) const noexcept;
};

// -----------------------------------------------------------------------------
// GLFramebufferCache
//
// Requires a current context for every call, including the destructor, so
// it must be destroyed before the GLContext.
// -----------------------------------------------------------------------------

class GLFramebufferCache {
public:
    GLFramebufferCache() = default;
    ~GLFramebufferCache();

    GLFramebufferCache(const GLFramebufferCache&)            = delete;
    GLFramebufferCache& operator=(const GLFramebufferCache&) = delete;

    // The FBO for these attachments, created if needed. Returns 0 (and logs
    // why) if an attachment is invalid or the combination is incomplete.
    GLuint get(
        const FramebufferKey&  key,
        const GLTexturePool&   textures,
        Logger& logger);

    // Deletes every cached FBO that references `texture`.
    void evict(TextureHandle texture);

    // Deletes every cached FBO.
    void clear();

private:
    GLuint create_(
        const FramebufferKey& key,
        const GLTexturePool&  textures,
        Logger& logger) const;

    std::unordered_map<FramebufferKey, GLuint, FramebufferKeyHash> fbos_;
};

} // namespace Nezumi::RHI::GL
