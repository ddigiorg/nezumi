// =============================================================================
// KindConcept.hpp
//
// An kind is everything that differs between draw objects (rectangles, sprites,
// circles, etc): the per-instance GPU data, the unit shape, and the pipeline.
//
// Pipeline contract (every kind):
//   - vertex buffer 0: the unit shape, VertexStepMode::Vertex
//   - vertex buffer 1: Instance, VertexStepMode::Instance, stride sizeof(Instance)
//   - bind group 0:    ctx.frame_layout (camera)
//   - bind group 1:    material, for textured kinds (later)
// =============================================================================

#pragma once
#include "Nezumi/Render/Frame/CommandQueue.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cassert>     // assert
#include <concepts>    // convertible_to, same_as
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view
#include <type_traits> // is_trivially_copyable_v

namespace Nezumi::Render {

template <typename K>
concept KindConcept =
    std::is_trivially_copyable_v<typename K::Instance> &&
    alignof(typename K::Instance) <= CommandQueue::ARENA_ALIGN &&
    requires(const RendererContext& ctx) {
        { K::NAME               } -> std::convertible_to<std::string_view>;
        { K::VERTEX_COUNT       } -> std::convertible_to<std::uint32_t>;
        { K::shape_vertices()   } -> std::convertible_to<std::span<const std::byte>>;
        { K::make_pipeline(ctx) } -> std::same_as<RHI::RenderPipelineHandle>;
    };

} // namespace Nezumi::Render
