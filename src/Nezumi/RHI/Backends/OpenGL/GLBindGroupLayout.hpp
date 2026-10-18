// =============================================================================
// GLBindGroupLayout.hpp
//
// The OpenGL backend's storage for an RHI bind group layout. GL has no
// equivalent object, so this is only the validated description, used to
// check bind groups and pipelines against each other.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutEntry.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include <cstdint> // uint32_t
#include <string>  // string
#include <vector>  // vector

namespace Nezumi::RHI::GL {

struct GLBindGroupLayout {
    std::vector<BindGroupLayoutEntry> entries; // sorted by binding
    std::uint32_t                     dynamic_offset_count = 0;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLBindGroupLayoutPool = SlotMap<GLBindGroupLayout, BindGroupLayoutTag>;

} // namespace Nezumi::RHI::GL
