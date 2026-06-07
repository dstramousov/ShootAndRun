#include "render/level_renderer.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "level/terrain_type.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/micro_scene_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/water_visual_plan.h"
#include "visual_pipeline/region_borders.h"
#include "visual_pipeline/visual_map_data.h"

namespace sar {
namespace {

struct VisibleTileRange {
  int min_x = 0;
  int max_x = 0;
  int min_y = 0;
  int max_y = 0;
};

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

Color RoadBandColor(std::uint8_t value) {
  const auto band = static_cast<visual_pipeline::RoadVisualBand>(value);
  switch (band) {
    case visual_pipeline::RoadVisualBand::kRoadCore:
      return Color{154, 112, 62, 255};
    case visual_pipeline::RoadVisualBand::kRoadSide:
      return Color{122, 108, 67, 255};
    case visual_pipeline::RoadVisualBand::kTrampledGrass:
      return Color{91, 112, 61, 255};
    case visual_pipeline::RoadVisualBand::kMudPatch:
      return Color{74, 56, 42, 255};
    case visual_pipeline::RoadVisualBand::kRuinApproach:
      return Color{136, 115, 77, 255};
    case visual_pipeline::RoadVisualBand::kNone:
      return Color{78, 104, 58, 255};
  }
  return Color{78, 104, 58, 255};
}

Color ForestDepthColor(std::uint8_t value) {
  const auto band =
      static_cast<visual_pipeline::ForestDepthBand>(value);
  switch (band) {
    case visual_pipeline::ForestDepthBand::kEdge:
      return Color{38, 88, 48, 255};
    case visual_pipeline::ForestDepthBand::kMid:
      return Color{22, 67, 39, 255};
    case visual_pipeline::ForestDepthBand::kDeep:
      return Color{10, 42, 27, 255};
    case visual_pipeline::ForestDepthBand::kNone:
      return Color{22, 64, 40, 255};
  }
  return Color{22, 64, 40, 255};
}

Color ClearingRoleColor(std::uint8_t value) {
  const auto role = static_cast<visual_pipeline::ClearingRole>(value);
  switch (role) {
    case visual_pipeline::ClearingRole::kMainClearing:
      return Color{114, 132, 70, 255};
    case visual_pipeline::ClearingRole::kSideClearing:
      return Color{78, 112, 58, 255};
    case visual_pipeline::ClearingRole::kConnectorCorridor:
      return Color{123, 110, 64, 255};
    case visual_pipeline::ClearingRole::kMicroClearing:
      return Color{94, 130, 72, 255};
    case visual_pipeline::ClearingRole::kSceneSpace:
      return Color{106, 101, 70, 255};
    case visual_pipeline::ClearingRole::kNone:
      return Color{78, 104, 58, 255};
  }
  return Color{78, 104, 58, 255};
}

Color ClearingSceneRoleColor(std::uint8_t value) {
  const auto role = static_cast<visual_pipeline::ClearingSceneRole>(value);
  switch (role) {
    case visual_pipeline::ClearingSceneRole::kRuinsScene:
      return Color{126, 99, 82, 255};
    case visual_pipeline::ClearingSceneRole::kRoadApproach:
      return Color{139, 112, 63, 255};
    case visual_pipeline::ClearingSceneRole::kObjectScene:
      return Color{105, 92, 128, 255};
    case visual_pipeline::ClearingSceneRole::kGenericScene:
      return Color{90, 108, 122, 255};
    case visual_pipeline::ClearingSceneRole::kNone:
      return Color{106, 101, 70, 255};
  }
  return Color{106, 101, 70, 255};
}

Color RuinVisualTileColor(std::uint8_t value) {
  const auto tile = static_cast<visual_pipeline::RuinVisualTile>(value);
  switch (tile) {
    case visual_pipeline::RuinVisualTile::kCrackedFloor:
      return Color{104, 101, 84, 255};
    case visual_pipeline::RuinVisualTile::kOvergrownFloor:
      return Color{78, 106, 66, 255};
    case visual_pipeline::RuinVisualTile::kWallIntact:
      return Color{55, 49, 42, 255};
    case visual_pipeline::RuinVisualTile::kWallBroken:
      return Color{78, 68, 55, 255};
    case visual_pipeline::RuinVisualTile::kWallCorner:
      return Color{64, 56, 46, 255};
    case visual_pipeline::RuinVisualTile::kWallEndcap:
      return Color{88, 76, 59, 255};
    case visual_pipeline::RuinVisualTile::kRubble:
      return Color{124, 104, 76, 255};
    case visual_pipeline::RuinVisualTile::kEntrance:
      return Color{148, 119, 72, 255};
    case visual_pipeline::RuinVisualTile::kNone:
      return Color{78, 104, 58, 255};
  }
  return Color{78, 104, 58, 255};
}

Color WaterVisualTileColor(std::uint8_t value) {
  const auto tile = static_cast<visual_pipeline::WaterVisualTile>(value);
  switch (tile) {
    case visual_pipeline::WaterVisualTile::kWaterCore:
      return Color{28, 82, 108, 255};
    case visual_pipeline::WaterVisualTile::kWaterEdge:
      return Color{42, 102, 118, 255};
    case visual_pipeline::WaterVisualTile::kMudRing:
      return Color{79, 67, 48, 255};
    case visual_pipeline::WaterVisualTile::kWetGrass:
      return Color{55, 101, 70, 255};
    case visual_pipeline::WaterVisualTile::kReedZone:
      return Color{68, 123, 74, 255};
    case visual_pipeline::WaterVisualTile::kCrossing:
      return Color{112, 98, 67, 255};
    case visual_pipeline::WaterVisualTile::kNone:
      return Color{78, 104, 58, 255};
  }
  return Color{78, 104, 58, 255};
}

Color AddTileVariation(Color base, std::string_view key) {
  const std::size_t hash = std::hash<std::string_view>{}(key);
  const int delta = static_cast<int>(hash % 15U) - 7;
  const auto apply = [delta](unsigned char value) -> unsigned char {
    return static_cast<unsigned char>(std::clamp(static_cast<int>(value) + delta,
                                                0, 255));
  };
  return Color{apply(base.r), apply(base.g), apply(base.b), base.a};
}

bool StartsWith(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

Color VisualTileColor(std::string_view tile_id) {
  Color base{138, 62, 128, 255};
  if (StartsWith(tile_id, "grass")) {
    base = Color{82, 121, 61, 255};
  } else if (StartsWith(tile_id, "forest")) {
    base = Color{24, 70, 42, 255};
  } else if (StartsWith(tile_id, "road")) {
    base = Color{151, 119, 71, 255};
  } else if (StartsWith(tile_id, "swamp")) {
    base = Color{41, 82, 67, 255};
  } else if (StartsWith(tile_id, "water")) {
    base = Color{35, 86, 112, 255};
  } else if (StartsWith(tile_id, "ruins")) {
    base = Color{100, 99, 83, 255};
  } else if (StartsWith(tile_id, "wall")) {
    base = Color{44, 38, 32, 255};
  } else if (StartsWith(tile_id, "boundary")) {
    base = Color{16, 44, 31, 255};
  }

  return AddTileVariation(base, tile_id);
}

Color VisualObjectColor(const visual_pipeline::VisualObjectData& object) {
  if (StartsWith(object.sprite_id, "boundary")) {
    return Color{18, 38, 28, 220};
  }
  if (StartsWith(object.sprite_id, "decor")) {
    return Color{175, 139, 88, 190};
  }
  if (StartsWith(object.sprite_id, "elevation")) {
    return Color{128, 119, 88, 190};
  }
  if (StartsWith(object.sprite_id, "ruin") ||
      StartsWith(object.sprite_id, "wall")) {
    return Color{120, 116, 102, 220};
  }
  if (StartsWith(object.sprite_id, "forest") ||
      StartsWith(object.sprite_id, "tree")) {
    return Color{18, 82, 45, 220};
  }
  if (StartsWith(object.sprite_id, "swamp") ||
      StartsWith(object.sprite_id, "water")) {
    return Color{49, 104, 92, 190};
  }
  if (object.draw_layer == "above_actor") {
    return Color{25, 58, 39, 210};
  }
  return Color{190, 165, 105, 190};
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

bool TextContains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool IsPreferredSpawnMarker(const Marker& marker) {
  return marker.type == "player_spawn" || marker.id == "player_spawn" ||
         TextContains(marker.type, "player_spawn") ||
         TextContains(marker.id, "player_spawn");
}

bool IsFallbackSpawnMarker(const Marker& marker) {
  return marker.type == "start" || marker.id == "start" ||
         TextContains(marker.type, "spawn") || TextContains(marker.id, "spawn");
}

const Marker* FindInitialCameraMarker(const LevelData& level) {
  for (const Marker& marker : level.markers) {
    if (IsPreferredSpawnMarker(marker)) {
      return &marker;
    }
  }

  for (const Marker& marker : level.markers) {
    if (IsFallbackSpawnMarker(marker)) {
      return &marker;
    }
  }

  return nullptr;
}

Color MarkerColor(const Marker& marker) {
  if (IsPreferredSpawnMarker(marker) || IsFallbackSpawnMarker(marker)) {
    return Color{90, 230, 120, 230};
  }

  if (TextContains(marker.type, "goal") || TextContains(marker.type, "exit") ||
      TextContains(marker.id, "goal") || TextContains(marker.id, "exit")) {
    return Color{250, 185, 70, 230};
  }

  return Color{170, 150, 240, 220};
}

Vector2 MarkerWorldCenter(const Marker& marker, int tile_size) {
  const float size = static_cast<float>(tile_size);
  return Vector2{(static_cast<float>(marker.x) + 0.5F) * size,
                 (static_cast<float>(marker.y) + 0.5F) * size};
}

void DrawDebugMarkers(const LevelData& level, float zoom) {
  const float radius = std::max(3.0F, 5.0F / std::max(zoom, 0.1F));
  const float line_length = radius * 2.0F;

  for (const Marker& marker : level.markers) {
    const Vector2 center = MarkerWorldCenter(marker, level.size.tile_size);
    const Color color = MarkerColor(marker);
    DrawCircleV(center, radius, Color{color.r, color.g, color.b, 70});
    DrawCircleLines(static_cast<int>(std::lround(center.x)),
                    static_cast<int>(std::lround(center.y)), radius, color);
    DrawLineEx(Vector2{center.x - line_length, center.y},
               Vector2{center.x + line_length, center.y}, 1.5F / zoom, color);
    DrawLineEx(Vector2{center.x, center.y - line_length},
               Vector2{center.x, center.y + line_length}, 1.5F / zoom, color);
  }
}

Camera2D BuildCamera(const LevelViewState& view, const WindowState& window) {
  Camera2D camera{};
  camera.offset = Vector2{static_cast<float>(window.width) * 0.5F,
                          static_cast<float>(window.height) * 0.5F};
  camera.target = Vector2{view.target_x, view.target_y};
  camera.rotation = 0.0F;
  camera.zoom = view.zoom;
  return camera;
}

VisibleTileRange CalculateVisibleTileRange(const Camera2D& camera,
                                           const WindowState& window,
                                           int width, int height,
                                           int tile_size) {
  const Vector2 world_top_left = GetScreenToWorld2D(Vector2{0.0F, 0.0F},
                                                    camera);
  const Vector2 world_bottom_right = GetScreenToWorld2D(
      Vector2{static_cast<float>(window.width),
              static_cast<float>(window.height)},
      camera);

  const float tile_size_float = static_cast<float>(tile_size);
  const float min_world_x = std::min(world_top_left.x, world_bottom_right.x);
  const float max_world_x = std::max(world_top_left.x, world_bottom_right.x);
  const float min_world_y = std::min(world_top_left.y, world_bottom_right.y);
  const float max_world_y = std::max(world_top_left.y, world_bottom_right.y);

  VisibleTileRange range;
  range.min_x = ClampTileIndex(
      static_cast<int>(std::floor(min_world_x / tile_size_float)) - 1, 0,
      width - 1);
  range.max_x = ClampTileIndex(
      static_cast<int>(std::ceil(max_world_x / tile_size_float)) + 1, 0,
      width - 1);
  range.min_y = ClampTileIndex(
      static_cast<int>(std::floor(min_world_y / tile_size_float)) - 1, 0,
      height - 1);
  range.max_y = ClampTileIndex(
      static_cast<int>(std::ceil(max_world_y / tile_size_float)) + 1, 0,
      height - 1);
  return range;
}


std::uint32_t VisualHashTile(int x, int y, int salt) {
  std::uint32_t value = static_cast<std::uint32_t>(x) * 0x9E3779B1U;
  value ^= static_cast<std::uint32_t>(y) * 0x85EBCA77U;
  value ^= static_cast<std::uint32_t>(salt) * 0xC2B2AE3DU;
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  return value;
}

bool IsForestAssetBand(std::uint8_t value) {
  const auto band = static_cast<visual_pipeline::ForestDepthBand>(value);
  return band == visual_pipeline::ForestDepthBand::kEdge ||
         band == visual_pipeline::ForestDepthBand::kMid ||
         band == visual_pipeline::ForestDepthBand::kDeep;
}

visual_pipeline::ForestDepthBand ForestBandAt(
    const visual_pipeline::ForestVisualPlan& plan, int x, int y) {
  if (!plan.IsValid() || x < 0 || y < 0 || x >= plan.size.width ||
      y >= plan.size.height) {
    return visual_pipeline::ForestDepthBand::kNone;
  }
  const std::size_t index = static_cast<std::size_t>(y * plan.size.width + x);
  if (index >= plan.forest_depth.size()) {
    return visual_pipeline::ForestDepthBand::kNone;
  }
  return static_cast<visual_pipeline::ForestDepthBand>(plan.forest_depth[index]);
}

bool HasForestAt(const visual_pipeline::ForestVisualPlan& plan, int x, int y) {
  if (!plan.IsValid() || x < 0 || y < 0 || x >= plan.size.width ||
      y >= plan.size.height) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y * plan.size.width + x);
  return index < plan.forest_depth.size() &&
         IsForestAssetBand(plan.forest_depth[index]);
}

TerrainType TerrainAt(const LevelData& level, int x, int y) {
  if (x < 0 || y < 0 || x >= level.size.width || y >= level.size.height) {
    return TerrainType::kUnknown;
  }
  const int index = y * level.size.width + x;
  if (index < 0 || index >= static_cast<int>(level.cells.size())) {
    return TerrainType::kUnknown;
  }
  return level.cells[static_cast<std::size_t>(index)].terrain;
}

bool HasRoadVisualAt(const visual_pipeline::PreparedLevel& prepared_level,
                     int x, int y) {
  const auto& plan = prepared_level.road_visual_plan;
  if (!plan.IsValid() || x < 0 || y < 0 || x >= plan.size.width ||
      y >= plan.size.height) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y * plan.size.width + x);
  if (index >= plan.road_bands.size()) {
    return false;
  }
  return static_cast<visual_pipeline::RoadVisualBand>(plan.road_bands[index]) !=
         visual_pipeline::RoadVisualBand::kNone;
}

bool HasWaterVisualAt(const visual_pipeline::PreparedLevel& prepared_level,
                      int x, int y) {
  const auto& plan = prepared_level.water_visual_plan;
  if (!plan.IsValid() || x < 0 || y < 0 || x >= plan.size.width ||
      y >= plan.size.height) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y * plan.size.width + x);
  if (index >= plan.tiles.size()) {
    return false;
  }
  return static_cast<visual_pipeline::WaterVisualTile>(plan.tiles[index]) !=
         visual_pipeline::WaterVisualTile::kNone;
}

bool HasRuinVisualAt(const visual_pipeline::PreparedLevel& prepared_level,
                     int x, int y) {
  const auto& plan = prepared_level.ruin_visual_plan;
  if (!plan.IsValid() || x < 0 || y < 0 || x >= plan.size.width ||
      y >= plan.size.height) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y * plan.size.width + x);
  if (index >= plan.tiles.size()) {
    return false;
  }
  return static_cast<visual_pipeline::RuinVisualTile>(plan.tiles[index]) !=
         visual_pipeline::RuinVisualTile::kNone;
}

