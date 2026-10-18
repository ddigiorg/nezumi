// =============================================================================
// BlendState.hpp
//
// Defines parameters of a newly created pipeline color blend state.
//
// WebGPU: GPUBlendState
// Vulkan: VkPipelineColorBlendStateCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/BlendComponent.hpp"

namespace Nezumi::RHI {

struct BlendState {
    bool           enabled = false;
    BlendComponent color   = {};
    BlendComponent alpha   = {};

    // -------------------------------------------------------------------------
    // Alpha Blend
    //
    // Straight (non-premultiplied) alpha. The usual sprite default.
    // -------------------------------------------------------------------------

    static constexpr BlendState ALPHA_BLEND() noexcept {
        return {
            /*.enabled = */true,
            /*.color   = */{
                /*.src = */BlendFactor::SrcAlpha,
                /*.dst = */BlendFactor::OneMinusSrcAlpha,
                /*.op  = */BlendOp::Add
            },
            /*.alhpa   = */{
                /*.src = */BlendFactor::One,
                /*.dst = */BlendFactor::OneMinusSrcAlpha,
                /*.op  = */BlendOp::Add
            }
        };
    }

    // -------------------------------------------------------------------------
    // Premultiplied Alpha
    //
    // For textures whose color is already multiplied by alpha. Avoids dark
    // fringes around filtered sprite edges.
    // -------------------------------------------------------------------------

    static constexpr BlendState PREMULTIPLIED_ALPHA() noexcept {
        return {
            /*.enabled = */true,
            /*.color   = */{
                /*.src = */BlendFactor::One,
                /*.dst = */BlendFactor::OneMinusSrcAlpha,
                /*.op  = */BlendOp::Add
            },
            /*.alhpa   = */{
                /*.src = */BlendFactor::One,
                /*.dst = */BlendFactor::OneMinusSrcAlpha,
                /*.op  = */BlendOp::Add
            }
        };
    }

    // -------------------------------------------------------------------------
    // Additive
    //
    // Glows, particles, light sprites.
    // -------------------------------------------------------------------------

    static constexpr BlendState ADDITIVE() noexcept {
        return {
            /*.enabled = */true,
            /*.color   = */{
                /*.src = */BlendFactor::SrcAlpha,
                /*.dst = */BlendFactor::One,
                /*.op  = */BlendOp::Add
            },
            /*.alhpa   = */{
                /*.src = */BlendFactor::One,
                /*.dst = */BlendFactor::One,
                /*.op  = */BlendOp::Add
            }
        };
    }

};

} // namespace Nezumi::RHI
