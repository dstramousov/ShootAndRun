/**
 * @file src/render3d/level_3d_renderer.cpp
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains implementation
 * for level_3d_renderer.cpp.
 */

#include "render3d/level_3d_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include "level/terrain_type.h"

namespace sar::render3d {
namespace {

constexpr unsigned char kVisibilityUnknown = 0;
constexpr unsigned char kVisibilitySeen = 1;
constexpr unsigned char kVisibilityVisible = 2;

/**
 * @brief Stores tile range 3D data shared between runtime systems.
 */
struct TileRange3D {
  int min_x = 0;
  int max_x = 0;
  int min_y = 0;
  int max_y = 0;
};

/**
 * @brief Stores chunk range 3D data shared between runtime systems.
 */
struct ChunkRange3D {
  int min_x = 0;
  int max_x = 0;
  int min_y = 0;
  int max_y = 0;
};

/**
 * @brief Executes the scale color rgb operation.
 */
Color ScaleColorRgb(Color color, float scale, unsigned char alpha);

/**
 * @brief Counts active tile candidates and visible/renderable tiles for diagnostics.
 */
void CountRenderableTilesInRange(const LevelData& level,
                                 const Level3DViewState& state,
                                 const TileRange3D& range,
                                 Level3DPerfStats* stats);

/**
 * @brief Converts a floating tile position to an integer tile index.
 */
int TileIndexFromPosition(float value) {
  return static_cast<int>(std::floor(value));
}

/**
 * @brief Converts tile coordinates to a level cell index.
 */
int TileIndex(const LevelData& level, int x, int y) {
  return y * level.size.width + x;
}

/**
 * @brief Checks whether tile coordinates are inside the loaded map.
 */
bool IsInsideMap(const LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

/**
 * @brief Returns the runtime cell at tile coordinates or nullptr when outside the map.
 */
const RuntimeCell* CellAt(const LevelData& level, int x, int y) {
  if (!IsInsideMap(level, x, y)) {
    return nullptr;
  }
  const int index = TileIndex(level, x, y);
  if (index < 0 || index >= static_cast<int>(level.cells.size())) {
    return nullptr;
  }
  return &level.cells[static_cast<std::size_t>(index)];
}

/**
 * @brief Clamps the configured chunk size to the supported runtime range.
 */
int SafeChunkSize(const Level3DViewState& state) {
  return std::clamp(state.chunk_size_tiles, 4, 64);
}

/**
 * @brief Returns chunk index from tile.
 */
int ChunkIndexFromTile(int tile, int chunk_size) {
  return tile / std::max(1, chunk_size);
}

/**
 * @brief Returns minimum chunk radius for tile radius.
 */
int MinimumChunkRadiusForTileRadius(int radius_tiles, int chunk_size) {
  const int safe_chunk_size = std::max(1, chunk_size);
  return std::max(0, (std::max(0, radius_tiles) + safe_chunk_size - 1) /
                         safe_chunk_size + 1);
}

/**
 * @brief Computes the active chunk range around the current 3D culling center.
 */
ChunkRange3D ActiveChunkRange(const LevelData& level,
                              const Level3DViewState& state) {
  const int chunk_size = SafeChunkSize(state);
  const int max_chunk_x = ChunkIndexFromTile(level.size.width - 1, chunk_size);
  const int max_chunk_y = ChunkIndexFromTile(level.size.height - 1, chunk_size);
  const int center_x = state.culling_center_initialized
                           ? state.culling_center_tile_x
                           : TileIndexFromPosition(state.player.tile_x);
  const int center_y = state.culling_center_initialized
                           ? state.culling_center_tile_y
                           : TileIndexFromPosition(state.player.tile_y);
  const int center_chunk_x = ChunkIndexFromTile(
      std::clamp(center_x, 0, level.size.width - 1), chunk_size);
  const int center_chunk_y = ChunkIndexFromTile(
      std::clamp(center_y, 0, level.size.height - 1), chunk_size);
  const int radius_source = state.visibility_enabled
                                ? state.visibility_radius_tiles
                                : state.visible_radius_tiles;
  const int minimum_radius = MinimumChunkRadiusForTileRadius(radius_source,
                                                            chunk_size);
  const int active_radius = std::clamp(
      std::max(state.active_chunk_radius, minimum_radius), 0, 32);
  return ChunkRange3D{std::clamp(center_chunk_x - active_radius, 0, max_chunk_x),
                      std::clamp(center_chunk_x + active_radius, 0, max_chunk_x),
                      std::clamp(center_chunk_y - active_radius, 0, max_chunk_y),
                      std::clamp(center_chunk_y + active_radius, 0, max_chunk_y)};
}

/**
 * @brief Converts an active chunk range to an inclusive tile range.
 */
TileRange3D TileRangeFromChunks(const LevelData& level,
                                const Level3DViewState& state,
                                const ChunkRange3D& chunks) {
  const int chunk_size = SafeChunkSize(state);
  return TileRange3D{std::clamp(chunks.min_x * chunk_size, 0, level.size.width - 1),
                     std::clamp((chunks.max_x + 1) * chunk_size - 1, 0,
                                level.size.width - 1),
                     std::clamp(chunks.min_y * chunk_size, 0, level.size.height - 1),
                     std::clamp((chunks.max_y + 1) * chunk_size - 1, 0,
                                level.size.height - 1)};
}

/**
 * @brief Updates culling center for the current frame.
 */
void UpdateCullingCenter(const LevelData& level, Level3DViewState* state) {
  if (state == nullptr || level.size.width <= 0 || level.size.height <= 0) {
    return;
  }

  const int player_tile_x = std::clamp(TileIndexFromPosition(state->player.tile_x),
                                       0, level.size.width - 1);
  const int player_tile_y = std::clamp(TileIndexFromPosition(state->player.tile_y),
                                       0, level.size.height - 1);
  if (!state->culling_center_initialized) {
    state->culling_center_tile_x = player_tile_x;
    state->culling_center_tile_y = player_tile_y;
    state->culling_center_initialized = true;
    return;
  }

  const int deadzone = std::max(1, state->culling_deadzone_tiles);
  const int dx = player_tile_x - state->culling_center_tile_x;
  const int dy = player_tile_y - state->culling_center_tile_y;
  if (std::abs(dx) < deadzone && std::abs(dy) < deadzone) {
    return;
  }

  state->culling_center_tile_x = player_tile_x;
  state->culling_center_tile_y = player_tile_y;
}

/**
 * @brief Ensures visibility buffer.
 */
void EnsureVisibilityBuffer(const LevelData& level, Level3DViewState* state) {
  if (state == nullptr) {
    return;
  }
  const int width = std::max(0, level.size.width);
  const int height = std::max(0, level.size.height);
  const std::size_t expected_size = static_cast<std::size_t>(width) *
                                    static_cast<std::size_t>(height);
  if (state->visibility_width == width && state->visibility_height == height &&
      state->visibility_tiles.size() == expected_size) {
    return;
  }
  state->visibility_width = width;
  state->visibility_height = height;
  state->visibility_tiles.assign(expected_size, kVisibilityUnknown);
  state->visibility_current_indices.clear();
  state->visibility_state_valid = false;
}

/**
 * @brief Checks whether vision blocker is true.
 */
bool IsVisionBlocker(const LevelData& level, int x, int y) {
  const RuntimeCell* cell = CellAt(level, x, y);
  return cell != nullptr && cell->blocks_vision;
}

/**
 * @brief Checks whether line of sight is present.
 */
bool HasLineOfSight(const LevelData& level, int from_x, int from_y,
                    int to_x, int to_y) {
  if (from_x == to_x && from_y == to_y) {
    return true;
  }

  int x = from_x;
  int y = from_y;
  const int dx = std::abs(to_x - from_x);
  const int dy = std::abs(to_y - from_y);
  const int step_x = from_x < to_x ? 1 : -1;
  const int step_y = from_y < to_y ? 1 : -1;
  int error = dx - dy;

  while (x != to_x || y != to_y) {
    const int doubled_error = error * 2;
    if (doubled_error > -dy) {
      error -= dy;
      x += step_x;
    }
    if (doubled_error < dx) {
      error += dx;
      y += step_y;
    }

    if (x == to_x && y == to_y) {
      return true;
    }
    if (!IsInsideMap(level, x, y)) {
      return false;
    }
    if (IsVisionBlocker(level, x, y)) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Executes the visibility index operation.
 */
std::size_t VisibilityIndex(const Level3DViewState& state, int x, int y) {
  return static_cast<std::size_t>(y) *
             static_cast<std::size_t>(state.visibility_width) +
         static_cast<std::size_t>(x);
}

/**
 * @brief Executes the visibility at operation.
 */
unsigned char VisibilityAt(const Level3DViewState& state, int x, int y) {
  if (!state.visibility_enabled) {
    return kVisibilityVisible;
  }
  if (x < 0 || y < 0 || x >= state.visibility_width ||
      y >= state.visibility_height) {
    return kVisibilityUnknown;
  }
  const std::size_t index = VisibilityIndex(state, x, y);
  if (index >= state.visibility_tiles.size()) {
    return kVisibilityUnknown;
  }
  return state.visibility_tiles[index];
}

/**
 * @brief Checks whether tile renderable is true.
 */
bool IsTileRenderable(const Level3DViewState& state, int x, int y) {
  const unsigned char visibility = VisibilityAt(state, x, y);
  return visibility == kVisibilityVisible ||
         (state.visibility_memory_enabled && visibility == kVisibilitySeen);
}

/**
 * @brief Applies visibility color.
 */
Color ApplyVisibilityColor(Color color, const Level3DViewState& state,
                           int x, int y) {
  if (!state.visibility_enabled) {
    return color;
  }
  const unsigned char visibility = VisibilityAt(state, x, y);
  if (visibility == kVisibilityVisible) {
    return color;
  }
  if (visibility == kVisibilitySeen) {
    const float factor = std::clamp(state.seen_tile_dim_factor, 0.0F, 1.0F);
    const int alpha = std::clamp(
        static_cast<int>(std::lround(static_cast<float>(color.a) *
                                     std::max(0.25F, factor))),
        0, 255);
    return ScaleColorRgb(color, factor, static_cast<unsigned char>(alpha));
  }
  return Color{0, 0, 0, 0};
}

/**
 * @brief Updates visibility state for the current frame.
 */
void UpdateVisibilityState(const LevelData& level, Level3DViewState* state) {
  if (state == nullptr || level.size.width <= 0 || level.size.height <= 0) {
    return;
  }

  EnsureVisibilityBuffer(level, state);

  const int center_x = std::clamp(TileIndexFromPosition(state->player.tile_x),
                                  0, level.size.width - 1);
  const int center_y = std::clamp(TileIndexFromPosition(state->player.tile_y),
                                  0, level.size.height - 1);
  const int radius = std::clamp(state->visibility_radius_tiles, 1, 128);

  const bool inputs_unchanged =
      state->visibility_state_valid &&
      state->visibility_last_center_x == center_x &&
      state->visibility_last_center_y == center_y &&
      state->visibility_last_radius_tiles == radius &&
      state->visibility_last_enabled == state->visibility_enabled &&
      state->visibility_last_memory_enabled == state->visibility_memory_enabled &&
      state->visibility_last_fog_mode == state->fog_mode;
  if (inputs_unchanged) {
    return;
  }

  if (!state->visibility_enabled) {
    std::fill(state->visibility_tiles.begin(), state->visibility_tiles.end(),
              kVisibilityVisible);
    state->visibility_current_indices.clear();
    state->visibility_state_valid = true;
    state->visibility_last_center_x = center_x;
    state->visibility_last_center_y = center_y;
    state->visibility_last_radius_tiles = radius;
    state->visibility_last_enabled = state->visibility_enabled;
    state->visibility_last_memory_enabled = state->visibility_memory_enabled;
    state->visibility_last_fog_mode = state->fog_mode;
    return;
  }

  if (state->visibility_state_valid && !state->visibility_last_enabled) {
    std::fill(state->visibility_tiles.begin(), state->visibility_tiles.end(),
              kVisibilityUnknown);
    state->visibility_current_indices.clear();
  }

  for (const std::size_t index : state->visibility_current_indices) {
    if (index >= state->visibility_tiles.size()) {
      continue;
    }
    state->visibility_tiles[index] = state->visibility_memory_enabled
                                        ? kVisibilitySeen
                                        : kVisibilityUnknown;
  }
  state->visibility_current_indices.clear();

  const int radius_squared = radius * radius;
  const int min_x = std::clamp(center_x - radius, 0, level.size.width - 1);
  const int max_x = std::clamp(center_x + radius, 0, level.size.width - 1);
  const int min_y = std::clamp(center_y - radius, 0, level.size.height - 1);
  const int max_y = std::clamp(center_y + radius, 0, level.size.height - 1);

  for (int y = min_y; y <= max_y; ++y) {
    for (int x = min_x; x <= max_x; ++x) {
      const int dx = x - center_x;
      const int dy = y - center_y;
      if (dx * dx + dy * dy > radius_squared) {
        continue;
      }
      if (state->fog_mode == Level3DFogMode::kRaycast &&
          !HasLineOfSight(level, center_x, center_y, x, y)) {
        continue;
      }
      const std::size_t index = VisibilityIndex(*state, x, y);
      state->visibility_tiles[index] = kVisibilityVisible;
      state->visibility_current_indices.push_back(index);
    }
  }

  state->visibility_state_valid = true;
  state->visibility_last_center_x = center_x;
  state->visibility_last_center_y = center_y;
  state->visibility_last_radius_tiles = radius;
  state->visibility_last_enabled = state->visibility_enabled;
  state->visibility_last_memory_enabled = state->visibility_memory_enabled;
  state->visibility_last_fog_mode = state->fog_mode;
}

/**
 * @brief Executes the tile world center operation.
 */
Vector3 TileWorldCenter(const LevelData& level, int x, int y,
                        std::int8_t elevation, float tile_world_size,
                        float elevation_step) {
  const float origin_x = static_cast<float>(level.size.width) *
                         tile_world_size * 0.5F;
  const float origin_z = static_cast<float>(level.size.height) *
                         tile_world_size * 0.5F;
  return Vector3{(static_cast<float>(x) + 0.5F) * tile_world_size - origin_x,
                 static_cast<float>(elevation) * elevation_step,
                 (static_cast<float>(y) + 0.5F) * tile_world_size - origin_z};
}

/**
 * @brief Executes the terrain color 3D operation.
 */
Color TerrainColor3D(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kOpenGround:
      return Color{24, 186, 58, 255};
    case TerrainType::kForest:
      return Color{76, 65, 46, 255};
    case TerrainType::kRoad:
      return Color{222, 186, 111, 255};
    case TerrainType::kSwamp:
      return Color{35, 132, 86, 255};
    case TerrainType::kRuins:
      return Color{112, 107, 91, 255};
    case TerrainType::kWater:
      return Color{28, 99, 220, 255};
    case TerrainType::kWall:
      return Color{72, 58, 48, 255};
    case TerrainType::kUnknown:
      return Color{190, 65, 172, 255};
  }
  return Color{190, 65, 172, 255};
}

/**
 * @brief Returns a muted lower-floor material for open pit cells.
 */
Color PitFloorColor(const RuntimeCell& cell) {
  switch (cell.terrain) {
    case TerrainType::kRuins:
      return Color{86, 80, 72, 255};
    case TerrainType::kRoad:
      return Color{118, 94, 66, 255};
    case TerrainType::kSwamp:
      return Color{42, 90, 68, 255};
    case TerrainType::kWater:
      return Color{25, 72, 130, 255};
    case TerrainType::kForest:
      return Color{54, 64, 45, 255};
    case TerrainType::kWall:
      return Color{68, 56, 48, 255};
    case TerrainType::kOpenGround:
      return Color{44, 102, 52, 255};
    case TerrainType::kUnknown:
      return Color{126, 70, 116, 255};
  }
  return Color{86, 80, 72, 255};
}

/**
 * @brief Returns a plain earth cut color for vertical pit walls.
 */
Color PitCutWallColor(const RuntimeCell& upper_cell,
                      const RuntimeCell& lower_cell) {
  if (upper_cell.terrain == TerrainType::kRuins ||
      lower_cell.terrain == TerrainType::kRuins) {
    return Color{73, 65, 55, 255};
  }
  if (upper_cell.terrain == TerrainType::kWater ||
      lower_cell.terrain == TerrainType::kWater ||
      upper_cell.terrain == TerrainType::kSwamp ||
      lower_cell.terrain == TerrainType::kSwamp) {
    return Color{48, 66, 52, 255};
  }
  return Color{70, 58, 46, 255};
}

/**
 * @brief Returns a visually quieter material for elevated cut faces.
 */
Color ElevationCutWallColor(const RuntimeCell& upper_cell,
                            const RuntimeCell& lower_cell) {
  if (lower_cell.height < 0) {
    return PitCutWallColor(upper_cell, lower_cell);
  }
  if (upper_cell.terrain == TerrainType::kRuins ||
      lower_cell.terrain == TerrainType::kRuins) {
    return Color{84, 77, 66, 255};
  }
  if (upper_cell.terrain == TerrainType::kWall ||
      lower_cell.terrain == TerrainType::kWall) {
    return Color{70, 58, 48, 255};
  }
  return ScaleColorRgb(TerrainColor3D(upper_cell.terrain), 0.42F, 255);
}

/**
 * @brief Returns the color used for elevation.
 */
Color ElevationColor(std::int8_t elevation) {
  switch (elevation) {
    case -1:
      return Color{80, 76, 88, 255};
    case 0:
      return Color{34, 150, 58, 255};
    case 1:
      return Color{92, 162, 68, 255};
    case 2:
      return Color{136, 146, 74, 255};
    case 3:
      return Color{168, 126, 78, 255};
    case 4:
      return Color{190, 102, 82, 255};
    default:
      return Color{214, 70, 180, 255};
  }
}

/**
 * @brief Returns the color used for collision.
 */
Color CollisionColor(const RuntimeCell& cell) {
  if (cell.collision) {
    return Color{82, 62, 47, 255};
  }
  if (!cell.walkable) {
    return Color{122, 78, 48, 255};
  }
  if (cell.blocks_vision) {
    return Color{67, 91, 54, 255};
  }
  return Color{33, 175, 58, 255};
}

/**
 * @brief Executes the scale color rgb operation.
 */
Color ScaleColorRgb(Color color, float scale, unsigned char alpha) {
  return Color{
      static_cast<unsigned char>(std::clamp(
          static_cast<int>(std::lround(static_cast<float>(color.r) * scale)),
          0, 255)),
      static_cast<unsigned char>(std::clamp(
          static_cast<int>(std::lround(static_cast<float>(color.g) * scale)),
          0, 255)),
      static_cast<unsigned char>(std::clamp(
          static_cast<int>(std::lround(static_cast<float>(color.b) * scale)),
          0, 255)),
      alpha};
}

/**
 * @brief Returns the color used for elevation wall.
 */
Color ElevationWallColor(const RuntimeCell& upper_cell,
                         const RuntimeCell& lower_cell,
                         Level3DRenderMode mode) {
  if (mode == Level3DRenderMode::kElevation) {
    return ScaleColorRgb(ElevationColor(upper_cell.height), 0.50F, 255);
  }
  if (mode == Level3DRenderMode::kCollision) {
    return Color{94, 72, 52, 255};
  }
  return ElevationCutWallColor(upper_cell, lower_cell);
}

/**
 * @brief Checks whether passable forest boundary is true.
 */
bool IsPassableForestBoundary(const RuntimeCell& cell) {
  return cell.terrain == TerrainType::kForest && cell.walkable &&
         !cell.collision && cell.movement_multiplier > 0.0F;
}

/**
 * @brief Returns the color used for tile.
 */
Color TileColor(const RuntimeCell& cell, Level3DRenderMode mode) {
  switch (mode) {
    case Level3DRenderMode::kTerrain:
      if (cell.height < 0) {
        return PitFloorColor(cell);
      }
      if (IsPassableForestBoundary(cell)) {
        return Color{38, 122, 55, 255};
      }
      return TerrainColor3D(cell.terrain);
    case Level3DRenderMode::kElevation:
      return ElevationColor(cell.height);
    case Level3DRenderMode::kCollision:
      return CollisionColor(cell);
  }
  return TerrainColor3D(cell.terrain);
}

/**
 * @brief Checks whether surface visible is true.
 */
bool IsSurfaceVisible(const RuntimeCell& cell) {
  return cell.height >= -1;
}

/**
 * @brief Returns the color used for transition.
 */
Color TransitionColor(ElevationTransitionType type) {
  switch (type) {
    case ElevationTransitionType::kRamp:
      return Color{128, 106, 72, 232};
    case ElevationTransitionType::kStairs:
      return Color{142, 132, 112, 236};
    case ElevationTransitionType::kHatch:
      return Color{92, 78, 104, 230};
    case ElevationTransitionType::kStep:
      return Color{122, 86, 58, 226};
    case ElevationTransitionType::kUnknown:
      return Color{118, 108, 86, 214};
  }
  return Color{118, 108, 86, 214};
}

/**
 * @brief Checks whether transition visible in range is true.
 */
bool IsTransitionVisibleInRange(const ElevationTransition& transition,
                                const TileRange3D& range) {
  const bool from_visible = transition.from_x >= range.min_x &&
                            transition.from_x <= range.max_x &&
                            transition.from_y >= range.min_y &&
                            transition.from_y <= range.max_y;
  const bool to_visible = transition.to_x >= range.min_x &&
                          transition.to_x <= range.max_x &&
                          transition.to_y >= range.min_y &&
                          transition.to_y <= range.max_y;
  return from_visible || to_visible;
}

/**
 * @brief Returns the normalized XZ direction between two tile centers.
 */
Vector3 TransitionDirectionXZ(Vector3 from_center, Vector3 to_center) {
  const float dx = to_center.x - from_center.x;
  const float dz = to_center.z - from_center.z;
  const float length = std::hypot(dx, dz);
  if (length <= 0.001F) {
    return Vector3{1.0F, 0.0F, 0.0F};
  }
  return Vector3{dx / length, 0.0F, dz / length};
}

/**
 * @brief Returns the perpendicular XZ direction for a transition.
 */
Vector3 TransitionPerpendicularXZ(Vector3 direction) {
  return Vector3{-direction.z, 0.0F, direction.x};
}

/**
 * @brief Adds a scaled XZ vector to a 3D point.
 */
Vector3 AddScaledXZ(Vector3 point, Vector3 vector, float scale) {
  point.x += vector.x * scale;
  point.z += vector.z * scale;
  return point;
}

/**
 * @brief Interpolates between two 3D points.
 */
Vector3 LerpVector3(Vector3 from, Vector3 to, float t) {
  return Vector3{from.x + (to.x - from.x) * t,
                 from.y + (to.y - from.y) * t,
                 from.z + (to.z - from.z) * t};
}

/**
 * @brief Draws a muted sloped ramp surface between two elevations.
 */
void DrawRampTransitionPrimitive(Vector3 from_center, Vector3 to_center,
                                 const Level3DViewState& state, Color color) {
  const Vector3 direction = TransitionDirectionXZ(from_center, to_center);
  const Vector3 perpendicular = TransitionPerpendicularXZ(direction);
  const float half_width = state.tile_world_size * 0.34F;
  const float end_inset = state.tile_world_size * 0.16F;

  Vector3 start = AddScaledXZ(from_center, direction, end_inset);
  Vector3 end = AddScaledXZ(to_center, direction, -end_inset);
  start.y += 0.055F;
  end.y += 0.055F;

  const Vector3 start_left = AddScaledXZ(start, perpendicular, half_width);
  const Vector3 start_right = AddScaledXZ(start, perpendicular, -half_width);
  const Vector3 end_left = AddScaledXZ(end, perpendicular, half_width);
  const Vector3 end_right = AddScaledXZ(end, perpendicular, -half_width);

  DrawTriangle3D(start_left, end_left, end_right, color);
  DrawTriangle3D(start_left, end_right, start_right, color);
  DrawTriangle3D(end_right, end_left, start_left, color);
  DrawTriangle3D(start_right, end_right, start_left, color);
}

/**
 * @brief Draws low-profile stair treads between two elevations.
 */
void DrawStairsTransitionPrimitive(Vector3 from_center, Vector3 to_center,
                                   const Level3DViewState& state, Color color) {
  const Vector3 direction = TransitionDirectionXZ(from_center, to_center);
  const bool horizontal = std::abs(direction.x) >= std::abs(direction.z);
  const float tread_long_axis = state.tile_world_size * 0.18F;
  const float tread_short_axis = state.tile_world_size * 0.68F;
  const float width = horizontal ? tread_long_axis : tread_short_axis;
  const float depth = horizontal ? tread_short_axis : tread_long_axis;

  constexpr int kStepCount = 4;
  for (int index = 0; index < kStepCount; ++index) {
    const float t = (static_cast<float>(index) + 0.5F) /
                    static_cast<float>(kStepCount);
    Vector3 center = LerpVector3(from_center, to_center, t);
    center.y += 0.075F;
    DrawCube(center, width, 0.055F, depth, color);
  }
}

/**
 * @brief Draws a small Space-step ledge marker at the high side of a transition.
 */
void DrawStepTransitionPrimitive(Vector3 from_center, Vector3 to_center,
                                 const Level3DViewState& state, Color color) {
  const Vector3 direction = TransitionDirectionXZ(from_center, to_center);
  const bool horizontal = std::abs(direction.x) >= std::abs(direction.z);
  const float width = horizontal ? state.tile_world_size * 0.20F
                                 : state.tile_world_size * 0.62F;
  const float depth = horizontal ? state.tile_world_size * 0.62F
                                 : state.tile_world_size * 0.20F;
  Vector3 center = LerpVector3(from_center, to_center, 0.5F);
  center.y = std::max(from_center.y, to_center.y) + 0.065F;
  DrawCube(center, width, 0.08F, depth, color);
}

/**
 * @brief Draws transition primitive.
 */
bool DrawTransitionPrimitive(const LevelData& level,
                             const ElevationTransition& transition,
                             const Level3DViewState& state) {
  const RuntimeCell* from = CellAt(level, transition.from_x, transition.from_y);
  const RuntimeCell* to = CellAt(level, transition.to_x, transition.to_y);
  if (from == nullptr || to == nullptr || !IsSurfaceVisible(*from) ||
      !IsSurfaceVisible(*to)) {
    return false;
  }
  if (from->height < 0 || to->height < 0) {
    return false;
  }

  const Vector3 from_center = TileWorldCenter(level, transition.from_x,
                                              transition.from_y, from->height,
                                              state.tile_world_size,
                                              state.elevation_step);
  const Vector3 to_center = TileWorldCenter(level, transition.to_x,
                                            transition.to_y, to->height,
                                            state.tile_world_size,
                                            state.elevation_step);
  const float dx = to_center.x - from_center.x;
  const float dz = to_center.z - from_center.z;
  const float distance = std::hypot(dx, dz);
  if (distance <= 0.001F) {
    return false;
  }

  const int color_tile_x = IsTileRenderable(state, transition.from_x,
                                            transition.from_y)
                               ? transition.from_x
                               : transition.to_x;
  const int color_tile_y = IsTileRenderable(state, transition.from_x,
                                            transition.from_y)
                               ? transition.from_y
                               : transition.to_y;
  const Color color = ApplyVisibilityColor(TransitionColor(transition.type),
                                           state, color_tile_x, color_tile_y);

  switch (transition.type) {
    case ElevationTransitionType::kRamp:
      DrawRampTransitionPrimitive(from_center, to_center, state, color);
      return true;
    case ElevationTransitionType::kStairs:
      DrawStairsTransitionPrimitive(from_center, to_center, state, color);
      return true;
    case ElevationTransitionType::kStep:
      DrawStepTransitionPrimitive(from_center, to_center, state, color);
      return true;
    case ElevationTransitionType::kHatch:
    case ElevationTransitionType::kUnknown:
      break;
  }

  const Vector3 center{(from_center.x + to_center.x) * 0.5F,
                       (from_center.y + to_center.y) * 0.5F + 0.055F,
                       (from_center.z + to_center.z) * 0.5F};
  const bool horizontal = std::abs(dx) >= std::abs(dz);
  const float long_axis = std::max(state.tile_world_size * 0.84F,
                                   distance + state.tile_world_size * 0.12F);
  const float short_axis = state.tile_world_size * 0.24F;
  const float width = horizontal ? long_axis : short_axis;
  const float depth = horizontal ? short_axis : long_axis;
  DrawCube(center, width, 0.055F, depth, color);
  return true;
}

/**
 * @brief Draws elevation transitions.
 */
void DrawElevationTransitions(const LevelData& level,
                              const Level3DViewState& state,
                              const TileRange3D& range,
                              Level3DPerfStats* stats) {
  for (const ElevationTransition& transition : level.elevation_transitions) {
    if (!IsTransitionVisibleInRange(transition, range)) {
      continue;
    }
    if (!IsTileRenderable(state, transition.from_x, transition.from_y) &&
        !IsTileRenderable(state, transition.to_x, transition.to_y)) {
      continue;
    }
    if (stats != nullptr) {
      ++stats->transition_candidates;
    }
    if (DrawTransitionPrimitive(level, transition, state) &&
        stats != nullptr) {
      ++stats->transitions_drawn;
    }
  }
}

/**
 * @brief Executes the blocking volume height operation.
 */
float BlockingVolumeHeight(const RuntimeCell& cell) {
  const bool has_blocking_runtime_semantics =
      cell.collision || cell.blocks_vision || !cell.walkable ||
      cell.movement_multiplier <= 0.0F;

  if (cell.terrain == TerrainType::kForest) {
    return 1.2F;
  }
  if (!has_blocking_runtime_semantics) {
    return 0.0F;
  }
  if (cell.terrain == TerrainType::kWall || cell.terrain == TerrainType::kRuins) {
    return 0.8F;
  }
  return 0.55F;
}

/**
 * @brief Draws ground tile.
 */
bool DrawGroundTile(const LevelData& level, int x, int y,
                    const RuntimeCell& cell, const Level3DViewState& state) {
  if (!IsSurfaceVisible(cell)) {
    return false;
  }

  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  const float slab_height = 0.045F;
  if (cell.terrain == TerrainType::kWater) {
    center.y -= 0.055F;
  }

  DrawCube(center, state.tile_world_size, slab_height, state.tile_world_size,
           ApplyVisibilityColor(TileColor(cell, state.mode), state, x, y));
  return true;
}

/**
 * @brief Draws elevation wall to neighbor.
 */
int DrawElevationWallToNeighbor(const LevelData& level, int x, int y,
                                 int neighbor_x, int neighbor_y,
                                 const RuntimeCell& cell,
                                 const Level3DViewState& state) {
  if (!IsSurfaceVisible(cell) || !IsTileRenderable(state, x, y)) {
    return 0;
  }

  const RuntimeCell* neighbor = CellAt(level, neighbor_x, neighbor_y);
  if (neighbor == nullptr || !IsSurfaceVisible(*neighbor)) {
    return 0;
  }

  const int height_delta = static_cast<int>(cell.height) -
                           static_cast<int>(neighbor->height);
  if (height_delta <= 0) {
    return 0;
  }

  const float wall_height = static_cast<float>(height_delta) *
                            state.elevation_step;
  if (wall_height <= 0.01F) {
    return 0;
  }

  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  const float neighbor_y_world = static_cast<float>(neighbor->height) *
                                 state.elevation_step;
  center.y = neighbor_y_world + wall_height * 0.5F;

  const bool pit_cut = neighbor->height < 0;
  float width = state.tile_world_size;
  float depth = pit_cut ? std::max(0.16F, state.elevation_wall_thickness)
                        : std::max(0.02F, state.elevation_wall_thickness);
  if (neighbor_x < x) {
    center.x -= state.tile_world_size * 0.5F;
    width = depth;
    depth = state.tile_world_size;
  } else if (neighbor_x > x) {
    center.x += state.tile_world_size * 0.5F;
    width = depth;
    depth = state.tile_world_size;
  } else if (neighbor_y < y) {
    center.z -= state.tile_world_size * 0.5F;
  } else {
    center.z += state.tile_world_size * 0.5F;
  }

  DrawCube(center, width, wall_height, depth,
           ApplyVisibilityColor(ElevationWallColor(cell, *neighbor, state.mode),
                                state, x, y));
  return 1;
}

/**
 * @brief Draws elevation walls.
 */
int DrawElevationWalls(const LevelData& level, int x, int y,
                        const RuntimeCell& cell,
                        const Level3DViewState& state) {
  int count = 0;
  count += DrawElevationWallToNeighbor(level, x, y, x - 1, y, cell, state);
  count += DrawElevationWallToNeighbor(level, x, y, x + 1, y, cell, state);
  count += DrawElevationWallToNeighbor(level, x, y, x, y - 1, cell, state);
  count += DrawElevationWallToNeighbor(level, x, y, x, y + 1, cell, state);
  return count;
}

/**
 * @brief Draws blocking volume.
 */
bool DrawBlockingVolume(const LevelData& level, int x, int y,
                        const RuntimeCell& cell,
                        const Level3DViewState& state) {
  if (IsPassableForestBoundary(cell) || !IsTileRenderable(state, x, y)) {
    return false;
  }

  const float height = BlockingVolumeHeight(cell);
  if (height <= 0.0F || !IsSurfaceVisible(cell)) {
    return false;
  }

  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  center.y += height * 0.5F;
  const float width = state.tile_world_size * 0.95F;
  const float depth = state.tile_world_size * 0.95F;
  Color color = cell.terrain == TerrainType::kForest
                    ? Color{75, 61, 43, 235}
                    : Color{72, 62, 52, 235};
  if (state.mode == Level3DRenderMode::kCollision) {
    color = Color{105, 77, 54, 238};
  }
  DrawCube(center, width, height, depth,
           ApplyVisibilityColor(color, state, x, y));
  return true;
}

/**
 * @brief Draws passable forest boundary volume.
 */
bool DrawPassableForestBoundaryVolume(const LevelData& level, int x, int y,
                                      const RuntimeCell& cell,
                                      const Level3DViewState& state) {
  if (!IsPassableForestBoundary(cell) || !IsSurfaceVisible(cell) ||
      !IsTileRenderable(state, x, y)) {
    return false;
  }

  const float height = 0.62F;
  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  center.y += height * 0.5F;
  const float width = state.tile_world_size * 0.92F;
  const float depth = state.tile_world_size * 0.92F;
  Color color = Color{54, 103, 48, 92};
  if (state.mode == Level3DRenderMode::kCollision) {
    color = Color{44, 146, 58, 104};
  }

  const Color visible_color = ApplyVisibilityColor(color, state, x, y);
  DrawCube(center, width, height, depth, visible_color);
  DrawCubeWires(center, width, height, depth,
                ApplyVisibilityColor(Color{118, 166, 95, 112}, state, x, y));
  return true;
}

/**
 * @brief Draws level bounds.
 */
void DrawLevelBounds(const LevelData& level, const Level3DViewState& state) {
  if (state.visibility_enabled) {
    return;
  }
  const float width = static_cast<float>(level.size.width) *
                      state.tile_world_size;
  const float height = static_cast<float>(level.size.height) *
                       state.tile_world_size;
  const Vector3 center{0.0F, 0.02F, 0.0F};
  DrawCubeWires(center, width, 0.04F, height, Color{150, 155, 170, 180});
}

/**
 * @brief Returns for each active renderable tile.
 */
template <typename DrawFunction>
/**
 * @brief Returns for each active renderable tile.
 */
void ForEachActiveRenderableTile(const LevelData& level,
                                 const Level3DViewState& state,
                                 DrawFunction draw_function) {
  const ChunkRange3D chunks = ActiveChunkRange(level, state);
  const int chunk_size = SafeChunkSize(state);
  for (int chunk_y = chunks.min_y; chunk_y <= chunks.max_y; ++chunk_y) {
    const int min_y = std::clamp(chunk_y * chunk_size, 0, level.size.height - 1);
    const int max_y = std::clamp((chunk_y + 1) * chunk_size - 1, 0,
                                 level.size.height - 1);
    for (int chunk_x = chunks.min_x; chunk_x <= chunks.max_x; ++chunk_x) {
      const int min_x = std::clamp(chunk_x * chunk_size, 0,
                                   level.size.width - 1);
      const int max_x = std::clamp((chunk_x + 1) * chunk_size - 1, 0,
                                   level.size.width - 1);
      for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
          if (!IsTileRenderable(state, x, y)) {
            continue;
          }
          const RuntimeCell* cell = CellAt(level, x, y);
          if (cell == nullptr) {
            continue;
          }
          draw_function(x, y, *cell);
        }
      }
    }
  }
}

/**
 * @brief Counts active tile candidates and visible/renderable tiles for diagnostics.
 */
void CountRenderableTilesInRange(const LevelData& level,
                                 const Level3DViewState& state,
                                 const TileRange3D& range,
                                 Level3DPerfStats* stats) {
  if (stats == nullptr) {
    return;
  }

  stats->active_tile_candidates =
      std::max(0, range.max_x - range.min_x + 1) *
      std::max(0, range.max_y - range.min_y + 1);
  stats->renderable_tiles = 0;

  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      if (CellAt(level, x, y) != nullptr && IsTileRenderable(state, x, y)) {
        ++stats->renderable_tiles;
      }
    }
  }
}

