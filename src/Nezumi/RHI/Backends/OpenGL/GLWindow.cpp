// =============================================================================
// GLWindow.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLWindow.hpp"

#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"

#include <glad/gl.h>
#include <SDL3/SDL.h>

namespace Nezumi::RHI::GL {

std::uint64_t prepare_sdl_window(bool debug) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, REQUIRED_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, REQUIRED_MINOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);

    // macOS only creates core profiles 3.2+ as forward-compatible contexts.
    int flags = debug ? SDL_GL_CONTEXT_DEBUG_FLAG : 0;
#ifdef __APPLE__
    flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,         flags);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,          1);

    // The window's own framebuffer only ever receives the final image, copied
    // from the RHI's backbuffer texture. Depth and stencil are RHI textures,
    // so the window doesn't need its own.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    return SDL_WINDOW_OPENGL;
}

} // namespace Nezumi::RHI::GL
