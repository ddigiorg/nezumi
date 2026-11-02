// =============================================================================
// KindResources.hpp
//
// A collection of GPU resources for each kind: its unit shape, and one
// pipeline per color format it has drawn into.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/RenderContext.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include <cstdint> // uint32_t
#include <string>  // string
#include <vector>  // vector

namespace Nezumi::Render {

struct KindResources {
    using MakePipeline = RHI::RenderPipelineHandle (*)(const RenderContext&, RHI::Format);

    struct FormatPipeline {
        RHI::Format               format   = RHI::Format::Undefined;
        RHI::RenderPipelineHandle pipeline = {};
    };

    std::string                 name;
    MakePipeline                make         = nullptr; // K::make_pipeline
    std::vector<FormatPipeline> pipelines;              // first: the backbuffer's
    RHI::BufferHandle           shape        = {};
    std::uint32_t               vertex_count = 0;
    std::uint32_t               stride       = 0; // sizeof(Instance)

    bool ready() const { return !pipelines.empty(); }

    // The pipeline for `format`, or a null handle if it hasn't been made
    RHI::RenderPipelineHandle pipeline(RHI::Format format) const {
        for (const FormatPipeline& p : pipelines) {
            if (p.format == format) return p.pipeline;
        }
        return {};
    }
};

} // namespace Nezumi::Render