/**
 * @brief Returns elevation debug overlay color for a runtime cell.
 */
Color ElevationDebugTileOverlayColor(const RuntimeCell& cell) {
  if (cell.collision) {
    return Color{235, 70, 58, 96};
  }
  if (!cell.walkable || cell.movement_multiplier <= 0.0F) {
    return Color{214, 104, 44, 78};
  }
  if (cell.height < 0) {
    return Color{72, 128, 255, 92};
  }
  if (cell.height > 0) {
    const int alpha = std::clamp(44 + static_cast<int>(cell.height) * 16,
                                 44, 120);
    return Color{255, 216, 72, static_cast<unsigned char>(alpha)};
  }
  return Color{0, 0, 0, 0};
}

/**
 * @brief Draws one debug tile overlay slab.
 */
bool DrawElevationDebugTileSlab(const LevelData& level, int x, int y,
                                const RuntimeCell& cell,
                                const Level3DViewState& state, Color color) {
  if (color.a == 0 || !IsSurfaceVisible(cell) || !IsTileRenderable(state, x, y)) {
    return false;
  }
  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  center.y += 0.085F;
  const float size = state.tile_world_size * 0.84F;
  DrawCube(center, size, 0.035F, size, ApplyVisibilityColor(color, state, x, y));
  return true;
}

