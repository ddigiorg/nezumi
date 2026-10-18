// =============================================================================
// GLBindGroup.hpp
//
// The OpenGL backend's storage for an RHI bind group: its resources, already
// validated against the layout, ready to bind with glBindBufferRange,
// glBindTextureUnit, and friends (see gl_binding in GLLimits.hpp).
//
// Resources are held by handle, so a bind group that outlives one of its
// resources is caught when bound rather than binding a deleted GL object.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLBindGroupEntry.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupHandle.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include <cstdint> // uint32_t, uint_64_t
#include <string>  // string
#include <vector>  // vector

namespace Nezumi::RHI::GL {

struct GLBindGroup {
    BindGroupLayoutHandle         layout  = {};
    std::vector<GLBindGroupEntry> entries;          // sorted by binding; samplers excluded
    SamplerHandle                 sampler = {};     // applied to every texture in the group
    std::uint32_t                 dynamic_offset_count = 0;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLBindGroupPool = SlotMap<GLBindGroup, BindGroupTag>;

} // namespace Nezumi::RHI::GL
