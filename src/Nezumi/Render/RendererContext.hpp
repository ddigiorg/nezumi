// =============================================================================
// RendererContext.hpp
// =============================================================================

#pragma once

namespace Nezumi::RHI {
    class Device;
} // namespace RHI

namespace Nezumi::Render {

class BatchBuilder;
class CommandQueue;
class FrameBindings;
class KindRegistry;
class MaterialTable;
class RendererConfig;
class ShaderLibrary;
class StreamBuffer;

struct RendererContext {
    RHI::Device*    device    = nullptr;
    RendererConfig* config    = nullptr;
    FrameBindings*  frame     = nullptr;
    MaterialTable*  materials = nullptr;
    ShaderLibrary*  shaders   = nullptr;
    KindRegistry*   kinds     = nullptr;
    StreamBuffer*   stream    = nullptr;
    CommandQueue*   queue     = nullptr;
    BatchBuilder*   builder   = nullptr;
};

} // namespace Nezumi::Render
