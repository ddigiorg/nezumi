// =============================================================================
// PostProcessConfig.hpp
//
// PostProcessConfig: what PostProcess reads and how big its bloom chain is,
// fixed at construction. PostSettings: the look, changed any frame.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/RenderTarget.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include <cstdint>     // uint8_t, uint32_t
#include <glm/glm.hpp> // vec3

namespace Nezumi::Render {

// How the lit image's HDR range is brought into the screen's 0..1
enum class Tonemap : std::uint8_t {
    Clamp,    // none: anything over 1 clips
    Shoulder, // identity up to `shoulder`, then rolls off smoothly toward 1:
              // daylight art keeps its exact colors, bright lights burn to white
    ACES,     // filmic (Narkowicz's fit): more contrast, darker midtones
};

// What the final image shows, for tuning lights
enum class PostDebugView : std::uint8_t {
    None,  // the lit, bloomed, tonemapped scene
    Light, // the light alone: ambient + every light, tonemapped
    Bloom, // the bloom alone
};

struct PostProcessConfig {
    // Required: the target the world was drawn into (any format; colors are
    // treated as sRGB-encoded, like the art)
    TargetID scene = Target::Backbuffer;

    // The light target (LINEAR light, added up by PointLights and light
    // quads), or Target::Backbuffer for none: the scene is then lit by the
    // ambient color alone.
    TargetID light = Target::Backbuffer;

    // Format of the lit target the composite writes, emissive draws add to,
    // and bloom reads. Float, so light can go over 1.
    RHI::Format lit_format = RHI::Format::RGBA16Float;

    // Bloom chain length: each level is half the size of the one before,
    // from half the lit target down. 6 levels at 1080p reach 30 x 17 pixels,
    // which spreads the glow wide. 0 = no bloom passes at all.
    std::uint32_t bloom_levels = 6;
};

struct PostSettings {
    // Lighting: lit = scene * (ambient + light), all linear
    bool      lighting = true;                 // false: the scene unchanged
    glm::vec3 ambient  = { 1.0f, 1.0f, 1.0f }; // day = 1; a dark night is ~0.02-0.05

    // Exposure, applied before the tonemap
    float exposure = 1.0f;

    // Bloom: what's brighter than `threshold` (with a soft `knee` below it)
    // is blurred wide and added back
    bool  bloom           = true;
    float bloom_threshold = 1.0f;
    float bloom_knee      = 0.5f;
    float bloom_intensity = 1.0f;
    float bloom_radius    = 1.0f; // upsample filter radius, in source texels

    Tonemap tonemap  = Tonemap::Shoulder;
    float   shoulder = 0.8f; // Tonemap::Shoulder: linear up to here

    PostDebugView debug = PostDebugView::None;
};

} // namespace Nezumi::Render
