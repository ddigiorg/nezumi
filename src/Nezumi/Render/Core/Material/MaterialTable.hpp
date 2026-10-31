// =============================================================================
// MaterialTable.hpp
//
// Bind group 1: one texture + sampler per MaterialId. A material is what the
// sort key's material field refers to, so draws that share one can batch.
// Today each atlas page is one material.
//
// Material ids are small (16 bits) and recycled after remove(). 0 means "no
// material" and is never handed out.
//
// Every texture/sampler call into the RHI lives in MaterialTable.cpp.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view
#include <vector>      // vector

namespace Nezumi::Render {

class MaterialTable {
public:

    // -------------------------------------------------------------------------
    // Statics
    // -------------------------------------------------------------------------

    static constexpr std::uint32_t MATERIAL_GROUP = 1;

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Throws if the shared layout or samplers can't be created.
    explicit MaterialTable(RHI::Device& device);
    ~MaterialTable();

    // Non-copyable and non-movable
    MaterialTable(const MaterialTable&)            = delete;
    MaterialTable& operator=(const MaterialTable&) = delete;
    MaterialTable(MaterialTable&&)                 = delete;
    MaterialTable& operator=(MaterialTable&&)      = delete;

    // -------------------------------------------------------------------------
    // Usage
    // -------------------------------------------------------------------------

    // A new material sampling `texture` through `sampler` (both must be live,
    // and stay live until remove()). Throws on failure; nothing is created
    // then, and the texture and sampler are left untouched for the caller.
    MaterialID create(
        std::string_view   name,
        RHI::TextureHandle texture,
        RHI::SamplerHandle sampler);

    // Removes the material's bind group; its id may be reused. Does NOT
    // remove the texture or sampler. Null or dead ids are ignored.
    void remove(MaterialID id);

    // Bind at GROUP. id must be live.
    void bind(RHI::CommandEncoder& encoder, MaterialID id) const;

    // For pipeline creation (RenderContext materials->layout()).
    RHI::BindGroupLayoutHandle layout() const { return layout_; }

    bool is_live(MaterialID id) const {
        return id != Material::None &&
               id <= slots_.size()  &&
               bool(slots_[id - 1].group);
    }

private:

    // -------------------------------------------------------------------------
    // Slot
    // -------------------------------------------------------------------------

    struct Slot {
        RHI::BindGroupHandle group   = {}; // owned
        RHI::TextureHandle   texture = {}; // not owned; kept for debugging
        RHI::SamplerHandle   sampler = {}; // not owned; kept for debugging
    };

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device&               device_;
    RHI::BindGroupLayoutHandle layout_ = {};
    std::vector<Slot>          slots_; // slots_[id - 1]
    std::vector<MaterialID>    free_;  // removed ids, reused first
};

} // namespace Nezumi::Render
