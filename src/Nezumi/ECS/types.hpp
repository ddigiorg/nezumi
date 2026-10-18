#pragma once

#include <cstddef>
#include <cstdint>
#include <limits> // for std::numeric_limits

namespace Game {
    class ECSManager;
}

namespace Game::ECS {

class Archetype;
class ArchetypeManager;
class Chunk;
class ChunkManager;
class Component;
class ComponentManager;
class Entity;
class EntityManager;
class Query;
class QueryManager;
class System;
class SystemManager;

using mask_t = uint64_t; // 64 bit mask where each bit represents a component

// =============================================================================
// Chunk
// =============================================================================

using ChunkID  = uint32_t;
using ChunkIdx = uint16_t;

constexpr const ChunkID  CHUNK_ID_NULL  = std::numeric_limits<ChunkID >::max();
constexpr const ChunkIdx CHUNK_IDX_NULL = std::numeric_limits<ChunkIdx>::max();

static constexpr ChunkIdx CHUNK_TOTAL_SIZE  = 16 * 1024; // 16 kilobytes
static constexpr ChunkIdx CHUNK_HEADER_SIZE = 256;
static constexpr ChunkIdx CHUNK_BUFFER_SIZE = CHUNK_TOTAL_SIZE - CHUNK_HEADER_SIZE;
static constexpr size_t CHUNK_COMPONENT_CAPACITY = 16;

} // namespace Game::ECS