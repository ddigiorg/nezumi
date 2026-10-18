// =============================================================================
// WindowPrepareFunc.hpp
//
// Used to prepare SDL window based on the RHI configuration.
//
// Called after SDL_Init, before SDL_CreateWindow.
// =============================================================================

#pragma once
#include <cstdint>    // uint64_t
#include <functional> // function

namespace Nezumi {

using WindowPrepareFunc = std::function<std::uint64_t()>;

} // namespace Nezumi
