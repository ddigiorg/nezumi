// =============================================================================
// KindResources.hpp
//
// A collection of GPU resources for each kind.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include <cstdint> // uint32_t
#include <string>  // string

namespace Nezumi::Render {

struct KindResources {
    std::string               name;
    RHI::RenderPipelineHandle pipeline     = {};
    RHI::BufferHandle         shape        = {};
    std::uint32_t             vertex_count = 0;
    std::uint32_t             stride       = 0; // sizeof(Instance)

    bool ready() const { return bool(pipeline); }
};

} // namespace Nezumi::Render
