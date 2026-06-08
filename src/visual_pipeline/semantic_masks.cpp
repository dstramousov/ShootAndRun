/**
 * @file src/visual_pipeline/semantic_masks.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for semantic_masks.cpp.
 */

#include "visual_pipeline/semantic_masks.h"

#include <cstddef>
#include <string>

namespace sar::visual_pipeline {
namespace {

/**
 * @brief Checks whether expected size is present.
 */
bool HasExpectedSize(const std::vector<std::uint8_t>& mask,
                     std::size_t expected_size) {
  return mask.size() == expected_size;
}

/**
 * @brief Executes the resize bool mask operation.
 */
void ResizeBoolMask(std::vector<std::uint8_t>* mask, std::size_t size) {
  if (mask == nullptr) {
    return;
  }
  mask->clear();
  mask->resize(size, 0);
}

}  // namespace

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string SemanticMaskSummary::Dump() const {
  return "SemanticMaskSummary { total: " + std::to_string(total_tiles) +
         ", open: " + std::to_string(open_ground_tiles) +
         ", forest: " + std::to_string(forest_tiles) +
         ", road: " + std::to_string(road_tiles) +
         ", swamp: " + std::to_string(swamp_tiles) +
         ", water: " + std::to_string(water_tiles) +
         ", ruins: " + std::to_string(ruins_tiles) +
         ", wall: " + std::to_string(wall_tiles) +
         ", unknown: " + std::to_string(unknown_tiles) +
         ", walkable: " + std::to_string(walkable_tiles) +
         ", blocked: " + std::to_string(blocked_tiles) +
         ", vision_blocked: " + std::to_string(vision_blocked_tiles) +
         ", projectile_blocked: " +
         std::to_string(projectile_blocked_tiles) +
         ", cover: " + std::to_string(cover_tiles) +
         ", concealment: " + std::to_string(concealment_tiles) +
         ", low_ground: " + std::to_string(low_ground_tiles) +
         ", elevated: " + std::to_string(elevated_tiles) + " }";
}

/**
 * @brief Checks whether valid is true.
 */
bool SemanticMasks::IsValid() const {
  if (size.width <= 0 || size.height <= 0) {
    return false;
  }

  const std::size_t expected_size =
      static_cast<std::size_t>(size.width * size.height);
  return HasExpectedSize(open_ground, expected_size) &&
         HasExpectedSize(forest, expected_size) &&
         HasExpectedSize(road, expected_size) &&
         HasExpectedSize(swamp, expected_size) &&
         HasExpectedSize(water, expected_size) &&
         HasExpectedSize(ruins, expected_size) &&
         HasExpectedSize(wall, expected_size) &&
         HasExpectedSize(unknown, expected_size) &&
         HasExpectedSize(walkable, expected_size) &&
         HasExpectedSize(blocked, expected_size) &&
         HasExpectedSize(vision_blocked, expected_size) &&
         HasExpectedSize(projectile_blocked, expected_size) &&
         HasExpectedSize(cover, expected_size) &&
         HasExpectedSize(concealment, expected_size) &&
         height.size() == expected_size;
}

/**
 * @brief Counts bool mask.
 */
int SemanticMasks::BoolMaskCount() const {
  return 14;
}

/**
 * @brief Builds semantic masks.
 */
SemanticMasks BuildSemanticMasks(const LevelData& level, std::string* error) {
  SemanticMasks masks;
  masks.size = level.size;

  if (level.size.width <= 0 || level.size.height <= 0) {
    if (error != nullptr) {
      *error = "level size is invalid for semantic masks";
    }
    return masks;
  }

  const std::size_t expected_size =
      static_cast<std::size_t>(level.size.width * level.size.height);
  if (level.cells.size() != expected_size) {
    if (error != nullptr) {
      *error = "level cell count does not match semantic mask dimensions";
    }
    return masks;
  }

  ResizeBoolMask(&masks.open_ground, expected_size);
  ResizeBoolMask(&masks.forest, expected_size);
  ResizeBoolMask(&masks.road, expected_size);
  ResizeBoolMask(&masks.swamp, expected_size);
  ResizeBoolMask(&masks.water, expected_size);
  ResizeBoolMask(&masks.ruins, expected_size);
  ResizeBoolMask(&masks.wall, expected_size);
  ResizeBoolMask(&masks.unknown, expected_size);
  ResizeBoolMask(&masks.walkable, expected_size);
  ResizeBoolMask(&masks.blocked, expected_size);
  ResizeBoolMask(&masks.vision_blocked, expected_size);
  ResizeBoolMask(&masks.projectile_blocked, expected_size);
  ResizeBoolMask(&masks.cover, expected_size);
  ResizeBoolMask(&masks.concealment, expected_size);
  masks.height.assign(expected_size, 0);

  masks.summary.total_tiles = static_cast<int>(expected_size);

  for (std::size_t index = 0; index < expected_size; ++index) {
    const RuntimeCell& cell = level.cells[index];

    switch (cell.terrain) {
      case TerrainType::kOpenGround:
        masks.open_ground[index] = 1;
        ++masks.summary.open_ground_tiles;
        break;
      case TerrainType::kForest:
        masks.forest[index] = 1;
        ++masks.summary.forest_tiles;
        break;
      case TerrainType::kRoad:
        masks.road[index] = 1;
        ++masks.summary.road_tiles;
        break;
      case TerrainType::kSwamp:
        masks.swamp[index] = 1;
        ++masks.summary.swamp_tiles;
        break;
      case TerrainType::kWater:
        masks.water[index] = 1;
        ++masks.summary.water_tiles;
        break;
      case TerrainType::kRuins:
        masks.ruins[index] = 1;
        ++masks.summary.ruins_tiles;
        break;
      case TerrainType::kWall:
        masks.wall[index] = 1;
        ++masks.summary.wall_tiles;
        break;
      case TerrainType::kUnknown:
        masks.unknown[index] = 1;
        ++masks.summary.unknown_tiles;
        break;
    }

    masks.walkable[index] = cell.walkable;
    masks.blocked[index] = cell.collision;
    masks.vision_blocked[index] = cell.blocks_vision;
    masks.projectile_blocked[index] = cell.blocks_projectiles;
    masks.cover[index] = cell.cover > 0;
    masks.concealment[index] = cell.concealment > 0;
    masks.height[index] = cell.height;

    if (masks.walkable[index]) {
      ++masks.summary.walkable_tiles;
    }
    if (masks.blocked[index]) {
      ++masks.summary.blocked_tiles;
    }
    if (masks.vision_blocked[index]) {
      ++masks.summary.vision_blocked_tiles;
    }
    if (masks.projectile_blocked[index]) {
      ++masks.summary.projectile_blocked_tiles;
    }
    if (masks.cover[index]) {
      ++masks.summary.cover_tiles;
    }
    if (masks.concealment[index]) {
      ++masks.summary.concealment_tiles;
    }
    if (cell.height < 0) {
      ++masks.summary.low_ground_tiles;
    } else if (cell.height > 0) {
      ++masks.summary.elevated_tiles;
    }
  }

  return masks;
}

}  // namespace sar::visual_pipeline
