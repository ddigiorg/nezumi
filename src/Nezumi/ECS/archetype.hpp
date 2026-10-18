#pragma once

#include "ecs/types.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "utils/assert.hpp"

#include <algorithm> // for std::sort
#include <array>
#include <deque>
#include <iostream>
#include <limits> // for std::numeric_limits
#include <unordered_map>
#include <vector>

namespace Game::ECS {

// =============================================================================
// Archetype Types
// =============================================================================

using ArchetypeID   = uint16_t;
using ArchetypeMask = mask_t; // TODO: standardize this with ComponentMask

constexpr const ArchetypeID ARCHETYPE_ID_NULL = std::numeric_limits<ArchetypeID>::max();

// =============================================================================
// Archetype
// =============================================================================
    
class Archetype {
    friend class ArchetypeManager;

public:
    Archetype(
        ArchetypeID id,
        ArchetypeMask mask,
        const std::vector<ComponentRecord>& components);

    bool hasComponent(ComponentID c_id) const noexcept;

    ArchetypeID id() const noexcept { return id_; }
    ArchetypeMask mask() const noexcept { return mask_; }
    ChunkIdx capacity() const noexcept { return capacity_; }
    const std::vector<ComponentRecord>& components() const noexcept { return components_; }

private:
	ArchetypeID id_;     // unique archetype identifier
    ArchetypeMask mask_; // bitmask where set bits represent components_
    ChunkIdx capacity_;  // maximum number of entities in chunk of this archetype
    std::vector<ComponentRecord> components_;

    // TODO: implement
    // neighbor graph nodes
    // std::array<Archetype*, COMPONENT_CAPACITY> removes_;
    // std::array<Archetype*, COMPONENT_CAPACITY> inserts_;
};

Archetype::Archetype(
        ArchetypeID id,
        ArchetypeMask mask,
        const std::vector<ComponentRecord>& components
) {
    ASSERT(components.size() < CHUNK_COMPONENT_CAPACITY,
        "Archetype cannot contain more than 16 components_.");

    id_ = id;
    mask_ = mask;
    components_ = components;
    capacity_ = CHUNK_BUFFER_SIZE;

    // get size of all component data in a single entity of this archetype
    // NOTE: first "component" is always EntityID
    ChunkIdx entity_size = static_cast<ChunkIdx>(sizeof(EntityID));
    for (const ComponentRecord& c : components_) {
        entity_size += c.size;
    }

    ASSERT(entity_size < CHUNK_BUFFER_SIZE,
        "Archetype component data size exceeds chunk buffer size.");

    // set archetype capacity_ (i.e. num entities in a chunk) based on data size
    if (entity_size > 0) {
        capacity_ = CHUNK_BUFFER_SIZE / entity_size;
    }

    // assign component information
    ComponentSize offset = sizeof(EntityID) * capacity_;
    for (ComponentRecord& c : components_) {
        c.offset = offset;
        offset += c.size * capacity_;
    }
}

inline bool Archetype::hasComponent(ComponentID c_id) const noexcept {
    return (mask_ & (ArchetypeMask(1) << c_id)) != 0;
}

// =============================================================================
// ArchetypeManager
// =============================================================================

class ArchetypeManager {
public:
    ArchetypeManager(ComponentManager& componentMgr);

    bool hasArchetype(ArchetypeID id) const;
    Archetype& getArchetype(ArchetypeID id);

    template<typename... Components>
    ArchetypeID getOrCreate();

    void print() const;

private:
    ComponentManager& component_mgr_;
    std::deque<Archetype> archetypes_;
    std::unordered_map<ArchetypeMask, ArchetypeID> maskToIDs_;
};

ArchetypeManager::ArchetypeManager(ComponentManager& componentMgr)
    : component_mgr_(componentMgr)
    , archetypes_()
    , maskToIDs_()
{
    getOrCreate();
}

bool ArchetypeManager::hasArchetype(ArchetypeID id) const {
    if (id < static_cast<ArchetypeID>(archetypes_.size()))
        return !(archetypes_[id].id() == ARCHETYPE_ID_NULL);
    return false;
}

Archetype& ArchetypeManager::getArchetype(ArchetypeID id) {
    ASSERT(hasArchetype(id), "ArchetypeID " << id << " does not exist.");
    return archetypes_[id];
}

template<typename... Components>
ArchetypeID ArchetypeManager::getOrCreate() {

    // build a vector of registered components_
    std::vector<ComponentRecord> components_;
    if constexpr (sizeof...(Components) > 0) {
        components_.reserve(sizeof...(Components));
    }

    ((components_.push_back(component_mgr_.getRecord<Components>())), ...);

    // sort components_ by ID
    std::sort(
        components_.begin(),
        components_.end(),
        [](const ComponentRecord& a, const ComponentRecord& b) {
            return a.id < b.id;
        }
    );

    // build archetype mask_
    ArchetypeMask mask_ = 0;
    for (const ComponentRecord& c : components_) {
        mask_ |= (ArchetypeMask(1) << c.id);
    }

    // if archetype already exists then return it
    auto it = maskToIDs_.find(mask_);
    if (it != maskToIDs_.end()) {
        return it->second;
    }

    // otherwise create new archetype
    ArchetypeID id_ = static_cast<ArchetypeID>(archetypes_.size());
    archetypes_.emplace_back(id_, mask_, std::move(components_));
    maskToIDs_.insert({mask_, id_});
    return id_;
}

void ArchetypeManager::print() const {
    std::cout << "archetypes_:" << std::endl;
    for (const Archetype& a : archetypes_) {
        std::cout << "  - id: "           << a.id_         << std::endl;
        std::cout << "    mask: "         << a.mask_       << std::endl;
        std::cout << "    capacity: "     << a.capacity_   << std::endl;
        std::cout << "    components: "                    << std::endl;
        std::cout << "    - id: <EntityIDs>"               << std::endl;
        std::cout << "      offset: "     << 0             << std::endl;
        for (const ComponentRecord& c : a.components()) {
            std::cout << "    - id: "     << (int)c.id     << std::endl;
            std::cout << "      offset: " <<      c.offset << std::endl;
        }
    }
}

} // namespace Game::ECS