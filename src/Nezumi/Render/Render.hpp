// =============================================================================
// Render.hpp
//
// Umbrella include for the render module:
//
//   Render/Core  shared by every feature renderer: RenderCore (frame recipe,
//                views, materials, shaders, instance stream), fonts and text
//   Render/2D    Renderer2D: batched sprites, shapes and SDF text
//   Render/3D    (planned)
//
// Include narrower headers in engine code; this one is for games and examples.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Renderer2D.hpp"
#include "Nezumi/Render/Core/RenderCore.hpp"
