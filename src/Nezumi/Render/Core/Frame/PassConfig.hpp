// =============================================================================
// PassConfig.hpp
//
// One render pass of a FrameRecipe: what it draws into, whether it clears
// first, and which features record into it (and for which views).
//
// A pass draws into the backbuffer (the default) or an offscreen target made
// with RenderCore::create_target (see RenderTarget.hpp). The viewport covers
// the whole target.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/RenderTarget.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/RHI/Common/Color.hpp"
#include <cstdint> // uint32_t
#include <string>  // string
#include <vector>  // vector

namespace Nezumi::Render {

class IRenderFeature;

// A feature to run in a pass, limited to some views.
struct PassFeature {
    IRenderFeature* feature = nullptr;   // not owned; must be registered with RenderCore
    ViewMask        views   = ALL_VIEWS; // views this feature records in this pass

    // Feature-defined: which of its steps to record here, for features that
    // run in several passes (PostProcess's composite, bloom and tonemap).
    // Renderer2D ignores it.
    std::uint32_t   step    = 0;
};

struct PassConfig {
    std::string name = "pass";

    // What to draw into: the backbuffer, or a RenderCore target
    TargetID target = Target::Backbuffer;

    // Clear the target to clear_color first (Clear), or keep what earlier
    // passes drew (Load).
    bool       clear       = true;
    RHI::Color clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Recorded in this order.
    std::vector<PassFeature> features;
};

} // namespace Nezumi::Render