bool HasReservedVisualAt(const visual_pipeline::PreparedLevel& prepared_level,
                         int x, int y) {
  return HasRoadVisualAt(prepared_level, x, y) ||
         HasWaterVisualAt(prepared_level, x, y) ||
         HasRuinVisualAt(prepared_level, x, y);
}

int CountForestNeighbors(const visual_pipeline::ForestVisualPlan& plan,
                         int x, int y) {
  int count = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      if (HasForestAt(plan, x + dx, y + dy)) {
        ++count;
      }
    }
  }
  return count;
}

int CountForestInRadius(const visual_pipeline::ForestVisualPlan& plan,
                        int center_x, int center_y, int radius) {
  int count = 0;
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy > radius * radius) {
        continue;
      }
      if (HasForestAt(plan, center_x + dx, center_y + dy)) {
        ++count;
      }
    }
  }
  return count;
}

bool IsForestInterior(const visual_pipeline::ForestVisualPlan& plan, int x,
                      int y) {
  return CountForestNeighbors(plan, x, y) >= 7 &&
         CountForestInRadius(plan, x, y, 2) >= 12;
}

int HashSignedOffset(int x, int y, int salt, int max_abs) {
  const int span = max_abs * 2 + 1;
  return static_cast<int>(VisualHashTile(x, y, salt) %
                          static_cast<std::uint32_t>(span)) -
         max_abs;
}

