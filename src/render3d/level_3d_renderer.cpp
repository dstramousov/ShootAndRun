#include "render3d/level_3d_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>

#include "level/terrain_type.h"

namespace sar::render3d {
namespace {

struct TileRange3D {
  int min_x = 0;
  int max_x = 0;
  int min_y = 0;
  int max_y = 0;
};

int TileIndexFromPosition(float value) {
  return static_cast<int>(std::floor(value));
}

int TileIndex(const LevelData& level, int x, int y) {
  return y * level.size.width + x;
}

bool IsInsideMap(const LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

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

TileRange3D VisibleTileRange(const LevelData& level,
                             const Level3DViewState& state) {
  const int center_x = state.culling_center_initialized
                           ? state.culling_center_tile_x
                           : TileIndexFromPosition(state.player.tile_x);
  const int center_y = state.culling_center_initialized
                           ? state.culling_center_tile_y
                           : TileIndexFromPosition(state.player.tile_y);
  const int radius = std::clamp(state.visible_radius_tiles, 8, 128);
  return TileRange3D{std::clamp(center_x - radius, 0, level.size.width - 1),
                     std::clamp(center_x + radius, 0, level.size.width - 1),
                     std::clamp(center_y - radius, 0, level.size.height - 1),
                     std::clamp(center_y + radius, 0, level.size.height - 1)};
}

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

Color ElevationColor(std::int8_t elevation) {
  switch (elevation) {
    case -1:
      return Color{78, 58, 112, 255};
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

Color ElevationWallColor(const RuntimeCell& cell, Level3DRenderMode mode) {
  if (mode == Level3DRenderMode::kElevation) {
    return ScaleColorRgb(ElevationColor(cell.height), 0.58F, 255);
  }
  if (mode == Level3DRenderMode::kCollision) {
    return Color{94, 72, 52, 255};
  }
  return ScaleColorRgb(TerrainColor3D(cell.terrain), 0.52F, 255);
}

bool IsPassableForestBoundary(const RuntimeCell& cell) {
  return cell.terrain == TerrainType::kForest && cell.walkable &&
         !cell.collision && cell.movement_multiplier > 0.0F;
}

Color TileColor(const RuntimeCell& cell, Level3DRenderMode mode) {
  switch (mode) {
    case Level3DRenderMode::kTerrain:
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

bool IsSurfaceVisible(const RuntimeCell& cell) {
  return cell.height >= 0;
}

float BlockingVolumeHeight(const RuntimeCell& cell) {
  if (cell.terrain == TerrainType::kForest) {
    return 1.2F;
  }
  if (cell.terrain == TerrainType::kWall || cell.terrain == TerrainType::kRuins) {
    return 0.8F;
  }
  if (cell.collision || cell.blocks_vision) {
    return 0.55F;
  }
  return 0.0F;
}

void DrawGroundTile(const LevelData& level, int x, int y,
                    const RuntimeCell& cell, const Level3DViewState& state) {
  if (!IsSurfaceVisible(cell)) {
    return;
  }

  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  const float slab_height = 0.045F;
  if (cell.terrain == TerrainType::kWater) {
    center.y -= 0.055F;
  }

  DrawCube(center, state.tile_world_size, slab_height, state.tile_world_size,
           TileColor(cell, state.mode));
}

void DrawElevationWallToNeighbor(const LevelData& level, int x, int y,
                                 int neighbor_x, int neighbor_y,
                                 const RuntimeCell& cell,
                                 const Level3DViewState& state) {
  if (!IsSurfaceVisible(cell) || cell.height <= 0) {
    return;
  }

  const RuntimeCell* neighbor = CellAt(level, neighbor_x, neighbor_y);
  if (neighbor == nullptr || !IsSurfaceVisible(*neighbor)) {
    return;
  }

  const int height_delta = static_cast<int>(cell.height) -
                           static_cast<int>(neighbor->height);
  if (height_delta <= 0) {
    return;
  }

  const float wall_height = static_cast<float>(height_delta) *
                            state.elevation_step;
  if (wall_height <= 0.01F) {
    return;
  }

  Vector3 center = TileWorldCenter(level, x, y, cell.height,
                                   state.tile_world_size,
                                   state.elevation_step);
  const float neighbor_y_world = static_cast<float>(neighbor->height) *
                                 state.elevation_step;
  center.y = neighbor_y_world + wall_height * 0.5F;

  float width = state.tile_world_size;
  float depth = std::max(0.02F, state.elevation_wall_thickness);
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
           ElevationWallColor(cell, state.mode));
}

void DrawElevationWalls(const LevelData& level, int x, int y,
                        const RuntimeCell& cell,
                        const Level3DViewState& state) {
  DrawElevationWallToNeighbor(level, x, y, x - 1, y, cell, state);
  DrawElevationWallToNeighbor(level, x, y, x + 1, y, cell, state);
  DrawElevationWallToNeighbor(level, x, y, x, y - 1, cell, state);
  DrawElevationWallToNeighbor(level, x, y, x, y + 1, cell, state);
}

void DrawBlockingVolume(const LevelData& level, int x, int y,
                        const RuntimeCell& cell,
                        const Level3DViewState& state) {
  if (IsPassableForestBoundary(cell)) {
    return;
  }

  const float height = BlockingVolumeHeight(cell);
  if (height <= 0.0F || !IsSurfaceVisible(cell)) {
    return;
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
  DrawCube(center, width, height, depth, color);
}

void DrawPassableForestBoundaryVolume(const LevelData& level, int x, int y,
                                      const RuntimeCell& cell,
                                      const Level3DViewState& state) {
  if (!IsPassableForestBoundary(cell) || !IsSurfaceVisible(cell)) {
    return;
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

  DrawCube(center, width, height, depth, color);
  DrawCubeWires(center, width, height, depth, Color{118, 166, 95, 112});
}

void DrawLevelBounds(const LevelData& level, float tile_world_size) {
  const float width = static_cast<float>(level.size.width) * tile_world_size;
  const float height = static_cast<float>(level.size.height) * tile_world_size;
  const Vector3 center{0.0F, 0.02F, 0.0F};
  DrawCubeWires(center, width, 0.04F, height, Color{150, 155, 170, 180});
}

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

void DrawTiles(const LevelData& level, const Level3DViewState& state) {
  const TileRange3D range = VisibleTileRange(level, state);
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const RuntimeCell* cell = CellAt(level, x, y);
      if (cell == nullptr) {
        continue;
      }
      DrawGroundTile(level, x, y, *cell, state);
    }
  }

  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const RuntimeCell* cell = CellAt(level, x, y);
      if (cell == nullptr) {
        continue;
      }
      DrawElevationWalls(level, x, y, *cell, state);
    }
  }

  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const RuntimeCell* cell = CellAt(level, x, y);
      if (cell == nullptr) {
        continue;
      }
      DrawBlockingVolume(level, x, y, *cell, state);
    }
  }

  BeginBlendMode(BLEND_ALPHA);
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const RuntimeCell* cell = CellAt(level, x, y);
      if (cell == nullptr) {
        continue;
      }
      DrawPassableForestBoundaryVolume(level, x, y, *cell, state);
    }
  }
  EndBlendMode();
}

}  // namespace

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