/**
 * @brief Draws debug markers for explicit elevation transitions.
 */
void DrawElevationDebugTransitionMarkers(const LevelData& level,
                                         const Level3DViewState& state,
                                         const TileRange3D& range) {
  for (const ElevationTransition& transition : level.elevation_transitions) {
    if (!IsTransitionVisibleInRange(transition, range)) {
      continue;
    }
    const RuntimeCell* from = CellAt(level, transition.from_x, transition.from_y);
    const RuntimeCell* to = CellAt(level, transition.to_x, transition.to_y);
    if (from == nullptr || to == nullptr || !IsSurfaceVisible(*from) ||
        !IsSurfaceVisible(*to)) {
      continue;
    }
    if (!IsTileRenderable(state, transition.from_x, transition.from_y) &&
        !IsTileRenderable(state, transition.to_x, transition.to_y)) {
      continue;
    }

    Vector3 from_center = TileWorldCenter(level, transition.from_x,
                                          transition.from_y, from->height,
                                          state.tile_world_size,
                                          state.elevation_step);
    Vector3 to_center = TileWorldCenter(level, transition.to_x,
                                        transition.to_y, to->height,
                                        state.tile_world_size,
                                        state.elevation_step);
    from_center.y += 0.22F;
    to_center.y += 0.22F;
    const int color_x = IsTileRenderable(state, transition.from_x,
                                         transition.from_y)
                            ? transition.from_x
                            : transition.to_x;
    const int color_y = IsTileRenderable(state, transition.from_x,
                                         transition.from_y)
                            ? transition.from_y
                            : transition.to_y;
    const Color color = ApplyVisibilityColor(TransitionColor(transition.type),
                                             state, color_x, color_y);
    DrawLine3D(from_center, to_center, color);
    DrawCube(from_center, 0.18F, 0.18F, 0.18F, color);
    DrawCube(to_center, 0.18F, 0.18F, 0.18F, color);
  }
}