Vector2 ForestSpriteJitter(int x, int y, int salt, int max_abs_px) {
  return Vector2{static_cast<float>(HashSignedOffset(x, y, salt, max_abs_px)),
                 static_cast<float>(HashSignedOffset(x, y, salt + 11,
                                                     max_abs_px))};
}

void DrawAnchoredForestSprite(const ForestAssetTexture& item, int tile_x,
                              int tile_y, int tile_size, Vector2 offset,
                              float scale, Color tint) {
  const float source_width = static_cast<float>(item.texture.width);
  const float source_height = static_cast<float>(item.texture.height);
  const float width = source_width * scale;
  const float height = source_height * scale;
  const float anchor_x = (static_cast<float>(tile_x) + 0.5F) *
                         static_cast<float>(tile_size) +
                         offset.x;
  const float anchor_y = static_cast<float>((tile_y + 1) * tile_size) +
                         offset.y;
  const Rectangle source{0.0F, 0.0F, source_width, source_height};
  const Rectangle destination{anchor_x - width * 0.5F, anchor_y - height,
                              width, height};
  DrawTexturePro(item.texture, source, destination, Vector2{0.0F, 0.0F},
                 0.0F, tint);
}

void DrawCenteredForestSprite(const ForestAssetTexture& item, int tile_x,
                              int tile_y, int tile_size, Vector2 offset,
                              float scale, Color tint) {
  const float source_width = static_cast<float>(item.texture.width);
  const float source_height = static_cast<float>(item.texture.height);
  const float width = source_width * scale;
  const float height = source_height * scale;
  const float center_x = (static_cast<float>(tile_x) + 0.5F) *
                         static_cast<float>(tile_size) +
                         offset.x;
  const float center_y = (static_cast<float>(tile_y) + 0.5F) *
                         static_cast<float>(tile_size) +
                         offset.y;
  const Rectangle source{0.0F, 0.0F, source_width, source_height};
  const Rectangle destination{center_x - width * 0.5F,
                              center_y - height * 0.5F, width, height};
  DrawTexturePro(item.texture, source, destination, Vector2{0.0F, 0.0F},
                 0.0F, tint);
}

void DrawForestSpriteShadow(const ForestAssetCatalog& assets, int tile_x,
                            int tile_y, int tile_size, Vector2 offset,
                            float scale) {
  const ForestAssetTexture* shadow = assets.PickShadow(tile_x, tile_y);
  if (shadow == nullptr) {
    return;
  }
  const float width = static_cast<float>(shadow->texture.width) * scale;
  const float height = static_cast<float>(shadow->texture.height) * scale;
  const float anchor_x = (static_cast<float>(tile_x) + 0.5F) *
                         static_cast<float>(tile_size) +
                         offset.x;
  const float anchor_y = static_cast<float>((tile_y + 1) * tile_size) +
                         offset.y;
  const Rectangle source{0.0F, 0.0F,
                         static_cast<float>(shadow->texture.width),
                         static_cast<float>(shadow->texture.height)};
  const Rectangle destination{anchor_x - width * 0.5F,
                              anchor_y - height * 0.55F, width, height};
  DrawTexturePro(shadow->texture, source, destination, Vector2{0.0F, 0.0F},
                 0.0F, Color{255, 255, 255, 105});
}

bool IsCanopyPlacement(const visual_pipeline::ForestVisualPlan& plan,
                       visual_pipeline::ForestDepthBand band, int x, int y,
                       std::size_t index) {
  if (index >= plan.canopy_candidates.size() ||
      plan.canopy_candidates[index] == 0U) {
    return false;
  }
  if (!IsForestInterior(plan, x, y)) {
    return false;
  }

  if (band == visual_pipeline::ForestDepthBand::kDeep) {
    return VisualHashTile(x, y, 149) % 7U == 0U;
  }
  if (band == visual_pipeline::ForestDepthBand::kMid) {
    return VisualHashTile(x, y, 151) % 17U == 0U;
  }
  return false;
}

