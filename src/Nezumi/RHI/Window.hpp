// =============================================================================
// Window.hpp
//
// The `prepare_window` func must be called BEFORE the SDL window is created.
//
// Each API needs something from the window at creation time: OpenGL needs
// SDL_WINDOW_OPENGL plus context attributes (version, profile, debug) set via
// SDL_GL_SetAttribute; Vulkan needs SDL_WINDOW_VULKAN. This sets whatever
// global SDL state the backend requires and returns SDL_WindowFlags to OR
// into the flags passed to SDL_CreateWindow.
//
// Returns a plain integer so this header doesn't include SDL.
// =============================================================================

#pragma once
#include "Nezumi/RHI/BackendType.hpp"
#include <cstdint> // uint64_t

namespace Nezumi::RHI {

std::uint64_t prepare_sdl_window(BackendType backend, bool debug);

} // namespace Nezumi::RHI