/**
 * @brief Draws optional elevation diagnostics in the 3D world.
 */
void DrawElevationDebugOverlay3D(const LevelData& level,
                                 const Level3DViewState& state,
                                 const TileRange3D& range,
                                 Level3DPerfStats* stats) {
  if (!state.debug_elevation_overlay_enabled) {
    return;
  }

  BeginBlendMode(BLEND_ALPHA);
  ForEachActiveRenderableTile(
      level, state, [&level, &state, stats](int x, int y, const RuntimeCell& cell) {
        if (DrawElevationDebugTileSlab(level, x, y, cell, state,
                                      ElevationDebugTileOverlayColor(cell)) &&
            stats != nullptr) {
          ++stats->debug_overlay_slabs_drawn;
        }
      });

  const int player_x = TileIndexFromPosition(state.player.tile_x);
  const int player_y = TileIndexFromPosition(state.player.tile_y);
  const RuntimeCell* player_cell = CellAt(level, player_x, player_y);
  if (player_cell != nullptr) {
    if (DrawElevationDebugTileSlab(level, player_x, player_y, *player_cell,
                                    state, Color{255, 235, 48, 160}) &&
        stats != nullptr) {
      ++stats->debug_overlay_slabs_drawn;
    }
  }

  const Level3DTargetTileDiagnostics target =
      FacingLevel3DTargetTileDiagnostics(level, state.player);
  const RuntimeCell* target_cell = CellAt(level, target.target_tile_x,
                                          target.target_tile_y);
  if (target_cell != nullptr) {
    const Color color = target.can_enter
                            ? Color{72, 255, 124, 150}
                            : Color{255, 96, 54, 160};
    if (DrawElevationDebugTileSlab(level, target.target_tile_x,
                                    target.target_tile_y, *target_cell, state,
                                    color) &&
        stats != nullptr) {
      ++stats->debug_overlay_slabs_drawn;
    }
  }

  DrawElevationDebugTransitionMarkers(level, state, range);
  EndBlendMode();
}

