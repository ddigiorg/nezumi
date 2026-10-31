// =============================================================================
// ViewBindings.cpp
// =============================================================================

#include "Nezumi/Render/Core/View/ViewBindings.hpp"
#include "Nezumi/RHI/Buffer/BufferConfig.hpp"
#include <algorithm> // max
#include <array>     // array
#include <cstring>   // memcpy
#include <cassert>   // assert
#include <span>      // span
#include <stdexcept> // runtime_error
#include <vector>    // vector

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

ViewBindings::ViewBindings(RHI::Device& device)
    : device_(device)
{
    // Dynamic offsets must be multiples of the device's alignment.
    const std::uint32_t align = std::max<std::uint32_t>(
        device_.info().uniform_buffer_offset_alignment,
        1
    );

    stride_ = (std::uint32_t(sizeof(Uniforms)) + align - 1) / align * align;

    // setup bind group layout entries
    const std::array<RHI::BindGroupLayoutEntry, 1> entries = {
        RHI::BindGroupLayoutEntry{
            .binding            = 0,
            .type               = RHI::BindType::UniformBuffer,
            .stage              = RHI::ShaderStageFlags::Vertex,
            .has_dynamic_offset = true,
            .name               = "Frame",
        },
    };

    // Setup bind group layout
    layout_ = device_.create(
        RHI::BindGroupLayoutConfig{
            .name = "frame",
            .entries = entries
        }
    );
    if (!layout_) {
        remove_();
        throw std::runtime_error(
            "ViewBindings: bind group layout creation failed (see log)"
        );
    }

    // Setup initial uniform buffer data
    //
    // Every slot starts as identity, so a view drawn before its first update
    // shows clip space instead of garbage.
    std::vector<std::byte> initial(std::size_t(stride_) * MAX_VIEWS);
    const Uniforms identity;
    for (std::uint32_t v = 0; v < MAX_VIEWS; ++v) {
        std::memcpy(
            /*.dst = */initial.data() + std::size_t(v) * stride_,
            /*.src = */&identity,
            /*.n   = */sizeof identity
        );
    }

    // Setup uniform buffer
    buffer_ = device_.create(
        RHI::BufferConfig{
            .name         = "frame uniforms",
            .size         = initial.size(),
            .usage        = RHI::BufferUsageFlags::Uniform |
                            RHI::BufferUsageFlags::CopyDst,
            .initial_data = initial,
        }
    );
    if (!buffer_) {
        remove_();
        throw std::runtime_error(
            "ViewBindings: uniform buffer creation failed (see log)"
        );
    }

    // Setup bind group entries
    //
    // The binding covers ONE slot; the dynamic offset slides it to a view's.
    const std::array<RHI::BindGroupEntry, 1> binds = {
        RHI::BindGroupEntry{
            .binding = 0,
            .buffer  = buffer_,
            .offset  = 0,
            .size    = sizeof(Uniforms)
        },
    };

    // Setup bind group
    group_ = device_.create(
        RHI::BindGroupConfig{
            .name    = "frame",
            .layout  = layout_,
            .entries = binds
        }
    );
    if (!group_) {
        remove_();
        throw std::runtime_error(
            "ViewBindings: bind group creation failed (see log)"
        );
    }
}

ViewBindings::~ViewBindings() {
    remove_();
}

// -----------------------------------------------------------------------------
// Usage
// -----------------------------------------------------------------------------

void ViewBindings::update(ViewID view, const ViewData& data) {
    assert(view < MAX_VIEWS);
    const Uniforms uniforms{ data.view_proj };
    device_.write_buffer(buffer_, std::uint64_t(view) * stride_, std::as_bytes(std::span(&uniforms, 1)));
}

void ViewBindings::bind(RHI::CommandEncoder& encoder, ViewID view) const {
    assert(view < MAX_VIEWS);
    const std::uint32_t offset = std::uint32_t(view) * stride_;
    encoder.set_bind_group(VIEW_GROUP, group_, std::span(&offset, 1));
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

void ViewBindings::remove_() {
    // Group before buffer and layout: it references both.
    device_.remove(group_);
    device_.remove(buffer_);
    device_.remove(layout_);
    group_  = {};
    buffer_ = {};
    layout_ = {};
}

} // namespace Nezumi::Render
