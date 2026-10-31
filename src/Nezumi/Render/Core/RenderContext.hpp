// =============================================================================
// RenderContext.hpp
// =============================================================================

#pragma once

namespace Nezumi::RHI {
    class Device;
} // namespace RHI

namespace Nezumi::Render {

class BatchBuilder;
class CommandQueue;
class ViewBindings;
class KindRegistry;
class MaterialTable;
struct Renderer2DConfig;
class ShaderLibrary;
class StreamBuffer;

struct RenderContext {
    RHI::Device*    device    = nullptr;
    Renderer2DConfig* config    = nullptr;
    ViewBindings*  frame     = nullptr;
    MaterialTable*  materials = nullptr;
    ShaderLibrary*  shaders   = nullptr;
    KindRegistry*   kinds     = nullptr;
    StreamBuffer*   stream    = nullptr;
    CommandQueue*   queue     = nullptr;
    BatchBuilder*   builder   = nullptr;
};

} // namespace Nezumi::Render