bool IsTreePlacement(const visual_pipeline::ForestVisualPlan& plan,
                     visual_pipeline::ForestDepthBand band, int x, int y) {
  const int neighbor_count = CountForestNeighbors(plan, x, y);
  std::uint32_t modulo = 9U;
  int salt = 131;
  int min_neighbors = 3;

  if (band == visual_pipeline::ForestDepthBand::kMid) {
    modulo = 5U;
    salt = 137;
    min_neighbors = 5;
  } else if (band == visual_pipeline::ForestDepthBand::kDeep) {
    modulo = 6U;
    salt = 139;
    min_neighbors = 6;
  }

  if (neighbor_count < min_neighbors) {
    return false;
  }
  return VisualHashTile(x, y, salt) % modulo == 0U;
}

ForestSpriteBand SpriteBandForDepth(
    visual_pipeline::ForestDepthBand band) {
  if (band == visual_pipeline::ForestDepthBand::kMid) {
    return ForestSpriteBand::kMid;
  }
  if (band == visual_pipeline::ForestDepthBand::kDeep) {
    return ForestSpriteBand::kDeep;
  }
  return ForestSpriteBand::kEdge;
}

Color ForestSpriteTint(visual_pipeline::ForestDepthBand band) {
  if (band == visual_pipeline::ForestDepthBand::kEdge) {
    return Color{255, 255, 255, 235};
  }
  if (band == visual_pipeline::ForestDepthBand::kMid) {
    return Color{242, 248, 226, 240};
  }
  return Color{220, 232, 204, 245};
}

float ForestSpriteScale(visual_pipeline::ForestDepthBand band) {
  if (band == visual_pipeline::ForestDepthBand::kEdge) {
    return 0.78F;
  }
  if (band == visual_pipeline::ForestDepthBand::kMid) {
    return 0.94F;
  }
  return 1.06F;
}

void DrawForestMassCompositionUnderlay(
    const visual_pipeline::ForestVisualPlan& plan, int tile_size,
    const VisibleTileRange& range) {
  if (!plan.IsValid()) {
    return;
  }
  const float tile = static_cast<float>(tile_size);
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const auto band = ForestBandAt(plan, x, y);
      if (band == visual_pipeline::ForestDepthBand::kNone) {
        continue;
      }

      const Vector2 center{(static_cast<float>(x) + 0.5F) * tile,
                           (static_cast<float>(y) + 0.5F) * tile};
      if (band == visual_pipeline::ForestDepthBand::kEdge) {
        DrawCircleV(center, tile * 0.58F, Color{39, 91, 48, 80});
      } else if (band == visual_pipeline::ForestDepthBand::kMid) {
        DrawCircleV(center, tile * 0.76F, Color{18, 65, 38, 72});
      } else {
        DrawCircleV(center, tile * 0.92F, Color{6, 34, 22, 88});
        if (VisualHashTile(x, y, 173) % 5U == 0U) {
          const Vector2 jitter = ForestSpriteJitter(x, y, 179, 5);
          DrawCircleV(Vector2{center.x + jitter.x, center.y + jitter.y},
                      tile * 1.18F, Color{4, 25, 17, 52});
        }
      }
    }
  }
}

int OuterForestFringeBand(const LevelData& level,
                          const visual_pipeline::PreparedLevel& prepared_level,
                          const visual_pipeline::ForestVisualPlan& plan,
                          int x, int y) {
  if (HasForestAt(plan, x, y) || TerrainAt(level, x, y) !=
                                      TerrainType::kOpenGround ||
      HasReservedVisualAt(prepared_level, x, y)) {
    return 0;
  }
  if (CountForestInRadius(plan, x, y, 1) > 0) {
    return 1;
  }
  if (CountForestInRadius(plan, x, y, 2) > 0) {
    return 2;
  }
  if (CountForestInRadius(plan, x, y, 3) > 0) {
    return 3;
  }
  return 0;
}

bool IsInnerForestFringeTile(const visual_pipeline::ForestVisualPlan& plan,
                             int x, int y) {
  if (ForestBandAt(plan, x, y) != visual_pipeline::ForestDepthBand::kEdge) {
    return false;
  }
  return CountForestNeighbors(plan, x, y) <= 6;
}

void DrawForestFringeGround(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const ForestAssetCatalog& assets, int tile_size,
    const VisibleTileRange& range) {
  if (!prepared_level.forest_visual_plan.IsValid()) {
    return;
  }
  const auto& plan = prepared_level.forest_visual_plan;
  const float tile = static_cast<float>(tile_size);
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int outer_band = OuterForestFringeBand(level, prepared_level, plan,
                                                  x, y);
      const bool inner_edge = IsInnerForestFringeTile(plan, x, y);
      if (outer_band == 0 && !inner_edge) {
        continue;
      }

      const Vector2 center{(static_cast<float>(x) + 0.5F) * tile,
                           (static_cast<float>(y) + 0.5F) * tile};
      if (outer_band > 0) {
        const unsigned char alpha = outer_band == 1 ? 54U :
                                    outer_band == 2 ? 34U : 20U;
        DrawCircleV(center, tile * (0.84F - 0.10F * static_cast<float>(outer_band)),
                    Color{23, 67, 38, alpha});
        if (VisualHashTile(x, y, 307) %
                static_cast<std::uint32_t>(outer_band + 1) ==
            0U) {
          const ForestAssetTexture* ground = assets.PickFringe(
              ForestFringeAssetKind::kGround, x, y);
          if (ground != nullptr) {
            const Vector2 jitter = ForestSpriteJitter(x, y, 311, 4);
            DrawCenteredForestSprite(*ground, x, y, tile_size, jitter, 1.04F,
                                     Color{232, 242, 206, 168});
          }
        }
      }

      if (inner_edge && VisualHashTile(x, y, 317) % 3U == 0U) {
        const ForestAssetTexture* shadow = assets.PickFringe(
            ForestFringeAssetKind::kShadow, x, y);
        if (shadow != nullptr) {
          const Vector2 jitter = ForestSpriteJitter(x, y, 319, 4);
          DrawCenteredForestSprite(*shadow, x, y, tile_size, jitter, 1.18F,
                                   Color{255, 255, 255, 118});
        }
      }
    }
  }
}

const ForestAssetTexture* PickOuterFringeSprite(const ForestAssetCatalog& assets,
                                                int x, int y) {
  const std::uint32_t hash = VisualHashTile(x, y, 331) % 100U;
  if (hash < 42U) {
    return assets.PickFringe(ForestFringeAssetKind::kBush, x, y);
  }
  if (hash < 76U) {
    return assets.PickFringe(ForestFringeAssetKind::kFern, x, y);
  }
  if (hash < 92U) {
    return assets.PickFringe(ForestFringeAssetKind::kWood, x, y);
  }
  return assets.PickFringe(ForestFringeAssetKind::kRock, x, y);
}

