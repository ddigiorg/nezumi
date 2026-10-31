// =============================================================================
// IRenderFeature.hpp
//
// A feature renderer: something that turns draw requests into GPU commands
// (Renderer2D today; meshes, particles, ... later). RenderCore owns the frame
// and calls every registered feature through these hooks, in this order:
//
//   RenderCore::begin_frame()
//     begin_frame(core)            reset per-frame state; the app draws after
//   RenderCore::end_frame()
//     prepare(core)                CPU work: sort, batch, fill stream mirrors.
//                                  Runs BEFORE Device::begin_frame (which may
//                                  block on an older frame), so it overlaps
//                                  with the GPU.
//     -- Device::begin_frame --
//     upload(core)                 publish views (core.set_view) and write
//                                  any buffers of its own
//     -- RenderCore uploads views and flushes the stream --
//     record(core, encoder, views) once per pass that lists this feature, with
//                                  that pass's view mask; inside the pass
//     -- Device::end_frame --
//     finish(core)                 frame statistics
//
// A feature never begins or ends a render pass itself: the FrameRecipe decides
// which passes exist, what they draw into, and which features run in each.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include <string_view> // string_view

namespace Nezumi::RHI {
    class CommandEncoder;
} // namespace Nezumi::RHI

namespace Nezumi::Render {

class RenderCore;

class IRenderFeature {
public:
    virtual ~IRenderFeature() = default;

    virtual std::string_view name() const = 0;

    virtual void begin_frame(RenderCore& /*core*/) {}
    virtual void prepare    (RenderCore& /*core*/) {}
    virtual void upload     (RenderCore& /*core*/) {}

    // Record this feature's draws for the views in `views` into the current
    // pass. State bound here doesn't carry over to the next feature.
    virtual void record(
        RenderCore&          core,
        RHI::CommandEncoder& encoder,
        ViewMask             views) = 0;

    virtual void finish(RenderCore& /*core*/) {}
};

} // namespace Nezumi::Render
