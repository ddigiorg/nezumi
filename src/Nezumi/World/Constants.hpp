// =============================================================================
// Constants.hpp
// =============================================================================

#pragma once
#include <cstdint>     // int16_t
#include <glm/glm.hpp> // vec2, uvec2

namespace Nezumi {

// -----------------------------------------------------------------------------
// Tile
// -----------------------------------------------------------------------------

inline constexpr glm::vec2 TILE_SIZE = {
    /*.x = */1.0f, // world-space
    /*.y = */1.0f, // world-space
};

// -----------------------------------------------------------------------------
// Chunk
// -----------------------------------------------------------------------------

inline constexpr int16_t CHUNK_MIN_X = -31250;
inline constexpr int16_t CHUNK_MIN_Y = -31250;
inline constexpr int16_t CHUNK_MAX_X =  31249; // 62,500 total columns
inline constexpr int16_t CHUNK_MAX_Y =  31249; // 62,500 total rows

inline constexpr glm::uvec2 CHUNK_TILE_COUNT = {
    /*.x = */32,
    /*.y = */32,
};

inline constexpr glm::vec2 CHUNK_SIZE = {
    /*.x = */CHUNK_TILE_COUNT.x * TILE_SIZE.x, // world-space
    /*.y = */CHUNK_TILE_COUNT.y * TILE_SIZE.y, // world-space
};

// -----------------------------------------------------------------------------
// Sector
// -----------------------------------------------------------------------------

static constexpr int32_t SECTOR_MIN_X = -500000;
static constexpr int32_t SECTOR_MIN_Y = -500000;
static constexpr int32_t SECTOR_MAX_X =  499999; // 1,000,000 total columns
static constexpr int32_t SECTOR_MAX_Y =  499999; // 1,000,000 total rows

inline constexpr glm::uvec2 SECTOR_TILE_COUNT = {
    /*.x = */2,
    /*.y = */2,
};

inline constexpr glm::vec2 SECTOR_SIZE = {
    TILE_SIZE.x * SECTOR_TILE_COUNT.x,
    TILE_SIZE.y * SECTOR_TILE_COUNT.y,
};

} // namespace Nezumi
