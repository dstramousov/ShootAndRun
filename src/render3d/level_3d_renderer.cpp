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
  const int center_x = TileIndexFromPosition(state.player.tile_x);
  const int center_y = TileIndexFromPosition(state.player.tile_y);
  const int radius = std::max(8, state.visible_radius_tiles);
  return TileRange3D{std::clamp(center_x - radius, 0, level.size.width - 1),
                     std::clamp(center_x + radius, 0, level.size.width - 1),
                     std::clamp(center_y - radius, 0, level.size.height - 1),
                     std::clamp(center_y + radius, 0, level.size.height - 1)};
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

Color TileColor(const RuntimeCell& cell, Level3DRenderMode mode) {
  switch (mode) {
    case Level3DRenderMode::kTerrain:
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

void DrawBlockingVolume(const LevelData& level, int x, int y,
                        const RuntimeCell& cell,
                        const Level3DViewState& state) {
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
      DrawBlockingVolume(level, x, y, *cell, state);
    }
  }
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
  UpdateLevel3DCamera(input, dt, &state->camera);
}

std::string Level3DViewStateToString(const Level3DViewState& state) {
  std::ostringstream stream;
  stream << "renderer3d: mode=" << Level3DRenderModeName(state.mode) << "  "
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
