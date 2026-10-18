#pragma once

#include "ecs/types.hpp"
#include "ecs/archetype.hpp"
#include "ecs/component.hpp"
#include "utils/assert.hpp"

#include <array>
#include <cstddef> // for std::byte
#include <cstdint>
#include <cstring> // for std::memcpy
#include <vector>

namespace Game::ECS {

// =============================================================================
// Chunk
//
// A Chunk is a flat contiguous block of memory storing entity component data.
// It is designed for efficient memory access and cache locality.
//
// { h_0, h_1, ..., h_N,   header 256 bytes
//   e_0, e_1, ..., e_X,   chunk.data<EntityIDs>()
//   cA0, cA1, ..., cAX,   chunk.data<ComponentA>()
//   cB0, cB1, ..., cBX,   chunk.data<ComponentB>()
//   ..., ..., ..., ...,   ...
//   cY0, cY1, ..., cYX, } chunk.data<ComponentY>
//
// =============================================================================

class Chunk {
    friend class ChunkManager;
    friend class ChunkList;

public:
    Chunk() { clear_(); }

    // buffer_ access (NOTE: unsafe! can be indexed out of bounds...)
    const EntityID* entityIDs() const;
    template <typename T>
    T*       data();
    template <typename T>
    const T* data() const;

    // queries
    template <typename T>
    bool hasComponent() const noexcept;
    bool isFull()  const noexcept { return count_ >= capacity_; }
    bool isEmpty() const noexcept { return count_ == 0; }

    // getters
    ChunkID    id()        const noexcept { return id_; }
    ChunkIdx   count()     const noexcept { return count_; }
    ChunkIdx   capacity()  const noexcept { return capacity_; }
    Archetype* archetype() const noexcept { return archetype_; }
    Chunk*     nextChunk() const noexcept { return next_; }
    Chunk*     prevChunk() const noexcept { return prev_; }

private:
    void clear_();
    void initialize_(ChunkID id, Archetype* archetype);
    EntityID* entityIDs_();

    // handle entity data
    template<typename... Components>
    void insertEntity_(EntityID e_id, EntityManager& entity_mgr, Components&&... data);
    void removeEntity_(EntityID e_id, EntityManager& entity_mgr);
    // void moveEntityTo_(Chunk* other, Entity& entity);
    // void moveEntityFrom_(Chunk* other, Entity& entity);

    // handle entity data helpers
    inline void setEntityID_(ChunkIdx index, EntityID e_id);
    template<typename T, typename... Rest>
    void setEntityComponentData_(ChunkIdx index, T&& first, Rest&&... rest);
    template<typename T>
    void setSingleEntityComponentData_(ChunkIdx index, T&& component_data);

    // header (metadata)
    ChunkID id_;           // 4B unique identifier
    Chunk* prev_;          // 8B list node to prev chunk
    Chunk* next_;          // 8B list node to next chunk
    Chunk* prev_open_;     // 8B list node to prev chunk with open entity slots
    Chunk* next_open_;     // 8B list node to next chunk with open entity slots
    ChunkIdx count_;       // 2B num entities in this chunk
    ChunkIdx capacity_;    // 2B max entities in this chunk
    uint32_t version;      // 4B structural change version
    Archetype* archetype_; // 8B pointer to parent archetype

    // header (component address lookup tables)
    std::array<ComponentID, COMPONENT_CAPACITY> to_idx_;   // 64B
    std::array<void*, CHUNK_COMPONENT_CAPACITY> buf_ptrs_; // 128B

