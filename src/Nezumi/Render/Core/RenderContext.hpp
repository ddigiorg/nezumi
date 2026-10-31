// =============================================================================
// RenderContext.hpp
//
// The shared GPU services a render resource or pipeline needs while it is
// created: the device, the view and material layouts, and the shader cache.
// RenderCore owns them all and hands this out (RenderCore::context()).
//
// Used by Atlas, Font, and every kind's make_pipeline(). It holds nothing
// feature-specific, so a 3D renderer can use the same context.
// =============================================================================

#pragma once

namespace Nezumi::RHI {
    class Device;
} // namespace RHI

namespace Nezumi::Render {

class MaterialTable;
class ShaderLibrary;
class StreamBuffer;
class ViewBindings;

struct RenderContext {
    RHI::Device*   device    = nullptr;
    ViewBindings*  views     = nullptr; // bind group 0 (per-view uniforms)
    MaterialTable* materials = nullptr; // bind group 1 (texture + sampler)
    ShaderLibrary* shaders   = nullptr;
    StreamBuffer*  stream    = nullptr; // per-frame instance data
};

} // namespace Nezumi::Render
