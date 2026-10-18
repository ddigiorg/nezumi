// =============================================================================
// BasePath.hpp
// =============================================================================

#pragma once
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <string_view>

namespace Nezumi::BasePath {

// TODO: might not be appropriate to put in Core/ since it requires SDL

// Returns <executable directory>/assets/<relative>. An absolute path is
// returned unchanged, so callers can still load from anywhere on disk.
inline std::filesystem::path get(std::string_view relative) {
    static const std::filesystem::path root = [] {
        // SDL3 owns this string; do not free it
        const char* base = SDL_GetBasePath();
        return std::filesystem::path(base ? base : "");
    }();
    return root / relative;
}

} // namespace Nezumi::BasePath
