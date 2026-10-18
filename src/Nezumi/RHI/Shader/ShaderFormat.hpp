// =============================================================================
// ShaderFormat.hpp
//
// What the bytes in ShaderConfig::code are. SPIR-V is the long-term
// canonical format; GLSL lets the OpenGL backend run existing shaders before
// the offline compile pipeline exists. A backend rejects formats it can't use.
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class ShaderFormat : uint8_t {
    Spirv = 0,
    Glsl  = 1,
};

} // namespace Nezumi::RHI
