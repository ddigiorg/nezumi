// =============================================================================
// GLContext.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLContext.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLDebug.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLInfo.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace Nezumi::RHI::GL {

// =============================================================================
// GLContext
// =============================================================================

GLContext::GLContext(SDL_Window* window, Logger* logger, bool debug, bool vsync)
    : window_(window)
    , logger_(logger)
{
    if (!window_) throw std::runtime_error("GLContext: window is null");
    if (!logger_) throw std::runtime_error("GLContext: logger is null");

    if (!(SDL_GetWindowFlags(window_) & SDL_WINDOW_OPENGL)) {
        throw std::runtime_error(
            "GLContext: window lacks SDL_WINDOW_OPENGL; "
            "call RHI::prepare_window() before creating it");
    }

    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
        throw std::runtime_error(
            std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    // From here on the context exists, so any failure must destroy it: the
    // destructor doesn't run for a constructor that throws.
    try {
        // Requires the context to be current, which SDL_GL_CreateContext
        // does. SDL_GL_GetProcAddress works for GLX, EGL, and WGL alike.
        const int version = gladLoadGL(
            reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
        if (!version) {
            throw std::runtime_error("gladLoadGL failed to load OpenGL functions");
        }

        const int major = GLAD_VERSION_MAJOR(version);
        const int minor = GLAD_VERSION_MINOR(version);
        if (major < REQUIRED_MAJOR || (major == REQUIRED_MAJOR && minor < REQUIRED_MINOR)) {
            throw std::runtime_error(
                "OpenGL " + std::to_string(REQUIRED_MAJOR) + "." +
                std::to_string(REQUIRED_MINOR) + " required, driver provides " +
                std::to_string(major) + "." + std::to_string(minor));
        }

        if (debug) {
            if (GLAD_GL_KHR_debug) {
                enable_debug_output(*logger_);
            } else {
                logger_->warn("GL_KHR_debug isn't available; driver debug output is off");
            }
        }

        // Clip-space depth 0..1 instead of GL's -1..1, matching Vulkan and
        // D3D so projection matrices are identical on every backend. Core in
        // 4.5; on older drivers it needs the extension, and without it depth
        // stays -1..1 (DeviceInfo::clip_depth_zero_to_one reports which).
        if (GLAD_GL_ARB_clip_control) {
            glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        } else {
            logger_->warn("GL_ARB_clip_control isn't available; clip-space depth is -1..1");
        }

        set_vsync(vsync);
        log_driver_info(*logger_);
    }
    catch (...) {
        SDL_GL_DestroyContext(context_);
        context_ = nullptr;
        throw;
    }
}

GLContext::~GLContext() {
    if (context_) SDL_GL_DestroyContext(context_);
}

void GLContext::swap_buffers() const {
    SDL_GL_SwapWindow(window_);
}

void GLContext::set_vsync(bool enabled) {
    if (!SDL_GL_SetSwapInterval(enabled ? 1 : 0)) {
        logger_->warn("Could not {} vsync: {}",
                      enabled ? "enable" : "disable", SDL_GetError());
    }
}

Extent2D GLContext::drawable_extent() const {
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    return { static_cast<std::uint32_t>(w > 0 ? w : 0),
             static_cast<std::uint32_t>(h > 0 ? h : 0) };
}

} // namespace Nezumi::RHI::GL
