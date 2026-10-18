// =============================================================================
// Window.cpp
// =============================================================================

#include "Nezumi/RHI/BackendType.hpp"

#include "Nezumi/RHI/Backends/OpenGL/GLWindow.hpp"

namespace Nezumi::RHI {

std::uint64_t prepare_sdl_window(BackendType backend, bool debug) {
    switch (backend) {
        case BackendType::OpenGL:     return GL::prepare_sdl_window(debug);
        case BackendType::Vulkan:     return 0;   // not implemented yet
        case BackendType::Direct3D12: return 0;   // not implemented yet
    }
    return 0;
}

} // namespace Nezumi::RHI
