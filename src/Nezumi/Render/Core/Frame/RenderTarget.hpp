// =============================================================================
// RenderTarget.hpp
//
// An offscreen color target that passes can draw into and later passes (or
// features) can sample. RenderCore owns them and keeps each one sized to a
// fraction of the backbuffer, so they follow window resizes:
//
//   const TargetID scene = core.create_target({ .name = "scene" });
//   const TargetID light = core.create_target({ .name = "light", .scale = 0.5f });
//
//   recipe.passes = {
//       { .name = "scene",  .target = scene, .features = { { &r2d, view_bit(View::World ) } } },
//       { .name = "lights", .target = light, .features = { { &r2d, view_bit(View::Lights) } } },
//       ...
//   };
//
// Target::Backbuffer (0) is the swapchain image; it is not created.
//
// A target's texture is recreated when its size changes, so read
// RenderCore::target_texture() each frame (in upload() or record()) instead of
// keeping the handle; features that build bind groups over targets compare
// handles and rebuild when they change.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Format.hpp"
#include <cstdint> // uint8_t
#include <string>  // string

namespace Nezumi::Render {

using TargetID = std::uint8_t;

struct Target {
    static constexpr TargetID Backbuffer = 0;
};

struct RenderTargetConfig {
    std::string name   = "target";
    RHI::Format format = RHI::Format::RGBA16Float; // float: HDR scenes and light
    float       scale  = 1.0f; // of the backbuffer's size; 0.5 = half width and height
};

} // namespace Nezumi::Render
