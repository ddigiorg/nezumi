// =============================================================================
// GLContext.hpp
//
// Owns the OpenGL context for an SDL window: creation, function loading
// (glad), vsync, and presenting. Part of the OpenGL backend; nothing outside
// RHI/Backends/OpenGL includes this.
//
// Requires OpenGL 4.5 core, for direct state access and glClipControl.
// Driver queries live in GLInfo; debug output in GLDebug.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Extent2D.hpp"
struct SDL_Window;
struct SDL_GLContextState;

namespace Nezumi {
    class Logger;
}

namespace Nezumi::RHI::GL {

// -----------------------------------------------------------------------------
// GLContext
//
// Throws std::runtime_error if the context can't be created or the driver is
// older than OpenGL 4.5. Must be destroyed before the window it was made for.
// -----------------------------------------------------------------------------

class GLContext {
public:
    explicit GLContext(
        SDL_Window* window = nullptr, // created with GLWindow prepare_window()'s flags
        Logger*     logger = nullptr, // required; receives driver debug output
        bool        debug  = false,   // enable GL_KHR_debug output
        bool        vsync  = true
    );

    ~GLContext();

    GLContext(const GLContext&)            = delete;
    GLContext& operator=(const GLContext&) = delete;
    GLContext(GLContext&&)                 = delete;
    GLContext& operator=(GLContext&&)      = delete;

    // Present the window's default framebuffer.
    void swap_buffers() const;

    void set_vsync(bool enabled);

    // The window's size in pixels (not screen coordinates, which differ on
    // high-DPI displays).
    Extent2D drawable_extent() const;

private:
    SDL_Window*         window_  = nullptr;
    SDL_GLContextState* context_ = nullptr;
    Logger*             logger_  = nullptr;
};

} // namespace Nezumi::RHI::GL
