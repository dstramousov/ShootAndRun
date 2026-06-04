#include "render/level_renderer.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "level/terrain_type.h"

namespace sar {
namespace {

Color TerrainColor(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kOpenGround:
      return Color{78, 104, 58, 255};
    case TerrainType::kForest:
      return Color{22, 64, 40, 255};
    case TerrainType::kRoad:
      return Color{145, 115, 68, 255};
    case TerrainType::kSwamp:
      return Color{42, 83, 68, 255};
    case TerrainType::kRuins:
      return Color{94, 92, 78, 255};
    case TerrainType::kWater:
      return Color{34, 84, 105, 255};
    case TerrainType::kWall:
      return Color{35, 31, 27, 255};
    case TerrainType::kUnknown:
      return Color{138, 62, 128, 255};
  }

  return Color{138, 62, 128, 255};
}

float MapWidthPx(const LevelData& level) {
  return static_cast<float>(level.size.width * level.size.tile_size);
}

float MapHeightPx(const LevelData& level) {
  return static_cast<float>(level.size.height * level.size.tile_size);
}

int ClampTileIndex(int value, int min_value, int max_value) {
  return std::clamp(value, min_value, max_value);
}

}  // namespace

void InitializeLevelView(const LevelData& level, LevelViewState* view) {
  if (view == nullptr) {
    return;
  }

  view->target_x = MapWidthPx(level) * 0.5F;
  view->target_y = MapHeightPx(level) * 0.5F;
  view->zoom = std::clamp(view->zoom, view->min_zoom, view->max_zoom);
}

void ClampLevelViewToMap(const LevelData& level, const WindowState& window,
                         LevelViewState* view) {
  if (view == nullptr) {
    return;
  }

  view->zoom = std::clamp(view->zoom, view->min_zoom, view->max_zoom);

  const float map_width = MapWidthPx(level);
  const float map_height = MapHeightPx(level);
  const float visible_width = static_cast<float>(window.width) / view->zoom;
  const float visible_height = static_cast<float>(window.height) / view->zoom;

  if (map_width <= visible_width) {
    view->target_x = map_width * 0.5F;
  } else {
    const float min_x = visible_width * 0.5F;
    const float max_x = map_width - visible_width * 0.5F;
    view->target_x = std::clamp(view->target_x, min_x, max_x);
  }

  if (map_height <= visible_height) {
    view->target_y = map_height * 0.5F;
  } else {
    const float min_y = visible_height * 0.5F;
    const float max_y = map_height - visible_height * 0.5F;
    view->target_y = std::clamp(view->target_y, min_y, max_y);
  }
}

std::string LevelViewStateToString(const LevelViewState& view) {
  return TextFormat("camera: %.1f,%.1f zoom=%.2f", view.target_x,
                    view.target_y, view.zoom);
}

void LevelRenderer::DrawTerrain(const LevelData& level,
                                const LevelViewState& view,
                                const WindowState& window) const {
  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0 || level.cells.empty()) {
    return;
  }

  Camera2D camera{};
  camera.offset = Vector2{static_cast<float>(window.width) * 0.5F,
                          static_cast<float>(window.height) * 0.5F};
  camera.target = Vector2{view.target_x, view.target_y};
  camera.rotation = 0.0F;
  camera.zoom = view.zoom;

  const Vector2 world_top_left = GetScreenToWorld2D(Vector2{0.0F, 0.0F},
                                                    camera);
  const Vector2 world_bottom_right = GetScreenToWorld2D(
      Vector2{static_cast<float>(window.width),
              static_cast<float>(window.height)},
      camera);

  const float tile_size = static_cast<float>(level.size.tile_size);
  const float min_world_x = std::min(world_top_left.x, world_bottom_right.x);
  const float max_world_x = std::max(world_top_left.x, world_bottom_right.x);
  const float min_world_y = std::min(world_top_left.y, world_bottom_right.y);
  const float max_world_y = std::max(world_top_left.y, world_bottom_right.y);

  const int min_x = ClampTileIndex(
      static_cast<int>(std::floor(min_world_x / tile_size)) - 1, 0,
      level.size.width - 1);
  const int max_x = ClampTileIndex(
      static_cast<int>(std::ceil(max_world_x / tile_size)) + 1, 0,
      level.size.width - 1);
  const int min_y = ClampTileIndex(
      static_cast<int>(std::floor(min_world_y / tile_size)) - 1, 0,
      level.size.height - 1);
  const int max_y = ClampTileIndex(
      static_cast<int>(std::ceil(max_world_y / tile_size)) + 1, 0,
      level.size.height - 1);

  BeginMode2D(camera);

  DrawRectangle(0, 0, static_cast<int>(MapWidthPx(level)),
                static_cast<int>(MapHeightPx(level)),
                Color{12, 16, 14, 255});

  for (int y = min_y; y <= max_y; ++y) {
    for (int x = min_x; x <= max_x; ++x) {
      const int index = y * level.size.width + x;
      if (index < 0 || index >= static_cast<int>(level.cells.size())) {
        continue;
      }

      const RuntimeCell& cell = level.cells[static_cast<std::size_t>(index)];
      DrawRectangle(x * level.size.tile_size, y * level.size.tile_size,
                    level.size.tile_size, level.size.tile_size,
                    TerrainColor(cell.terrain));
    }
  }

  DrawRectangleLinesEx(Rectangle{0.0F, 0.0F, MapWidthPx(level),
                                 MapHeightPx(level)},
                       2.0F / view.zoom, Color{180, 180, 190, 160});

  EndMode2D();
}

}  // namespace sar