    // entity component data buffer_
    alignas(64) std::array<std::byte, CHUNK_BUFFER_SIZE> buffer_; // 16KB - 256B
};

// =============================================================================
// Chunk Functions
// =============================================================================

template <typename T>
inline bool Chunk::hasComponent() const noexcept {
    ComponentID c_id = getComponentID<T>();
    if (c_id < COMPONENT_CAPACITY)
        return buf_ptrs_[to_idx_[c_id]] != nullptr;
    return false;
}

inline EntityID* Chunk::entityIDs_() {
    return reinterpret_cast<EntityID*>(buffer_.data());
}

inline const EntityID* Chunk::entityIDs() const {
    return reinterpret_cast<const EntityID*>(buffer_.data());
}

template <typename T>
inline T* Chunk::data() {
    ASSERT(hasComponent<T>(), "Component is not in Chunk.");
    ComponentID c_id = getComponentID<T>();
    return reinterpret_cast<T*>(buf_ptrs_[to_idx_[c_id]]);
}

template <typename T>
inline const T* Chunk::data() const {
    ASSERT(hasComponent<T>(), "Component is not in Chunk.");
    ComponentID c_id = getComponentID<T>();
    return reinterpret_cast<const T*>(buf_ptrs_[to_idx_[c_id]]);
}

void Chunk::clear_() {
    id_        = CHUNK_ID_NULL;
    prev_      = nullptr;
    next_      = nullptr;
    prev_open_ = nullptr;
    next_open_ = nullptr;
    count_     = 0;
    capacity_  = 0;
    version    = 0;
    archetype_ = nullptr;

    to_idx_.fill(COMPONENT_ID_NULL);
    buf_ptrs_.fill(nullptr);
}

void Chunk::initialize_(ChunkID id, Archetype* archetype) {
    id_ = id;
    archetype_ = archetype;
    capacity_ = archetype_->capacity();

    // initialize component address lookup tables
    const std::vector<ComponentRecord>& components = archetype_->components();
    for (size_t i = 0; i < components.size(); i++) {
        const ComponentRecord& c = components[i];
        to_idx_[c.id] = static_cast<ComponentID>(i);
        // TODO: need to check if archetype_ does this
        // ASSERT(offset + sizeof(T) * count_ <= BUFFER_SIZE, "Component array exceeds chunk buffer_.");
        buf_ptrs_[i] = static_cast<void*>(buffer_.data() + c.offset);
    }
}

// =============================================================================
// Chunk Entity Functions
// =============================================================================

template<typename... Components>
void Chunk::insertEntity_(EntityID e_id, EntityManager& entity_mgr, Components&&... data) {
    ASSERT(!isFull(), "Cannot insert entity into full chunk.");

    const EntityRecord& entity = entity_mgr.get(e_id);
    setEntityID_(count_, e_id);
    setEntityComponentData_(count_, std::forward<Components>(data)...);
    entity_mgr.set(e_id, this, count_);
    count_++;
    // version++;
}

void Chunk::removeEntity_(EntityID e_id, EntityManager& entity_mgr) {
    const EntityRecord& remv_entity = entity_mgr.get(e_id);
    ChunkIdx remv_idx = remv_entity.index;

    ASSERT(entityIDs_()[remv_idx] == e_id, "Entity ID mismatch at chunk location.");

    ChunkIdx last_idx = count_ - 1;

    if (remv_idx != last_idx) {
        EntityID last_id = entityIDs_()[last_idx];
        const EntityRecord& last_entity = entity_mgr.get(last_id);

        setEntityID_(remv_idx, last_id);

        for (const ComponentRecord& component : archetype_->components()) {
            if (component.isTag()) continue;

            ComponentID c_id = component.id;
            ComponentSize c_size = component.size;

            std::byte* arr = static_cast<std::byte*>(buf_ptrs_[to_idx_[c_id]]);
            std::byte* dst = arr + (c_size * remv_idx);
            std::byte* src = arr + (c_size * last_idx);

            std::memcpy(dst, src, c_size);
        }

        entity_mgr.set(last_id, this, remv_idx);
    }

    entity_mgr.destroy(e_id);

    count_--;
    // version++;
}

inline void Chunk::setEntityID_(ChunkIdx index, EntityID e_id) {
    ASSERT(index < capacity_, "Index out of bounds.");
    entityIDs_()[index] = e_id;
}

template<typename T, typename... Rest>
void Chunk::setEntityComponentData_(ChunkIdx index, T&& first, Rest&&... rest) {
    // set the first component data
    setSingleEntityComponentData_<T>(index, std::forward<T>(first));

    // recursively set the rest if any
    if constexpr (sizeof...(rest) > 0) {
        setEntityComponentData_(index, std::forward<Rest>(rest)...);
    }
}

template<typename T>
void Chunk::setSingleEntityComponentData_(ChunkIdx index, T&& component_data) {
    if constexpr (IsTagType<T>) return; // ignore tags
    ASSERT(hasComponent<T>(), "Component is not in Chunk.");
    data<T>()[index] = std::forward<T>(component_data);
}

} // namespace Game::ECS