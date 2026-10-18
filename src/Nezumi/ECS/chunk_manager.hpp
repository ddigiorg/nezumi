#pragma once

#include "ecs/types.hpp"
#include "ecs/archetype.hpp"
#include "ecs/chunk.hpp"
#include "ecs/chunk_list.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "utils/assert.hpp"

#include <deque>
#include <unordered_map>
#include <vector>

namespace Game::ECS {

// =============================================================================
// ChunkManager
// =============================================================================

class ChunkManager {
public:
    ChunkManager(ArchetypeManager& archetype_mgr, EntityManager& entity_mgr);

    template<typename... Components>
    void insertEntity(EntityID e_id, ArchetypeID a_id, Components&&... data);
    void removeEntity(EntityID e_id);
    // void insertEntityComponent();
    // void removeEntityComponent();

    bool   hasChunk(ChunkID id) const;
    Chunk& getChunk(ChunkID id);

    bool       hasList(Archetype& archetype) const;
    ChunkList& getList(Archetype& archetype);

    std::vector<ChunkList*> getMatchedLists(ComponentMask mask);

    void print() const;

private:
    Chunk* newChunk_(Archetype& archetype);
    void freeChunk_(ChunkID c_id);
    ChunkList& getOrCreateList_(Archetype& archetype);

    ArchetypeManager& archetype_mgr_;
    EntityManager& entity_mgr_;
    std::deque<Chunk> chunks_;
	std::vector<ChunkID> chunk_free_ids_;
    std::unordered_map<ComponentMask, ChunkList> lists_;
};

ChunkManager::ChunkManager(ArchetypeManager& archetype_mgr, EntityManager& entity_mgr)
    : archetype_mgr_(archetype_mgr)
    , entity_mgr_(entity_mgr)
    , chunks_()
    , chunk_free_ids_()
    , lists_()
{}

bool ChunkManager::hasChunk(ChunkID id) const {
    if (id < static_cast<ChunkID>(chunks_.size()))
        return !(chunks_[id].id() == CHUNK_ID_NULL);
    return false;
}

Chunk& ChunkManager::getChunk(ChunkID id) {
    ASSERT(hasChunk(id), "ChunkID " << id << " does not exist.");
    return chunks_[id];
}

bool ChunkManager::hasList(Archetype& archetype) const {
    return lists_.find(archetype.mask()) != lists_.end();
}

ChunkList& ChunkManager::getList(Archetype& archetype) {
    ASSERT(hasList(archetype), "List does not exist.");
    return lists_.at(archetype.mask());
}

std::vector<ChunkList*> ChunkManager::getMatchedLists(ComponentMask mask) {
    std::vector<ChunkList*> matched_lists;
    matched_lists.reserve(lists_.size()); // Pre-allocate to avoid reallocation

    for (auto& [key, list] : lists_) {
        if ((key & mask) == mask) {
            matched_lists.push_back(&list);
        }
    }

    return matched_lists;
}

template<typename... Components>
void ChunkManager::insertEntity(EntityID e_id, ArchetypeID a_id, Components&&... data) {
    Archetype& archetype = archetype_mgr_.getArchetype(a_id);
    ChunkList& list = getOrCreateList_(archetype);
    Chunk* chunk = list.nextOpenChunk();

    // allocate new chunk if no empty slots available
    if (!chunk || chunk->isFull()) {
        chunk = newChunk_(archetype);
        list.insertChunk(chunk);
        list.insertChunkOpen(chunk);
    }

    chunk->insertEntity_(e_id, entity_mgr_, std::forward<Components>(data)...);

    // if chunk becomes full, remove it from open list
    if (chunk->isFull()) {
        list.removeChunkOpen(chunk);
    }
}

void ChunkManager::removeEntity(EntityID e_id) {
    const EntityRecord& entity = entity_mgr_.get(e_id);
    Chunk* chunk = entity.chunk;

    // Get chunk list for managing chunk state
    // TODO: clean this up (maybe put ptr to chunk list in chunk?)
    Archetype* archetype = chunk->archetype();
    ChunkList& list = lists_.at(archetype->mask()); // NOTE: should exist since chunk exists

    bool was_full_before_removal = chunk->isFull();

    chunk->removeEntity_(e_id, entity_mgr_);

    // if chunk becomes empty then remove it from lists_ and free it
    if (chunk->isEmpty()) {
        list.removeChunk(chunk);
        list.removeChunkOpen(chunk);
        freeChunk_(chunk->id());
    }

    // if chunk is no longer full then add it back to open list
    else if (was_full_before_removal) {
        list.insertChunkOpen(chunk);
    }
}

void ChunkManager::print() const {
    std::cout << "chunks_:" << std::endl;
    for (const Chunk& c : chunks_) {
        if (c.id() == CHUNK_ID_NULL) continue;
        std::cout << "  - id: "       << c.id()       << std::endl;
        std::cout << "    count: "    << c.count()    << std::endl;
        std::cout << "    capacity: " << c.capacity() << std::endl;
    }
}

Chunk* ChunkManager::newChunk_(Archetype& archetype) {
    ChunkID c_id = CHUNK_ID_NULL;

    if (!chunk_free_ids_.empty()) {
        c_id = chunk_free_ids_.back();
        chunk_free_ids_.pop_back();
        chunks_[c_id].initialize_(c_id, &archetype); // TODO: cleanup groupID in Chunk
        return &chunks_[c_id];
    }

    c_id = static_cast<ChunkID>(chunks_.size());
    chunks_.emplace_back();
    chunks_.back().initialize_(c_id, &archetype); // TODO: cleanup groupID in Chunk
    return &chunks_[c_id];
}

void ChunkManager::freeChunk_(ChunkID c_id) {
    ASSERT(hasChunk(c_id), "ChunkID " << c_id << " does not exist.");
    chunks_[c_id].clear_();
    chunk_free_ids_.push_back(c_id);
}

ChunkList& ChunkManager::getOrCreateList_(Archetype& archetype) {
    ComponentMask mask = archetype.mask();
    auto it = lists_.find(mask);
    if (it == lists_.end()) {
        return lists_.emplace(mask, ChunkList(mask)).first->second;
    }
    return it->second;
}

} // namespace Game::ECS