/**
 * @brief Draws player.
 */
void DrawPlayer(const LevelData& level, const Level3DViewState& state) {
  Vector3 position = Level3DPlayerWorldPosition(level, state.player,
                                                state.tile_world_size,
                                                state.elevation_step);
  position.y += 0.28F;
  DrawCube(position, 0.34F, 0.56F, 0.34F, Color{246, 204, 38, 255});
  DrawCubeWires(position, 0.34F, 0.56F, 0.34F, Color{80, 62, 18, 255});

  const Vector3 vertical_end{position.x, position.y + 3.0F, position.z};
  DrawLine3D(position, vertical_end, Color{246, 204, 38, 200});

  const Vector3 facing_end{
      position.x + state.player.facing_x * state.tile_world_size * 1.4F,
      position.y + 0.18F,
      position.z + state.player.facing_y * state.tile_world_size * 1.4F};
  DrawLine3D(position, facing_end, Color{255, 146, 28, 255});
  DrawCube(facing_end, 0.18F, 0.18F, 0.18F, Color{255, 146, 28, 255});
}

/**
 * @brief Draws tiles.
 */
void DrawTiles(const LevelData& level, const Level3DViewState& state,
               Level3DPerfStats* stats) {
  const ChunkRange3D chunks = ActiveChunkRange(level, state);
  const TileRange3D range = TileRangeFromChunks(level, state, chunks);

  if (stats != nullptr) {
    stats->active_chunks_x = chunks.max_x - chunks.min_x + 1;
    stats->active_chunks_y = chunks.max_y - chunks.min_y + 1;
    stats->active_chunks_total = stats->active_chunks_x * stats->active_chunks_y;
    CountRenderableTilesInRange(level, state, range, stats);
  }

  ForEachActiveRenderableTile(
      level, state, [&level, &state, stats](int x, int y, const RuntimeCell& cell) {
        if (DrawGroundTile(level, x, y, cell, state) && stats != nullptr) {
          ++stats->ground_tiles_drawn;
        }
      });

  DrawElevationTransitions(level, state, range, stats);

  ForEachActiveRenderableTile(
      level, state, [&level, &state, stats](int x, int y, const RuntimeCell& cell) {
        if (stats != nullptr) {
          stats->elevation_wall_faces_drawn +=
              DrawElevationWalls(level, x, y, cell, state);
          return;
        }
        DrawElevationWalls(level, x, y, cell, state);
      });

  ForEachActiveRenderableTile(
      level, state, [&level, &state, stats](int x, int y, const RuntimeCell& cell) {
        if (DrawBlockingVolume(level, x, y, cell, state) && stats != nullptr) {
          ++stats->blocking_volumes_drawn;
        }
      });

  BeginBlendMode(BLEND_ALPHA);
  ForEachActiveRenderableTile(
      level, state, [&level, &state, stats](int x, int y, const RuntimeCell& cell) {
        if (DrawPassableForestBoundaryVolume(level, x, y, cell, state) &&
            stats != nullptr) {
          ++stats->forest_boundary_volumes_drawn;
          ++stats->forest_boundary_wireframes_drawn;
        }
      });
  EndBlendMode();

  DrawElevationDebugOverlay3D(level, state, range, stats);
}

