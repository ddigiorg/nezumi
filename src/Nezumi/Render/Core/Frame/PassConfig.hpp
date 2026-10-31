// =============================================================================
// PassConfig.hpp
//
// One render pass of a FrameRecipe: what it draws into, whether it clears
// first, and which features record into it (and for which views).
//
// Today every pass draws into the backbuffer. Offscreen color targets and a
// depth attachment are the next additions (both already exist in the RHI).
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/RHI/Common/Color.hpp"
#include <string> // string
#include <vector> // vector

namespace Nezumi::Render {

class IRenderFeature;

// A feature to run in a pass, limited to some views.
struct PassFeature {
    IRenderFeature* feature = nullptr;   // not owned; must be registered with RenderCore
    ViewMask        views   = ALL_VIEWS; // views this feature records in this pass
};

struct PassConfig {
    std::string name = "pass";

    // Clear the backbuffer to clear_color first (Clear), or keep what earlier
    // passes drew (Load).
    bool       clear       = true;
    RHI::Color clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Recorded in this order.
    std::vector<PassFeature> features;
};

} // namespace Nezumi::Render
