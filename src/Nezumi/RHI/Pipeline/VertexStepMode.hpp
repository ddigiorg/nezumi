// =============================================================================
// VertexStepMode.hpp
//
// Defines whether a vertex attribute is per vertex or per instance.
//
// WebGPU: GPUVertexStepMode
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class VertexStepMode : uint8_t {
    Vertex   = 0, // advance per vertex
    Instance = 1, // advance per instance (e.g. glVertexAttribDivisor = 1)
};

} // namespace Nezumi::RHI
