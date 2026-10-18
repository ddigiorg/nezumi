// =============================================================================
// GLSampler.hpp
//
// The OpenGL backend's storage for an RHI sampler: a GL sampler object,
// which overrides the filtering and wrapping state of whatever texture is
// bound alongside it.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include <glad/gl.h>
#include <string>

namespace Nezumi::RHI::GL {

struct GLSampler {
    GLuint id = 0;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLSamplerPool = SlotMap<GLSampler, SamplerTag>;

} // namespace Nezumi::RHI::GL