const ForestAssetTexture* PickInnerFringeSprite(const ForestAssetCatalog& assets,
                                                int x, int y) {
  const std::uint32_t hash = VisualHashTile(x, y, 337) % 100U;
  if (hash < 46U) {
    return assets.PickFringe(ForestFringeAssetKind::kSapling, x, y);
  }
  if (hash < 78U) {
    return assets.PickFringe(ForestFringeAssetKind::kBush, x, y);
  }
  if (hash < 90U) {
    return assets.PickFringe(ForestFringeAssetKind::kFern, x, y);
  }
  return assets.PickFringe(ForestFringeAssetKind::kWood, x, y);
}

void DrawForestFringeSprites(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const ForestAssetCatalog& assets, int tile_size,
    const VisibleTileRange& range) {
  if (!prepared_level.forest_visual_plan.IsValid()) {
    return;
  }
  const auto& plan = prepared_level.forest_visual_plan;
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int outer_band = OuterForestFringeBand(level, prepared_level, plan,
                                                  x, y);
      const bool inner_edge = IsInnerForestFringeTile(plan, x, y);
      const ForestAssetTexture* sprite = nullptr;
      float scale = 0.82F;
      Color tint{238, 246, 216, 222};

      if (outer_band == 1 && VisualHashTile(x, y, 347) % 4U == 0U) {
        sprite = PickOuterFringeSprite(assets, x, y);
        scale = 0.74F;
      } else if (outer_band == 2 && VisualHashTile(x, y, 349) % 9U == 0U) {
        sprite = PickOuterFringeSprite(assets, x, y);
        scale = 0.64F;
        tint = Color{232, 242, 206, 188};
      } else if (inner_edge && VisualHashTile(x, y, 353) % 5U == 0U) {
        sprite = PickInnerFringeSprite(assets, x, y);
        scale = 0.82F;
        tint = Color{226, 238, 204, 216};
      }

      if (sprite == nullptr) {
        continue;
      }
      const Vector2 jitter = ForestSpriteJitter(x, y, 359, 6);
      DrawAnchoredForestSprite(*sprite, x, y, tile_size, jitter, scale, tint);
    }
  }
}

void DrawForestAssetSprites(const visual_pipeline::ForestVisualPlan& plan,
                            const ForestAssetCatalog& assets, int tile_size,
                            const VisibleTileRange& range) {
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const std::size_t index =
          static_cast<std::size_t>(y * plan.size.width + x);
      if (index >= plan.forest_depth.size() ||
          !IsForestAssetBand(plan.forest_depth[index])) {
        continue;
      }

      const auto band = static_cast<visual_pipeline::ForestDepthBand>(
          plan.forest_depth[index]);
      if (IsCanopyPlacement(plan, band, x, y, index)) {
        const ForestAssetTexture* canopy = assets.PickSprite(
            ForestSpriteBand::kCanopy, x, y);
        if (canopy != nullptr) {
          const Vector2 jitter = ForestSpriteJitter(x, y, 211, 7);
          DrawForestSpriteShadow(assets, x, y, tile_size, jitter, 1.12F);
          DrawAnchoredForestSprite(*canopy, x, y, tile_size, jitter, 1.06F,
                                   Color{232, 242, 210, 232});
        }
        continue;
      }

      if (!IsTreePlacement(plan, band, x, y)) {
        continue;
      }
      const ForestAssetTexture* sprite = assets.PickSprite(
          SpriteBandForDepth(band), x, y);
      if (sprite == nullptr) {
        continue;
      }

      const int jitter_px = band == visual_pipeline::ForestDepthBand::kEdge
                                ? 5
                                : 7;
      const float scale = ForestSpriteScale(band);
      const Vector2 jitter = ForestSpriteJitter(x, y, 223, jitter_px);
      DrawForestSpriteShadow(assets, x, y, tile_size, jitter, scale);
      DrawAnchoredForestSprite(*sprite, x, y, tile_size, jitter, scale,
                               ForestSpriteTint(band));
    }
  }
}

void DrawForestAssetsForVisualPreview(
    const LevelData& level, const visual_pipeline::PreparedLevel& prepared_level,
    const ForestAssetCatalog& assets, int tile_size,
    const VisibleTileRange& range) {
  if (!prepared_level.forest_visual_plan.IsValid() || !assets.loaded()) {
    return;
  }
  DrawForestMassCompositionUnderlay(prepared_level.forest_visual_plan,
                                     tile_size, range);
  DrawForestFringeGround(level, prepared_level, assets, tile_size, range);
  DrawForestAssetSprites(prepared_level.forest_visual_plan, assets, tile_size,
                         range);
  DrawForestFringeSprites(level, prepared_level, assets, tile_size, range);
}

bool IsTileVisible(int x, int y, const VisibleTileRange& range) {
  return x >= range.min_x && x <= range.max_x && y >= range.min_y &&
         y <= range.max_y;
}

void DrawRawTerrainTiles(const LevelData& level,
                         const VisibleTileRange& range) {
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
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
}

Color CppAnalysisTileColor(
    const RuntimeCell& cell,
    const visual_pipeline::ForestVisualPlan* forest_visual_plan,
    const visual_pipeline::RoadVisualPlan* road_visual_plan,
    const visual_pipeline::RuinVisualPlan* ruin_visual_plan,
    const visual_pipeline::WaterVisualPlan* water_visual_plan,
    std::size_t index) {
  if (ruin_visual_plan != nullptr && ruin_visual_plan->IsValid() &&
      index < ruin_visual_plan->tiles.size() &&
      ruin_visual_plan->tiles[index] != 0) {
    return RuinVisualTileColor(ruin_visual_plan->tiles[index]);
  }

  if (water_visual_plan != nullptr && water_visual_plan->IsValid() &&
      index < water_visual_plan->tiles.size() &&
      water_visual_plan->tiles[index] != 0) {
    return WaterVisualTileColor(water_visual_plan->tiles[index]);
  }

  if (road_visual_plan != nullptr && road_visual_plan->IsValid() &&
      index < road_visual_plan->road_bands.size() &&
      road_visual_plan->road_bands[index] != 0) {
    return RoadBandColor(road_visual_plan->road_bands[index]);
  }

  if (forest_visual_plan != nullptr && forest_visual_plan->IsValid()) {
    if (index < forest_visual_plan->forest_depth.size() &&
        forest_visual_plan->forest_depth[index] != 0) {
      return ForestDepthColor(forest_visual_plan->forest_depth[index]);
    }
    if (index < forest_visual_plan->clearing_scene_roles.size() &&
        forest_visual_plan->clearing_scene_roles[index] != 0) {
      return ClearingSceneRoleColor(
          forest_visual_plan->clearing_scene_roles[index]);
    }
    if (index < forest_visual_plan->clearing_roles.size() &&
        forest_visual_plan->clearing_roles[index] != 0) {
      return ClearingRoleColor(forest_visual_plan->clearing_roles[index]);
    }
    if (index < forest_visual_plan->route_influence.size() &&
        forest_visual_plan->route_influence[index] >= 3 &&
        cell.terrain == TerrainType::kRoad) {
      return Color{154, 120, 66, 255};
    }
  }
  return TerrainColor(cell.terrain);
}

