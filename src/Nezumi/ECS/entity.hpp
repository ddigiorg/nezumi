#pragma once

#include "ecs/types.hpp"
#include "utils/assert.hpp"

#include <limits>
#include <vector>

namespace Game::ECS {

// =============================================================================
// Entity Types
// =============================================================================

using EntityID  = uint32_t;
constexpr EntityID ENTITY_ID_NULL = std::numeric_limits<EntityID>::max();

struct EntityRecord {
    EntityID id    = ENTITY_ID_NULL;
    Chunk*   chunk = nullptr;
    ChunkIdx index = CHUNK_IDX_NULL;

    // Ordering by chunk addresses for better cache hits
    bool operator<(const EntityRecord& other) const noexcept {
        return chunk < other.chunk;
    }
};

// =============================================================================
// EntityManager
// =============================================================================

class EntityManager {
public:
    EntityManager();

    bool has(EntityID id) const;
    const EntityRecord& get(EntityID id) const;
    void set(EntityID id, Chunk* chunk, ChunkIdx index);
    EntityID create();
    void destroy(EntityID id);

    size_t count() const { return count_; };

    void print() const;

private:
    // TODO: add reserve function that reserves space for these
    std::vector<EntityRecord> records_;
    std::vector<EntityID> free_ids_;
    size_t count_;
};

EntityManager::EntityManager()
    : records_()
    , free_ids_()
    , count_(0)
{}

inline bool EntityManager::has(EntityID id) const {
    return id < static_cast<EntityID>(records_.size()) && records_[id].id != ENTITY_ID_NULL;
}

inline const EntityRecord& EntityManager::get(EntityID id) const {
    ASSERT(has(id), "EntityID " << id << " does not exist.");
    return records_[id];
}

inline void EntityManager::set(EntityID id, Chunk* chunk, ChunkIdx index) {
    ASSERT(has(id), "EntityID " << id << " does not exist.");
    EntityRecord& r = records_[id];
    r.id    = id;
    r.chunk = chunk;
    r.index = index;
}

EntityID EntityManager::create() {
    EntityID id = ENTITY_ID_NULL;

    if (!free_ids_.empty()) {
        id = free_ids_.back();
        free_ids_.pop_back();
        records_[id].id = id;
    }
    else {
        id = static_cast<EntityID>(records_.size());
        records_.emplace_back();
        records_.back().id = id;
    }

    count_++;
    return id;
}

void EntityManager::destroy(EntityID id) {
    ASSERT(has(id), "EntityID " << id << " does not exist.");
    EntityRecord& r = records_[id];
    r.id    = ENTITY_ID_NULL;
    r.chunk = nullptr;
    r.index = CHUNK_IDX_NULL;
    free_ids_.push_back(id);
    count_--;
}

void EntityManager::print() const {
    std::cout << "entities:" << std::endl;
    for (const EntityRecord& r : records_) {
        if (r.id != ENTITY_ID_NULL && r.chunk != nullptr) continue;
        // TODO: fix this!!!!!!!!!!!!!!!!!!!!!!!!!
        // std::cout << "  - id: "    << r.id          << std::endl;
        // std::cout << "    chunk: " << r.chunk->id() << std::endl;
        // std::cout << "    index: " << r.index       << std::endl;
    }
}

} // namepsace Game::ECS