/**
 * @brief Draws the F8 render3d performance diagnostics overlay.
 */
void DrawRender3DPerfOverlay(const Level3DPerfStats& stats,
                             const WindowState& window) {
  const int font_size = std::max(10, static_cast<int>(12.0F * window.ui_scale));
  const int line_step = font_size + std::max(2, static_cast<int>(4.0F * window.ui_scale));
  const int x = std::max(8, static_cast<int>(16.0F * window.ui_scale));
  int y = std::max(96, static_cast<int>(112.0F * window.ui_scale));
  const int width = std::max(520, static_cast<int>(590.0F * window.ui_scale));
  const int height = line_step * 10 + std::max(12, static_cast<int>(16.0F * window.ui_scale));
  const Color text = Color{228, 232, 218, 255};
  const Color title = Color{255, 220, 96, 255};

  DrawRectangle(x - 8, y - 8, width, height, Color{12, 12, 18, 194});
  DrawText("RENDER3D PERF DEBUG  F8 toggle", x, y, font_size, title);
  y += line_step;
  DrawText(TextFormat("fps=%d frame=%.2fms visibility_update=%.3fms",
                      stats.fps, stats.frame_ms, stats.visibility_update_ms),
           x, y, font_size, text);
  y += line_step;
  DrawText(TextFormat("chunks=%dx%d total=%d tile_candidates=%d renderable=%d",
                      stats.active_chunks_x, stats.active_chunks_y,
                      stats.active_chunks_total, stats.active_tile_candidates,
                      stats.renderable_tiles),
           x, y, font_size, text);
  y += line_step;
  DrawText(TextFormat("ground=%d walls=%d blockers=%d forest=%d forest_wires=%d",
                      stats.ground_tiles_drawn,
                      stats.elevation_wall_faces_drawn,
                      stats.blocking_volumes_drawn,
                      stats.forest_boundary_volumes_drawn,
                      stats.forest_boundary_wireframes_drawn),
           x, y, font_size, text);
  y += line_step;
  DrawText(TextFormat("transitions considered=%d drawn=%d debug_slabs=%d",
                      stats.transition_candidates, stats.transitions_drawn,
                      stats.debug_overlay_slabs_drawn),
           x, y, font_size, text);
  y += line_step;
  DrawText(TextFormat("estimated primitive submissions=%d",
                      stats.EstimatedPrimitiveSubmissions()),
           x, y, font_size, text);
}

}  // namespace

