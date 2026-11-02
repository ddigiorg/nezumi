// =============================================================================
// KindRegistry.hpp
//
// GPU resources per kind, registered up front in the Renderer2D constructor.
//
// A kind's pipeline is made for the backbuffer's format when it registers,
// and for any other target format the first time ensure_format() asks
// (Renderer2D does, before each frame's passes, for every pass it records).
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindConcept.hpp"
#include "Nezumi/Render/2D/Kind/KindID.hpp"
#include "Nezumi/Render/2D/Kind/KindResources.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>   // uint32_t
#include <stdexcept> // runtime_error
#include <string>    // string
#include <vector>    // vector

namespace Nezumi::Render {

class KindRegistry {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit KindRegistry(RHI::Device& device) : device_(device) {}
    ~KindRegistry() { clear(); }

    // Non-copyable and non-movable
    KindRegistry(const KindRegistry&)            = delete;
    KindRegistry& operator=(const KindRegistry&) = delete;
    KindRegistry(KindRegistry&&)                 = delete;
    KindRegistry& operator=(KindRegistry&&)      = delete;

    // -------------------------------------------------------------------------
    // Usage
    // -------------------------------------------------------------------------

    // Returns the kind's id, creating its resources on first call. Throws if
    // they can't be created (a missing or broken shader is a programmer
    // error, same as the old Renderer2D::validate_).
    template <KindConcept K>
    KindID ensure(const RenderContext& ctx) {
        const KindID id = get_kind_id<K>();
        if (id < kinds_.size() && kinds_[id].ready()) return id; // fast path
        create_<K>(id, ctx);
        return id;
    }

    // Only valid for ids returned by ensure().
    const KindResources& get(KindID id) const {
        return kinds_[id];
    }

    // Makes every registered kind's pipeline for `format`, if missing.
    // Throws like ensure() if one can't be made.
    void ensure_format(const RenderContext& ctx, RHI::Format format) {
        for (KindResources& k : kinds_) {
            if (!k.ready() || k.pipeline(format)) continue;
            const RHI::RenderPipelineHandle pipeline = k.make(ctx, format);
            if (!pipeline) {
                throw std::runtime_error(
                    "KindRegistry: pipeline for '" + k.name + "' failed (see log)"
                );
            }
            k.pipelines.push_back({ format, pipeline });
        }
    }

    void clear() {
        for (const KindResources& k : kinds_) {
            for (const auto& p : k.pipelines) device_.remove(p.pipeline);
            device_.remove(k.shape);
        }
        kinds_.clear();
    }

private:

    // -------------------------------------------------------------------------
    // Create Helper
    // -------------------------------------------------------------------------

    template <KindConcept K>
    void create_(KindID id, const RenderContext& ctx) {
        if (kinds_.size() <= id) kinds_.resize(std::size_t(id) + 1);

        const std::string name(K::NAME);
        const std::span<const std::byte> shape_bytes = K::shape_vertices();
        const std::string shape_name = name + " shape";

        // Create shape buffer
        // Written once at creation, so no CopyDst.
        const RHI::BufferHandle shape = device_.create(
            RHI::BufferConfig{
                .name         = shape_name,
                .size         = shape_bytes.size(),
                .usage        = RHI::BufferUsageFlags::Vertex,
                .initial_data = shape_bytes,
            }
        );
        if (!shape) {
            throw std::runtime_error(
                "KindRegistry: shape buffer for '" + name + "' failed (see log)"
            );
        }

        // Create pipeline, for the backbuffer
        const RHI::Format               format   = ctx.device->backbuffer_format();
        const RHI::RenderPipelineHandle pipeline = K::make_pipeline(ctx, format);
        if (!pipeline) {
            device_.remove(shape);
            throw std::runtime_error(
                "KindRegistry: pipeline for '" + name + "' failed (see log)"
            );
        }

        kinds_[id] = KindResources{
            .name         = name,
            .make         = &K::make_pipeline,
            .pipelines    = { { format, pipeline } },
            .shape        = shape,
            .vertex_count = static_cast<std::uint32_t>(K::VERTEX_COUNT),
            .stride       = static_cast<std::uint32_t>(sizeof(typename K::Instance)),
        };
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device&               device_;
    std::vector<KindResources> kinds_;
};

} // namespace Nezumi::Render