void InitializeLevel3DView(const LevelData& level, Level3DViewState* state) {
  if (state == nullptr) {
    return;
  }
  InitializeLevel3DPlayer(level, &state->player);
  InitializeLevel3DCamera(level, &state->camera);
  state->mode = Level3DRenderMode::kTerrain;
  state->culling_center_initialized = false;
  UpdateCullingCenter(level, state);
  state->initialized = true;
}

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

  UpdateLevel3DPlayer(level, input, dt, &state->player);
  UpdateCullingCenter(level, state);
  UpdateLevel3DCamera(level, state->player, input, dt, state->tile_world_size,
                      state->elevation_step, &state->camera);
}

std::string Level3DViewStateToString(const Level3DViewState& state) {
  std::ostringstream stream;
  stream << "renderer3d: mode=" << Level3DRenderModeName(state.mode)
         << " radius=" << state.visible_radius_tiles
         << " cull_center=" << state.culling_center_tile_x << ','
         << state.culling_center_tile_y
         << " deadzone=" << state.culling_deadzone_tiles << "  "
         << Level3DPlayerStateToString(state.player) << "  "
         << Level3DCameraStateToString(state.camera);
  return stream.str();
}

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

  BeginMode3D(camera);
  DrawTiles(level, state);
  DrawLevelBounds(level, state.tile_world_size);
  DrawPlayer(level, state);
  EndMode3D();
}

}  // namespace sar::render3d
