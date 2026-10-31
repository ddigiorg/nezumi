// =============================================================================
// MaterialTable.cpp
//
// Every texture / sampler call the renderer makes into the RHI is here.
// =============================================================================

#include "Nezumi/Render/Core/Material/MaterialTable.hpp"

#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include <array>     // array
#include <cassert>   // assert
#include <stdexcept> // runtime_error
#include <string>    // string

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

MaterialTable::MaterialTable(RHI::Device& device)
    : device_(device)
{
    // Setup material bind group layout entries
    // binding 0: texture, binding 1: sampler (fragment stage).
    const std::array<RHI::BindGroupLayoutEntry, 2> entries = {
        RHI::BindGroupLayoutEntry{
            .binding = 0,
            .type    = RHI::BindType::SampledTexture,
            .stage   = RHI::ShaderStageFlags::Fragment,
            .name    = "u_texture",
        },
        RHI::BindGroupLayoutEntry{
            .binding = 1,
            .type    = RHI::BindType::Sampler,
            .stage   = RHI::ShaderStageFlags::Fragment,
        },
    };

    // Setup material bind group layout
    layout_ = device_.create(RHI::BindGroupLayoutConfig{
        .name    = "material",
        .entries = entries
    });
    if (!layout_) {
        throw std::runtime_error(
            "MaterialTable: bind group layout failed (see log)"
        );
    }
}

MaterialTable::~MaterialTable() {
    // Owners should have removed their materials already; drop any left over
    // so the device doesn't report them as leaks. Their textures and samplers
    // are the owners' to clean up.
    for (Slot& slot : slots_) device_.remove(slot.group);
    device_.remove(layout_);
}

// -----------------------------------------------------------------------------
// Create
// -----------------------------------------------------------------------------

MaterialID MaterialTable::create(
    std::string_view   name,
    RHI::TextureHandle texture,
    RHI::SamplerHandle sampler)
{
    const std::string label(name);

    if (!texture) {
        throw std::runtime_error(
            "MaterialTable: material '" + label + "' needs a texture");
    }

    if (!sampler) {
        throw std::runtime_error(
            "MaterialTable: material '" + label + "' needs a sampler");
    }

    // Check for a free id first, so running out never leaves a bind group behind.
    if (free_.empty() && slots_.size() >= 0xFFFF) {
        throw std::runtime_error(
            "MaterialTable: out of material ids (65535 live)"
        );
    }

    // Setup bind group entries
    // binding 0: texture, binding 1: sampler (fragment stage).
    const std::array<RHI::BindGroupEntry, 2> entries = {
        RHI::BindGroupEntry{
            .binding = 0,
            .texture = texture
        },
        RHI::BindGroupEntry{
            .binding = 1,
            .sampler = sampler
        },
    };

    // Setup bind group
    const RHI::BindGroupHandle group = device_.create(
        RHI::BindGroupConfig{
            .name = label,
            .layout = layout_,
            .entries = entries
        }
    );
    if (!group) {
        throw std::runtime_error(
            "MaterialTable: bind group for material '" + label + "' failed (see log)"
        );
    }

    MaterialID id;
    if (!free_.empty()) {
        id = free_.back();
        free_.pop_back();
    } else {
        slots_.emplace_back();
        id = static_cast<MaterialID>(slots_.size()); // ids start at 1
    }

    slots_[id - 1] = Slot{
        .group   = group,
        .texture = texture,
        .sampler = sampler,
    };

    return id;
}

// -----------------------------------------------------------------------------
// Remove
// -----------------------------------------------------------------------------

void MaterialTable::remove(MaterialID id) {
    if (!is_live(id)) return;
    device_.remove(slots_[id - 1].group);
    slots_[id - 1] = {};
    free_.push_back(id);
}

// -----------------------------------------------------------------------------
// Bind
// -----------------------------------------------------------------------------

void MaterialTable::bind(RHI::CommandEncoder& encoder, MaterialID id) const {
    assert(is_live(id) && "MaterialTable::bind: dead or null material");
    encoder.set_bind_group(MATERIAL_GROUP, slots_[id - 1].group);
}

} // namespace Nezumi::Render