/**
 * @brief Implements Level3DPerfStats::EstimatedPrimitiveSubmissions.
 */
int Level3DPerfStats::EstimatedPrimitiveSubmissions() const {
  return ground_tiles_drawn + transitions_drawn + elevation_wall_faces_drawn +
         blocking_volumes_drawn + forest_boundary_volumes_drawn +
         forest_boundary_wireframes_drawn + debug_overlay_slabs_drawn;
}

/**
 * @brief Returns level 3D fog mode name.
 */
const char* Level3DFogModeName(Level3DFogMode mode) {
  switch (mode) {
    case Level3DFogMode::kCircle:
      return "circle";
    case Level3DFogMode::kRaycast:
      return "raycast";
  }

  return "circle";
}

/**
 * @brief Returns level 3D render mode name.
 */
const char* Level3DRenderModeName(Level3DRenderMode mode) {
  switch (mode) {
    case Level3DRenderMode::kTerrain:
      return "terrain";
    case Level3DRenderMode::kElevation:
      return "elevation";
    case Level3DRenderMode::kCollision:
      return "collision";
  }
  return "terrain";
}

/**
 * @brief Initializes level 3D view.
 */
void InitializeLevel3DView(const LevelData& level, Level3DViewState* state) {
  if (state == nullptr) {
    return;
  }
  InitializeLevel3DPlayer(level, &state->player);
  InitializeLevel3DCamera(level, &state->camera);
  state->mode = Level3DRenderMode::kTerrain;
  state->culling_center_initialized = false;
  UpdateCullingCenter(level, state);
  UpdateVisibilityState(level, state);
  state->initialized = true;
}

