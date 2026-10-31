// =============================================================================
// DrawBatch.hpp
//
// A contiguous range of instances that share bindings, a pipeline, and a single
// draw call.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindID.hpp"
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include <cstdint> // uint8_t, uint32_t, uint64_t

namespace Nezumi::Render {

struct DrawBatch {
    std::uint8_t      view           = 0;
    KindID            kind           = 0;
    MaterialID        material       = Material::None;
    RHI::BufferHandle buffer         = {}; // a StreamBuffer chunk
    std::uint64_t     byte_offset    = 0;  // where this batch's objects start
    std::uint32_t     instance_count = 0;
};

} // namespace Nezumi::Render
