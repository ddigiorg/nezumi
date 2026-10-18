// =============================================================================
// GLWindow
//
// Sets the SDL OpenGL attributes (version, profile, buffers, debug flag) and
// returns the SDL_WindowFlags an OpenGL window needs. Must be called after
// SDL_Init and before SDL_CreateWindow. Called through RHI::prepare_window.
// =============================================================================

#pragma once
#include <cstdint> // uint64_t

namespace Nezumi::RHI::GL {

std::uint64_t prepare_sdl_window(bool debug);

} // namespace Nezumi::RHI::GL