Color ForestClearingAnalysisTileColor(
    const RuntimeCell& cell,
    const visual_pipeline::ForestVisualPlan* forest_visual_plan,
    std::size_t index) {
  if (forest_visual_plan == nullptr || !forest_visual_plan->IsValid()) {
    return TerrainColor(cell.terrain);
  }

  if (index < forest_visual_plan->forest_depth.size() &&
      forest_visual_plan->forest_depth[index] != 0) {
    return ForestDepthColor(forest_visual_plan->forest_depth[index]);
  }
  if (index < forest_visual_plan->clearing_scene_roles.size() &&
      forest_visual_plan->clearing_scene_roles[index] != 0) {
    return ClearingSceneRoleColor(
        forest_visual_plan->clearing_scene_roles[index]);
  }
  if (index < forest_visual_plan->clearing_roles.size() &&
      forest_visual_plan->clearing_roles[index] != 0) {
    return ClearingRoleColor(forest_visual_plan->clearing_roles[index]);
  }
  return TerrainColor(cell.terrain);
}

Color VisualIntentPreviewTileColor(
    const RuntimeCell& cell,
    const visual_pipeline::ForestVisualPlan* forest_visual_plan,
    const visual_pipeline::RoadVisualPlan* road_visual_plan,
    const visual_pipeline::RuinVisualPlan* ruin_visual_plan,
    const visual_pipeline::WaterVisualPlan* water_visual_plan,
    std::size_t index) {
  if (ruin_visual_plan != nullptr && ruin_visual_plan->IsValid() &&
      index < ruin_visual_plan->tiles.size() &&
      ruin_visual_plan->tiles[index] != 0) {
    return RuinVisualTileColor(ruin_visual_plan->tiles[index]);
  }

  if (water_visual_plan != nullptr && water_visual_plan->IsValid() &&
      index < water_visual_plan->tiles.size() &&
      water_visual_plan->tiles[index] != 0) {
    return WaterVisualTileColor(water_visual_plan->tiles[index]);
  }

  if (road_visual_plan != nullptr && road_visual_plan->IsValid() &&
      index < road_visual_plan->road_bands.size() &&
      road_visual_plan->road_bands[index] != 0) {
    return RoadBandColor(road_visual_plan->road_bands[index]);
  }

  if (forest_visual_plan != nullptr && forest_visual_plan->IsValid() &&
      index < forest_visual_plan->forest_depth.size() &&
      forest_visual_plan->forest_depth[index] != 0) {
    return ForestDepthColor(forest_visual_plan->forest_depth[index]);
  }

  return TerrainColor(cell.terrain);
}

void DrawCppAnalysisTiles(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const VisibleTileRange& range) {
  const visual_pipeline::ForestVisualPlan* forest_visual_plan =
      prepared_level.forest_visual_plan.IsValid()
          ? &prepared_level.forest_visual_plan
          : nullptr;
  const visual_pipeline::RoadVisualPlan* road_visual_plan =
      prepared_level.road_visual_plan.IsValid()
          ? &prepared_level.road_visual_plan
          : nullptr;
  const visual_pipeline::RuinVisualPlan* ruin_visual_plan =
      prepared_level.ruin_visual_plan.IsValid()
          ? &prepared_level.ruin_visual_plan
          : nullptr;
  const visual_pipeline::WaterVisualPlan* water_visual_plan =
      prepared_level.water_visual_plan.IsValid()
          ? &prepared_level.water_visual_plan
          : nullptr;
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int index = y * level.size.width + x;
      if (index < 0 || index >= static_cast<int>(level.cells.size())) {
        continue;
      }

      const auto item = static_cast<std::size_t>(index);
      const RuntimeCell& cell = level.cells[item];
      DrawRectangle(x * level.size.tile_size, y * level.size.tile_size,
                    level.size.tile_size, level.size.tile_size,
                    CppAnalysisTileColor(cell, forest_visual_plan,
                                         road_visual_plan, ruin_visual_plan,
                                         water_visual_plan, item));
    }
  }
}

void DrawForestClearingAnalysisTiles(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const VisibleTileRange& range) {
  const visual_pipeline::ForestVisualPlan* forest_visual_plan =
      prepared_level.forest_visual_plan.IsValid()
          ? &prepared_level.forest_visual_plan
          : nullptr;
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int index = y * level.size.width + x;
      if (index < 0 || index >= static_cast<int>(level.cells.size())) {
        continue;
      }

      const auto item = static_cast<std::size_t>(index);
      const RuntimeCell& cell = level.cells[item];
      DrawRectangle(x * level.size.tile_size, y * level.size.tile_size,
                    level.size.tile_size, level.size.tile_size,
                    ForestClearingAnalysisTileColor(cell, forest_visual_plan,
                                                    item));
    }
  }
}

void DrawVisualIntentPreviewTiles(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const VisibleTileRange& range) {
  const visual_pipeline::ForestVisualPlan* forest_visual_plan =
      prepared_level.forest_visual_plan.IsValid()
          ? &prepared_level.forest_visual_plan
          : nullptr;
  const visual_pipeline::RoadVisualPlan* road_visual_plan =
      prepared_level.road_visual_plan.IsValid()
          ? &prepared_level.road_visual_plan
          : nullptr;
  const visual_pipeline::RuinVisualPlan* ruin_visual_plan =
      prepared_level.ruin_visual_plan.IsValid()
          ? &prepared_level.ruin_visual_plan
          : nullptr;
  const visual_pipeline::WaterVisualPlan* water_visual_plan =
      prepared_level.water_visual_plan.IsValid()
          ? &prepared_level.water_visual_plan
          : nullptr;
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int index = y * level.size.width + x;
      if (index < 0 || index >= static_cast<int>(level.cells.size())) {
        continue;
      }

      const auto item = static_cast<std::size_t>(index);
      const RuntimeCell& cell = level.cells[item];
      DrawRectangle(x * level.size.tile_size, y * level.size.tile_size,
                    level.size.tile_size, level.size.tile_size,
                    VisualIntentPreviewTileColor(cell, forest_visual_plan,
                                                 road_visual_plan,
                                                 ruin_visual_plan,
                                                 water_visual_plan, item));
    }
  }
}

void DrawAnalysisOverlay(const visual_pipeline::PreparedLevel& prepared_level,
                         int tile_size, float zoom,
                         const VisibleTileRange& range) {
  if (!prepared_level.region_borders.IsValid()) {
    return;
  }

  const float line_width = std::max(1.0F / std::max(zoom, 0.1F), 0.5F);
  for (const visual_pipeline::RegionBorderInfo& region :
       prepared_level.region_borders.regions) {
    for (const visual_pipeline::BorderTile& tile : region.tiles) {
      if (!IsTileVisible(tile.x, tile.y, range)) {
        continue;
      }

      Color color = Color{245, 190, 70, 120};
      if (tile.differing_neighbor_count >= 3) {
        color = Color{235, 85, 95, 150};
      } else if (tile.IsCorner()) {
        color = Color{95, 170, 245, 135};
      } else if (tile.touches_map_edge) {
        color = Color{220, 220, 235, 120};
      }

      DrawRectangleLinesEx(
          Rectangle{static_cast<float>(tile.x * tile_size),
                    static_cast<float>(tile.y * tile_size),
                    static_cast<float>(tile_size),
                    static_cast<float>(tile_size)},
          line_width, color);
    }
  }
}

