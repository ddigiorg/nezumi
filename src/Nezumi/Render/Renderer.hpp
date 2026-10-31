// =============================================================================
// Renderer.hpp
//
// DEPRECATED: compatibility for code written before the Render/Core +
// Render/2D split. Include "Nezumi/Render/Render.hpp" and use the new names
// instead; this header will be removed.
//
//   Render::Renderer       -> Render::Renderer2D
//   Render::RendererConfig -> Render::Renderer2DConfig (clear_color moved to
//                             Render::RenderCoreConfig)
//   Render::Camera         -> Render::Camera2D
//   Render::Projection     -> Nezumi::Projection (World/Projection.hpp)
//
// Header paths changed too (e.g. Render/View/Camera.hpp is now
// Render/2D/Camera/Camera2D.hpp); this header doesn't cover those.
// =============================================================================

#pragma once
#include "Nezumi/Render/Render.hpp"
#include "Nezumi/World/Projection.hpp"

namespace Nezumi::Render {

using Renderer       [[deprecated("use Render::Renderer2D")]]       = Renderer2D;
using RendererConfig [[deprecated("use Render::Renderer2DConfig")]] = Renderer2DConfig;
using Camera         [[deprecated("use Render::Camera2D")]]         = Camera2D;
using Projection     [[deprecated("use Nezumi::Projection")]]       = Nezumi::Projection;

} // namespace Nezumi::Render
