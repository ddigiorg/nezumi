// =============================================================================
// ShaderLibrary.hpp
//
// Loads shaders on first request and caches them by path, so several kinds
// can share a vertex shader without loading it twice.
// =============================================================================

#pragma once
#include "Nezumi/RHI/RHI.hpp"
#include <string>        // string
#include <string_view>   // string_view
#include <unordered_map> // unordered_map

namespace Nezumi::Render {

class ShaderLibrary {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit ShaderLibrary(RHI::Device& device) : device_(device) {}
    ~ShaderLibrary();

    // Non-copyable and non-movable
    ShaderLibrary(const ShaderLibrary&)            = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&)                 = delete;
    ShaderLibrary& operator=(ShaderLibrary&&)      = delete;

    // -------------------------------------------------------------------------
    // Usage
    // -------------------------------------------------------------------------

    // Returns a null handle if the file can't be read or doesn't compile
    // (the device logs why).
    RHI::ShaderHandle get(
        std::string_view      path,
        RHI::ShaderStageFlags stage,
        RHI::ShaderFormat     format = RHI::ShaderFormat::Glsl
    );

    void clear();

private:

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device& device_;
    std::unordered_map<std::string, RHI::ShaderHandle> shaders_;
};

} // namespace Nezumi::Render