const visual_pipeline::VisualLayerGrid* FindRenderableVisualLayer(
    const visual_pipeline::VisualMapData& visual_map) {
  for (const visual_pipeline::VisualLayerGrid& layer : visual_map.layers) {
    if (layer.IsRenderable()) {
      return &layer;
    }
  }
  return nullptr;
}

void DrawPreparedVisualTiles(const visual_pipeline::VisualLayerGrid& layer,
                             int tile_size,
                             const VisibleTileRange& range) {
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int index = y * layer.width + x;
      if (index < 0 || index >= static_cast<int>(layer.tile_ids.size())) {
        continue;
      }
      const std::string& tile_id = layer.tile_ids[static_cast<std::size_t>(index)];
      DrawRectangle(x * tile_size, y * tile_size, tile_size, tile_size,
                    VisualTileColor(tile_id));
    }
  }
}


void DrawFinalRenderReference(const Texture2D& texture,
                              const LevelData& level) {
  const Rectangle source{0.0F, 0.0F, static_cast<float>(texture.width),
                         static_cast<float>(texture.height)};
  const Rectangle destination{0.0F, 0.0F, MapWidthPx(level),
                              MapHeightPx(level)};
  DrawTexturePro(texture, source, destination, Vector2{0.0F, 0.0F}, 0.0F,
                 WHITE);
}

Color MicroSceneTileColor(std::uint8_t value) {
  const auto tile = static_cast<visual_pipeline::MicroSceneTile>(value);
  switch (tile) {
    case visual_pipeline::MicroSceneTile::kGroundDetail:
      return Color{126, 111, 66, 115};
    case visual_pipeline::MicroSceneTile::kSmallDebris:
      return Color{142, 112, 74, 130};
    case visual_pipeline::MicroSceneTile::kSecondaryProp:
      return Color{154, 108, 64, 140};
    case visual_pipeline::MicroSceneTile::kPrimaryProp:
      return Color{190, 132, 70, 155};
    case visual_pipeline::MicroSceneTile::kVegetationDetail:
      return Color{58, 118, 56, 120};
    case visual_pipeline::MicroSceneTile::kStoneDetail:
      return Color{142, 132, 112, 135};
    case visual_pipeline::MicroSceneTile::kWetDetail:
      return Color{54, 112, 82, 125};
    case visual_pipeline::MicroSceneTile::kNone:
      return Color{0, 0, 0, 0};
  }
  return Color{0, 0, 0, 0};
}

void DrawMicroSceneVisualPlanForPreview(
    const visual_pipeline::MicroSceneVisualPlan& plan,
    int tile_size,
    const VisibleTileRange& range) {
  if (!plan.IsValid()) {
    return;
  }
  for (int y = range.min_y; y <= range.max_y; ++y) {
    for (int x = range.min_x; x <= range.max_x; ++x) {
      const int index = y * plan.size.width + x;
      if (index < 0 || index >= static_cast<int>(plan.tiles.size())) {
        continue;
      }
      const Color color = MicroSceneTileColor(
          plan.tiles[static_cast<std::size_t>(index)]);
      if (color.a == 0) {
        continue;
      }
      DrawRectangle(x * tile_size, y * tile_size, tile_size, tile_size, color);
    }
  }
}

Color ObjectVisualItemColor(const visual_pipeline::ObjectVisualItem& item) {
  switch (item.kind) {
    case visual_pipeline::ObjectVisualKind::kVegetation:
      return Color{34, 102, 54, 150};
    case visual_pipeline::ObjectVisualKind::kWood:
      return Color{128, 86, 46, 150};
    case visual_pipeline::ObjectVisualKind::kStone:
      return Color{132, 126, 112, 155};
    case visual_pipeline::ObjectVisualKind::kScrap:
      return Color{126, 102, 88, 150};
    case visual_pipeline::ObjectVisualKind::kCamp:
      return Color{174, 124, 64, 160};
    case visual_pipeline::ObjectVisualKind::kCache:
      return Color{218, 172, 72, 170};
    case visual_pipeline::ObjectVisualKind::kStructure:
      return Color{98, 88, 72, 170};
    case visual_pipeline::ObjectVisualKind::kRuin:
      return Color{116, 106, 88, 150};
    case visual_pipeline::ObjectVisualKind::kElevation:
      return Color{104, 86, 64, 120};
    case visual_pipeline::ObjectVisualKind::kLandmark:
      return Color{184, 138, 82, 165};
    case visual_pipeline::ObjectVisualKind::kCover:
      return Color{86, 74, 56, 145};
    case visual_pipeline::ObjectVisualKind::kTypedFallback:
      return Color{166, 132, 84, 130};
    case visual_pipeline::ObjectVisualKind::kUnknown:
      return Color{206, 46, 180, 170};
  }
  return Color{166, 132, 84, 130};
}

Color RuntimeObjectPreviewColor(const RuntimeObject& object) {
  if (StartsWith(object.type, "tree") || StartsWith(object.family, "forest") ||
      StartsWith(object.family, "vegetation")) {
    return Color{18, 84, 45, 120};
  }
  if (StartsWith(object.type, "ruin") || StartsWith(object.family, "ruin") ||
      StartsWith(object.type, "wall") || StartsWith(object.family, "wall") ||
      StartsWith(object.type, "bunker") || StartsWith(object.family, "military")) {
    return Color{112, 101, 84, 150};
  }
  if (StartsWith(object.type, "water") || StartsWith(object.family, "swamp")) {
    return Color{50, 105, 98, 110};
  }
  if (object.blocks_movement || object.blocks_vision ||
      object.blocks_projectiles) {
    return Color{70, 61, 45, 130};
  }
  return Color{165, 132, 86, 105};
}

void DrawRuntimeObjectsForVisualPreview(const LevelData& level,
                                        const VisibleTileRange& range) {
  for (const RuntimeObject& object : level.objects) {
    if (object.width <= 0 || object.height <= 0) {
      continue;
    }

    const int max_x = object.x + object.width - 1;
    const int max_y = object.y + object.height - 1;
    if (max_x < range.min_x || object.x > range.max_x || max_y < range.min_y ||
        object.y > range.max_y) {
      continue;
    }

    const Rectangle bounds{
        static_cast<float>(object.x * level.size.tile_size),
        static_cast<float>(object.y * level.size.tile_size),
        static_cast<float>(object.width * level.size.tile_size),
        static_cast<float>(object.height * level.size.tile_size)};
    DrawRectangleRec(bounds, RuntimeObjectPreviewColor(object));
  }
}

