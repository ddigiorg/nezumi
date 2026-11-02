// =============================================================================
// PassContext.hpp
//
// What a feature knows about the pass it is recording into: which views and
// step the recipe gave it, and the target's format and size. Pipelines are
// created per color format, so a feature drawing into several targets picks
// the pipeline that matches `format`.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/RenderTarget.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/RHI/Common/Extent2D.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view

namespace Nezumi::Render {

struct PassContext {
    std::string_view name   = "";
    ViewMask         views  = ALL_VIEWS;               // PassFeature::views
    std::uint32_t    step   = 0;                       // PassFeature::step
    TargetID         target = Target::Backbuffer;
    RHI::Format      format = RHI::Format::Undefined;  // the target's color format
    RHI::Extent2D    extent = {};                      // the target's size in pixels
};

} // namespace Nezumi::Render
