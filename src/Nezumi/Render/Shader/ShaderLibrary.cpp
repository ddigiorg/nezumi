// =============================================================================
// ShaderLibrary.cpp
// =============================================================================

#include "Nezumi/Render/Shader/ShaderLibrary.hpp"
#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/Core/ReadFile.hpp"
#include <cstddef>   // byte
#include <exception> // exception

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

ShaderLibrary::~ShaderLibrary() {
    clear();
}

// -----------------------------------------------------------------------------
// Usage
// -----------------------------------------------------------------------------

RHI::ShaderHandle ShaderLibrary::get(
    std::string_view      path,
    RHI::ShaderStageFlags stage,
    RHI::ShaderFormat     format)
{
    std::string key(path);

    if (const auto it = shaders_.find(key); it != shaders_.end()) {
        return it->second;
    }

    const std::filesystem::path resolved_path = BasePath::get(key);

    std::vector<std::byte> source;
    try {
        source = read_file(resolved_path.string());
    } catch (const std::exception&) {
        // TODO: log error
        // logger_.error("ShaderLibrary: cannot read '" + resolved.string() + "': " + e.what());
        return {};
    }

    const RHI::ShaderHandle shader = device_.create(
        RHI::ShaderConfig{
            .name   = key,
            .stage  = stage,
            .format = format,
            .code   = source,
        }
    );

    if (!shader) {
        // TODO: log error
        // logger_.error("ShaderLibrary: failed to create shader '" + key + "'");
        return {};
    }

    // Cache only successes, so a fixed file can be retried.
    if (shader) shaders_.emplace(std::move(key), shader);
    return shader;
}

void ShaderLibrary::clear() {
    for (const auto& [path, shader] : shaders_) {
        device_.remove(shader);
    }
    shaders_.clear();
}

} // namespace Nezumi::Render
