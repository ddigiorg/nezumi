// =============================================================================
// ViewBindings.hpp
//
// Bind group 0: per-frame uniforms, one slot per view (today each slot holds
// that view's camera matrix). Every kind's pipeline lists layout() as its
// group 0 layout, so the binding survives pipeline switches.
//
// All slots live in one uniform buffer, MAX_VIEWS slots spaced `stride()`
// bytes apart (the device's dynamic-offset alignment, often 256). The bind
// group points at ONE slot's worth of bytes; bind(encoder, view) picks the
// slot with a dynamic offset. Shaders don't change: they still read
//
//   layout(std140, binding = 0) uniform Frame { mat4 view_proj; };
//
// Each view writes its own byte range, so there's never more than one write
// to the same bytes per frame (Device::write_buffer's upload rule).
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/View/ViewData.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // mat4

namespace Nezumi::Render {

class ViewBindings {
public:

    // -------------------------------------------------------------------------
    // Statics
    // -------------------------------------------------------------------------

    static constexpr std::uint32_t VIEW_GROUP  = 0;
    static constexpr std::uint32_t MAX_VIEWS   = VIEW_CAPACITY; // one slot per possible ViewID

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Throws if any of the GPU objects can't be created.
    explicit ViewBindings(RHI::Device& device);
    ~ViewBindings();

    // Non-copyable and non-movable
    ViewBindings(const ViewBindings&)            = delete;
    ViewBindings& operator=(const ViewBindings&) = delete;
    ViewBindings(ViewBindings&&)                 = delete;
    ViewBindings& operator=(ViewBindings&&)      = delete;

    // -------------------------------------------------------------------------
    // Usage
    // -------------------------------------------------------------------------

    // Upload one view's uniforms. Once per view per frame (RenderCore does
    // this for every view published with set_view()).
    void update(ViewID view, const ViewData& data);

    // Bind at GROUP with the view's slot selected. Call before the first draw
    // of each view.
    void bind(RHI::CommandEncoder& encoder, ViewID view) const;

    // For pipeline creation (each kind's group 0 layout).
    RHI::BindGroupLayoutHandle layout() const { return layout_; }

    // Bytes between view slots.
    std::uint32_t stride() const { return stride_; }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    struct Uniforms {
        glm::mat4 view_proj = glm::mat4(1.0f);
    };

    void remove_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device&               device_;
    std::uint32_t              stride_ = 0;
    RHI::BindGroupLayoutHandle layout_ = {};
    RHI::BufferHandle          buffer_ = {};
    RHI::BindGroupHandle       group_  = {};
};

} // namespace Nezumi::Render