void DrawObjectVisualPlanForPreview(
    const visual_pipeline::ObjectVisualPlan& plan,
    int tile_size,
    const VisibleTileRange& range) {
  for (const visual_pipeline::ObjectVisualItem& item : plan.items) {
    const int max_x = item.x + item.width - 1;
    const int max_y = item.y + item.height - 1;
    if (max_x < range.min_x || item.x > range.max_x || max_y < range.min_y ||
        item.y > range.max_y) {
      continue;
    }

    const Rectangle bounds{static_cast<float>(item.x * tile_size),
                           static_cast<float>(item.y * tile_size),
                           static_cast<float>(item.width * tile_size),
                           static_cast<float>(item.height * tile_size)};
    DrawRectangleRec(bounds, ObjectVisualItemColor(item));
  }
}

void DrawPreparedVisualObjects(const visual_pipeline::VisualMapData& visual_map,
                               int tile_size,
                               const VisibleTileRange& range) {
  for (const visual_pipeline::VisualObjectData& object : visual_map.objects) {
    const int max_x = object.x + object.width - 1;
    const int max_y = object.y + object.height - 1;
    if (max_x < range.min_x || object.x > range.max_x || max_y < range.min_y ||
        object.y > range.max_y) {
      continue;
    }

    const Color color = VisualObjectColor(object);
    const Rectangle bounds{static_cast<float>(object.x * tile_size),
                           static_cast<float>(object.y * tile_size),
                           static_cast<float>(object.width * tile_size),
                           static_cast<float>(object.height * tile_size)};
    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 1.0F, Color{10, 12, 10, 110});
  }
}

}  // namespace

const char* LevelRenderModeName(LevelRenderMode mode) {
  switch (mode) {
    case LevelRenderMode::kRawTerrain:
      return "raw_terrain";
    case LevelRenderMode::kCppAnalysis:
      return "cpp_analysis";
    case LevelRenderMode::kForestClearingAnalysis:
      return "forest_clearing_analysis";
    case LevelRenderMode::kVisualIntentPreview:
      return "visual_intent_preview";
    case LevelRenderMode::kPreparedVisualMap:
      return "prepared_visual_map";
    case LevelRenderMode::kFinalRenderReference:
      return "final_render_package";
  }
  return "raw_terrain";
}

void InitializeLevelView(const LevelData& level, LevelViewState* view) {
  if (view == nullptr) {
    return;
  }

  const Marker* marker = FindInitialCameraMarker(level);
  if (marker != nullptr) {
    const Vector2 center = MarkerWorldCenter(*marker, level.size.tile_size);
    view->target_x = center.x;
    view->target_y = center.y;
  } else {
    view->target_x = MapWidthPx(level) * 0.5F;
    view->target_y = MapHeightPx(level) * 0.5F;
  }
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


bool LevelRenderer::PreloadForestAssets() {
  return EnsureForestAssetsLoaded();
}

int LevelRenderer::forest_asset_texture_count() const {
  return forest_assets_.loaded_texture_count();
}

void LevelRenderer::ResetForestAssets() {
  forest_assets_.Reset();
  forest_assets_load_attempted_ = false;
}

bool LevelRenderer::EnsureForestAssetsLoaded() const {
  if (forest_assets_.loaded()) {
    return true;
  }
  if (forest_assets_load_attempted_) {
    return false;
  }
  forest_assets_load_attempted_ = true;

  std::string error;
  const bool loaded = forest_assets_.Load(
      std::filesystem::path("assets") / "visual" / "forest", &error);
  return loaded;
}

void LevelRenderer::Draw(const LevelData& level,
                         const visual_pipeline::PreparedLevel* prepared_level,
                         const LevelViewState& view,
                         const WindowState& window,
                         LevelRenderMode mode,
                         const Texture2D* final_render_texture) const {
  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0 || level.cells.empty()) {
    return;
  }

  const Camera2D camera = BuildCamera(view, window);
  const VisibleTileRange range = CalculateVisibleTileRange(
      camera, window, level.size.width, level.size.height,
      level.size.tile_size);

  BeginMode2D(camera);

  DrawRectangle(0, 0, static_cast<int>(MapWidthPx(level)),
                static_cast<int>(MapHeightPx(level)), Color{12, 16, 14, 255});

  const bool can_draw_final_render =
      mode == LevelRenderMode::kFinalRenderReference &&
      final_render_texture != nullptr && final_render_texture->id > 0;
  const bool can_draw_prepared_visual =
      prepared_level != nullptr && prepared_level->prepared_visual_map.loaded &&
      prepared_level->prepared_visual_map.HasRenderableLayer();
  const bool forest_assets_ready = prepared_level != nullptr &&
                                   EnsureForestAssetsLoaded();
  if (can_draw_final_render) {
    DrawFinalRenderReference(*final_render_texture, level);
    if (forest_assets_ready) {
      DrawForestAssetsForVisualPreview(level, *prepared_level, forest_assets_,
                                       level.size.tile_size, range);
    }
  } else if (mode == LevelRenderMode::kPreparedVisualMap &&
             can_draw_prepared_visual) {
    const visual_pipeline::VisualLayerGrid* layer = FindRenderableVisualLayer(
        prepared_level->prepared_visual_map);
    if (layer != nullptr) {
      DrawPreparedVisualTiles(*layer, level.size.tile_size, range);
      DrawPreparedVisualObjects(prepared_level->prepared_visual_map,
                                level.size.tile_size, range);
    }
  } else if (mode == LevelRenderMode::kCppAnalysis &&
             prepared_level != nullptr) {
    DrawCppAnalysisTiles(level, *prepared_level, range);
    DrawAnalysisOverlay(*prepared_level, level.size.tile_size, view.zoom,
                        range);
  } else if (mode == LevelRenderMode::kForestClearingAnalysis &&
             prepared_level != nullptr) {
    DrawForestClearingAnalysisTiles(level, *prepared_level, range);
  } else if (mode == LevelRenderMode::kVisualIntentPreview &&
             prepared_level != nullptr) {
    DrawVisualIntentPreviewTiles(level, *prepared_level, range);
    if (forest_assets_ready) {
      DrawForestAssetsForVisualPreview(level, *prepared_level, forest_assets_,
                                       level.size.tile_size, range);
    }
    if (prepared_level->micro_scene_visual_plan.IsValid()) {
      DrawMicroSceneVisualPlanForPreview(
          prepared_level->micro_scene_visual_plan, level.size.tile_size, range);
    }
    if (prepared_level->object_visual_plan.IsValid()) {
      DrawObjectVisualPlanForPreview(prepared_level->object_visual_plan,
                                     level.size.tile_size, range);
    } else {
      DrawRuntimeObjectsForVisualPreview(level, range);
    }
  } else {
    DrawRawTerrainTiles(level, range);
  }

  DrawRectangleLinesEx(Rectangle{0.0F, 0.0F, MapWidthPx(level),
                                 MapHeightPx(level)},
                       2.0F / view.zoom, Color{180, 180, 190, 160});

  if (mode == LevelRenderMode::kRawTerrain ||
      mode == LevelRenderMode::kCppAnalysis ||
      mode == LevelRenderMode::kForestClearingAnalysis) {
    DrawDebugMarkers(level, view.zoom);
  }

  EndMode2D();
}

void LevelRenderer::DrawTerrain(const LevelData& level,
                                const LevelViewState& view,
                                const WindowState& window) const {
  Draw(level, nullptr, view, window, LevelRenderMode::kRawTerrain);
}

}  // namespace sar
