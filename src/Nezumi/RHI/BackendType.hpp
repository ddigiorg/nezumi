// =============================================================================
// BackendType.hpp
//
// Which graphics API on which a Device is built.
// =============================================================================

#pragma once
#include <cstdint>     // uint8_t
#include <string_view> // string_view

namespace Nezumi::RHI {

enum class BackendType : std::uint8_t {
    OpenGL     = 0,
    Vulkan     = 1,
    Direct3D12 = 2,
};

constexpr std::string_view backend_type_to_string(BackendType backend) noexcept {
    switch (backend) {
        case BackendType::OpenGL:     return "OpenGL";
        case BackendType::Vulkan:     return "Vulkan";
        case BackendType::Direct3D12: return "Direct3D12";
    }
    return "Unknown";
}

} // namespace Nezumi::RHI