/**
 * @brief Updates level 3D view for the current frame.
 */
void UpdateLevel3DView(const LevelData& level, const InputState& input,
                       float dt, Level3DViewState* state) {
  if (state == nullptr || !state->initialized) {
    return;
  }

  if (input.debug_view_raw_pressed) {
    state->mode = Level3DRenderMode::kTerrain;
  }
  if (input.debug_view_analysis_pressed) {
    state->mode = Level3DRenderMode::kElevation;
  }
  if (input.debug_view_visual_pressed) {
    state->mode = Level3DRenderMode::kCollision;
  }
  if (input.debug_elevation_overlay_pressed) {
    state->debug_elevation_overlay_enabled =
        !state->debug_elevation_overlay_enabled;
  }
  if (input.debug_elevation_logs_pressed) {
    state->debug_elevation_move_logs_enabled =
        !state->debug_elevation_move_logs_enabled;
  }
  if (input.debug_render3d_perf_pressed) {
    state->debug_render3d_perf_enabled = !state->debug_render3d_perf_enabled;
  }

  bool skip_intro_this_frame = false;
  if (Level3DCameraIntroSkipRequested(state->camera, input)) {
    SkipLevel3DCameraIntro(&state->camera);
    skip_intro_this_frame = true;
  }

  if (!Level3DCameraIntroLocksPlayer(state->camera) &&
      !skip_intro_this_frame) {
    UpdateLevel3DPlayer(level, input, dt, &state->player);
  }
  UpdateCullingCenter(level, state);
  if (state->debug_render3d_perf_enabled) {
    const double visibility_start = GetTime();
    UpdateVisibilityState(level, state);
    state->last_visibility_update_ms = (GetTime() - visibility_start) * 1000.0;
  } else {
    UpdateVisibilityState(level, state);
    state->last_visibility_update_ms = 0.0;
  }
  UpdateLevel3DCamera(level, state->player, input, dt, state->tile_world_size,
                      state->elevation_step, &state->camera);
}

/**
 * @brief Returns level 3D view state to string.
 */
std::string Level3DViewStateToString(const Level3DViewState& state) {
  std::ostringstream stream;
  stream << "renderer3d: mode=" << Level3DRenderModeName(state.mode)
         << " radius=" << state.visible_radius_tiles
         << " chunks=" << state.chunk_size_tiles << "x"
         << state.chunk_size_tiles << "/r" << state.active_chunk_radius
         << " visibility=" << (state.visibility_enabled ? "on" : "off")
         << "/r" << state.visibility_radius_tiles
         << "/memory=" << (state.visibility_memory_enabled ? "on" : "off")
         << "/fog=" << Level3DFogModeName(state.fog_mode)
         << " elevation_debug="
         << (state.debug_elevation_overlay_enabled ? "on" : "off")
         << "/logs="
         << (state.debug_elevation_move_logs_enabled ? "on" : "off")
         << " render3d_perf="
         << (state.debug_render3d_perf_enabled ? "on" : "off")
         << " cull_center=" << state.culling_center_tile_x << ','
         << state.culling_center_tile_y
         << " deadzone=" << state.culling_deadzone_tiles << "  "
         << Level3DPlayerStateToString(state.player) << "  "
         << Level3DCameraStateToString(state.camera);
  return stream.str();
}

/**
 * @brief Draws runtime visuals.
 */
void Level3DRenderer::Draw(const LevelData& level,
                           const Level3DViewState& state,
                           const WindowState& window) const {
  if (!state.initialized || level.size.width <= 0 || level.size.height <= 0 ||
      level.cells.empty()) {
    return;
  }

  const Camera3D camera = BuildLevel3DCamera(
      level, state.player, state.camera, window, state.tile_world_size,
      state.elevation_step);

  Level3DPerfStats stats;
  Level3DPerfStats* stats_ptr = nullptr;
  if (state.debug_render3d_perf_enabled) {
    stats.fps = GetFPS();
    stats.frame_ms = static_cast<double>(GetFrameTime()) * 1000.0;
    stats.visibility_update_ms = state.last_visibility_update_ms;
    stats_ptr = &stats;
  }

  BeginMode3D(camera);
  DrawTiles(level, state, stats_ptr);
  DrawLevelBounds(level, state);
  DrawPlayer(level, state);
  EndMode3D();

  if (state.debug_render3d_perf_enabled) {
    DrawRender3DPerfOverlay(stats, window);
  }
}

}  // namespace sar::render3d
