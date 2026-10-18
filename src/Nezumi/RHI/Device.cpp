// =============================================================================
// Device.cpp
// =============================================================================

#include "Nezumi/RHI/Device.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLDevice.hpp"
#include <exception>

namespace Nezumi::RHI {

std::unique_ptr<Device> make_device(
    SDL_Window* window,
    Logger* logger,
    const DeviceConfig& config
) {
    // Without a logger there's nowhere to report problems; see DeviceConfig.
    if (!logger) return nullptr;

    try {
        switch (config.backend) {
            case BackendType::OpenGL:
                return std::make_unique<GL::GLDevice>(window, logger, config);

            case BackendType::Vulkan:
            case BackendType::Direct3D12:
                logger->error(
                    "The {} backend is not available in this build",
                    backend_type_to_string(config.backend)
                );
                return nullptr;
        }
    }
    catch (const std::exception& e) {
        logger->error(
            "Failed to create {} device: {}",
            backend_type_to_string(config.backend),
            e.what()
        );
    }
    return nullptr;
}

} // namespace Nezumi::RHI
