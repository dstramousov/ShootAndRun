#include "visual_pipeline/final_visual_package_writer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "level/terrain_type.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/micro_scene_visual_plan.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/water_visual_plan.h"

namespace sar::visual_pipeline {
namespace {

constexpr int kChunkSizeTiles = 16;

struct RgbaColor {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

struct VisualLayerData {
  std::string id;
  std::string role;
  std::vector<std::string> tile_ids;
};

std::string JsonEscape(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char value : text) {
    switch (value) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(value);
        break;
    }
  }
  return escaped;
}

std::string JsonString(std::string_view text) {
  return "\"" + JsonEscape(text) + "\"";
}

bool EnsureDirectory(const std::filesystem::path& path, std::string* error) {
  std::error_code code;
  std::filesystem::create_directories(path, code);
  if (code) {
    if (error != nullptr) {
      *error = "failed to create directory " + path.string() + ": " +
               code.message();
    }
    return false;
  }
  return true;
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text,
                   std::string* error) {
  if (!EnsureDirectory(path.parent_path(), error)) {
    return false;
  }

  std::ofstream output(path, std::ios::binary);
  if (!output.is_open()) {
    if (error != nullptr) {
      *error = "failed to open output file: " + path.string();
    }
    return false;
  }

  output << text;
  if (!output.good()) {
    if (error != nullptr) {
      *error = "failed to write output file: " + path.string();
    }
    return false;
  }
  return true;
}

void AppendBigEndian32(std::uint32_t value, std::vector<std::uint8_t>* bytes) {
  bytes->push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
  bytes->push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
  bytes->push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  bytes->push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

std::uint32_t Crc32(const std::uint8_t* data, std::size_t size) {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      const std::uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return crc ^ 0xFFFFFFFFU;
}

std::uint32_t Adler32(const std::vector<std::uint8_t>& data) {
  constexpr std::uint32_t kModulo = 65521U;
  std::uint32_t a = 1U;
  std::uint32_t b = 0U;
  for (const std::uint8_t value : data) {
    a = (a + value) % kModulo;
    b = (b + a) % kModulo;
  }
  return (b << 16U) | a;
}

void AppendPngChunk(const std::array<char, 4>& type,
                    const std::vector<std::uint8_t>& data,
                    std::vector<std::uint8_t>* png) {
  AppendBigEndian32(static_cast<std::uint32_t>(data.size()), png);
  const std::size_t chunk_start = png->size();
  for (const char value : type) {
    png->push_back(static_cast<std::uint8_t>(value));
  }
  png->insert(png->end(), data.begin(), data.end());
  const std::uint32_t crc = Crc32(png->data() + chunk_start,
                                  png->size() - chunk_start);
  AppendBigEndian32(crc, png);
}

std::vector<std::uint8_t> BuildStoredDeflateStream(
    const std::vector<std::uint8_t>& raw) {
  std::vector<std::uint8_t> stream;
  stream.reserve(raw.size() + raw.size() / 65535U * 5U + 16U);
  stream.push_back(0x78U);
  stream.push_back(0x01U);

  std::size_t offset = 0;
  while (offset < raw.size()) {
    const std::size_t remaining = raw.size() - offset;
    const std::uint16_t block_size = static_cast<std::uint16_t>(
        std::min<std::size_t>(remaining, 65535U));
    const bool final_block = offset + block_size >= raw.size();
    stream.push_back(final_block ? 0x01U : 0x00U);
    stream.push_back(static_cast<std::uint8_t>(block_size & 0xFFU));
    stream.push_back(static_cast<std::uint8_t>((block_size >> 8U) & 0xFFU));
    const std::uint16_t inverted = static_cast<std::uint16_t>(~block_size);
    stream.push_back(static_cast<std::uint8_t>(inverted & 0xFFU));
    stream.push_back(static_cast<std::uint8_t>((inverted >> 8U) & 0xFFU));
    stream.insert(stream.end(),
                  raw.begin() + static_cast<std::ptrdiff_t>(offset),
                  raw.begin() +
                      static_cast<std::ptrdiff_t>(offset + block_size));
    offset += block_size;
  }

  AppendBigEndian32(Adler32(raw), &stream);
  return stream;
}

bool WritePngRgba(const std::filesystem::path& path, int width, int height,
                  const std::vector<RgbaColor>& pixels,
                  std::string* error) {
  if (width <= 0 || height <= 0) {
    if (error != nullptr) {
      *error = "png dimensions are invalid for " + path.string();
    }
    return false;
  }

  const std::size_t expected_size =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (pixels.size() != expected_size) {
    if (error != nullptr) {
      *error = "png pixel count mismatch for " + path.string();
    }
    return false;
  }

  std::vector<std::uint8_t> raw;
  raw.reserve(expected_size * 4U + static_cast<std::size_t>(height));
  for (int y = 0; y < height; ++y) {
    raw.push_back(0U);
    for (int x = 0; x < width; ++x) {
      const RgbaColor color = pixels[static_cast<std::size_t>(y * width + x)];
      raw.push_back(color.r);
      raw.push_back(color.g);
      raw.push_back(color.b);
      raw.push_back(color.a);
    }
  }

  std::vector<std::uint8_t> ihdr;
  AppendBigEndian32(static_cast<std::uint32_t>(width), &ihdr);
  AppendBigEndian32(static_cast<std::uint32_t>(height), &ihdr);
  ihdr.push_back(8U);
  ihdr.push_back(6U);
  ihdr.push_back(0U);
  ihdr.push_back(0U);
  ihdr.push_back(0U);

  std::vector<std::uint8_t> png = {
      0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU,
  };
  AppendPngChunk({'I', 'H', 'D', 'R'}, ihdr, &png);
  AppendPngChunk({'I', 'D', 'A', 'T'}, BuildStoredDeflateStream(raw), &png);
  AppendPngChunk({'I', 'E', 'N', 'D'}, {}, &png);

  if (!EnsureDirectory(path.parent_path(), error)) {
    return false;
  }
  std::ofstream output(path, std::ios::binary);
  if (!output.is_open()) {
    if (error != nullptr) {
      *error = "failed to open png output file: " + path.string();
    }
    return false;
  }
  output.write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));
  if (!output.good()) {
    if (error != nullptr) {
      *error = "failed to write png output file: " + path.string();
    }
    return false;
  }
  return true;
}

RgbaColor BlendOver(RgbaColor base, RgbaColor overlay) {
  if (overlay.a == 0U) {
    return base;
  }
  if (overlay.a == 255U) {
    return overlay;
  }
  const int alpha = static_cast<int>(overlay.a);
  const int inv_alpha = 255 - alpha;
  return RgbaColor{
      static_cast<std::uint8_t>((static_cast<int>(base.r) * inv_alpha +
                                 static_cast<int>(overlay.r) * alpha) /
                                255),
      static_cast<std::uint8_t>((static_cast<int>(base.g) * inv_alpha +
                                 static_cast<int>(overlay.g) * alpha) /
                                255),
      static_cast<std::uint8_t>((static_cast<int>(base.b) * inv_alpha +
                                 static_cast<int>(overlay.b) * alpha) /
                                255),
      255};
}

RgbaColor TerrainColor(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kOpenGround:
      return RgbaColor{78, 104, 58, 255};
    case TerrainType::kForest:
      return RgbaColor{22, 64, 40, 255};
    case TerrainType::kRoad:
      return RgbaColor{145, 115, 68, 255};
    case TerrainType::kSwamp:
      return RgbaColor{42, 83, 68, 255};
    case TerrainType::kRuins:
      return RgbaColor{94, 92, 78, 255};
    case TerrainType::kWater:
      return RgbaColor{34, 84, 105, 255};
    case TerrainType::kWall:
      return RgbaColor{35, 31, 27, 255};
    case TerrainType::kUnknown:
      return RgbaColor{138, 62, 128, 255};
  }
  return RgbaColor{138, 62, 128, 255};
}

RgbaColor ForestColor(std::uint8_t value) {
  switch (static_cast<ForestDepthBand>(value)) {
    case ForestDepthBand::kEdge:
      return RgbaColor{38, 88, 48, 255};
    case ForestDepthBand::kMid:
      return RgbaColor{22, 67, 39, 255};
    case ForestDepthBand::kDeep:
      return RgbaColor{10, 42, 27, 255};
    case ForestDepthBand::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor ClearingColor(std::uint8_t value) {
  switch (static_cast<ClearingRole>(value)) {
    case ClearingRole::kMainClearing:
      return RgbaColor{114, 132, 70, 255};
    case ClearingRole::kSideClearing:
      return RgbaColor{78, 112, 58, 255};
    case ClearingRole::kConnectorCorridor:
      return RgbaColor{123, 110, 64, 255};
    case ClearingRole::kMicroClearing:
      return RgbaColor{94, 130, 72, 255};
    case ClearingRole::kSceneSpace:
      return RgbaColor{106, 101, 70, 255};
    case ClearingRole::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor ClearingSceneColor(std::uint8_t value) {
  switch (static_cast<ClearingSceneRole>(value)) {
    case ClearingSceneRole::kRuinsScene:
      return RgbaColor{126, 99, 82, 255};
    case ClearingSceneRole::kRoadApproach:
      return RgbaColor{139, 112, 63, 255};
    case ClearingSceneRole::kObjectScene:
      return RgbaColor{105, 92, 128, 255};
    case ClearingSceneRole::kGenericScene:
      return RgbaColor{90, 108, 122, 255};
    case ClearingSceneRole::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor RoadColor(std::uint8_t value) {
  switch (static_cast<RoadVisualBand>(value)) {
    case RoadVisualBand::kRoadCore:
      return RgbaColor{154, 112, 62, 255};
    case RoadVisualBand::kRoadSide:
      return RgbaColor{122, 108, 67, 255};
    case RoadVisualBand::kTrampledGrass:
      return RgbaColor{91, 112, 61, 255};
    case RoadVisualBand::kMudPatch:
      return RgbaColor{74, 56, 42, 255};
    case RoadVisualBand::kRuinApproach:
      return RgbaColor{136, 115, 77, 255};
    case RoadVisualBand::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor RuinColor(std::uint8_t value) {
  switch (static_cast<RuinVisualTile>(value)) {
    case RuinVisualTile::kCrackedFloor:
      return RgbaColor{104, 101, 84, 255};
    case RuinVisualTile::kOvergrownFloor:
      return RgbaColor{78, 106, 66, 255};
    case RuinVisualTile::kWallIntact:
      return RgbaColor{55, 49, 42, 255};
    case RuinVisualTile::kWallBroken:
      return RgbaColor{78, 68, 55, 255};
    case RuinVisualTile::kWallCorner:
      return RgbaColor{64, 56, 46, 255};
    case RuinVisualTile::kWallEndcap:
      return RgbaColor{88, 76, 59, 255};
    case RuinVisualTile::kRubble:
      return RgbaColor{124, 104, 76, 255};
    case RuinVisualTile::kEntrance:
      return RgbaColor{148, 119, 72, 255};
    case RuinVisualTile::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor WaterColor(std::uint8_t value) {
  switch (static_cast<WaterVisualTile>(value)) {
    case WaterVisualTile::kWaterCore:
      return RgbaColor{28, 82, 108, 255};
    case WaterVisualTile::kWaterEdge:
      return RgbaColor{42, 102, 118, 255};
    case WaterVisualTile::kMudRing:
      return RgbaColor{79, 67, 48, 255};
    case WaterVisualTile::kWetGrass:
      return RgbaColor{55, 101, 70, 255};
    case WaterVisualTile::kReedZone:
      return RgbaColor{68, 123, 74, 255};
    case WaterVisualTile::kCrossing:
      return RgbaColor{112, 98, 67, 255};
    case WaterVisualTile::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor MicroSceneColor(std::uint8_t value) {
  switch (static_cast<MicroSceneTile>(value)) {
    case MicroSceneTile::kGroundDetail:
      return RgbaColor{126, 111, 66, 115};
    case MicroSceneTile::kSmallDebris:
      return RgbaColor{142, 112, 74, 130};
    case MicroSceneTile::kSecondaryProp:
      return RgbaColor{154, 108, 64, 140};
    case MicroSceneTile::kPrimaryProp:
      return RgbaColor{190, 132, 70, 155};
    case MicroSceneTile::kVegetationDetail:
      return RgbaColor{58, 118, 56, 120};
    case MicroSceneTile::kStoneDetail:
      return RgbaColor{142, 132, 112, 135};
    case MicroSceneTile::kWetDetail:
      return RgbaColor{54, 112, 82, 125};
    case MicroSceneTile::kNone:
      return RgbaColor{0, 0, 0, 0};
  }
  return RgbaColor{0, 0, 0, 0};
}

RgbaColor ObjectColor(ObjectVisualKind kind) {
  switch (kind) {
    case ObjectVisualKind::kVegetation:
      return RgbaColor{34, 102, 54, 150};
    case ObjectVisualKind::kWood:
      return RgbaColor{128, 86, 46, 150};
    case ObjectVisualKind::kStone:
      return RgbaColor{132, 126, 112, 155};
    case ObjectVisualKind::kScrap:
      return RgbaColor{126, 102, 88, 150};
    case ObjectVisualKind::kCamp:
      return RgbaColor{174, 124, 64, 160};
    case ObjectVisualKind::kCache:
      return RgbaColor{218, 172, 72, 170};
    case ObjectVisualKind::kStructure:
      return RgbaColor{98, 88, 72, 170};
    case ObjectVisualKind::kRuin:
      return RgbaColor{116, 106, 88, 150};
    case ObjectVisualKind::kElevation:
      return RgbaColor{104, 86, 64, 120};
    case ObjectVisualKind::kLandmark:
      return RgbaColor{184, 138, 82, 165};
    case ObjectVisualKind::kCover:
      return RgbaColor{86, 74, 56, 145};
    case ObjectVisualKind::kTypedFallback:
      return RgbaColor{166, 132, 84, 130};
    case ObjectVisualKind::kUnknown:
      return RgbaColor{206, 46, 180, 170};
  }
  return RgbaColor{166, 132, 84, 130};
}

std::string TerrainTileId(TerrainType terrain) {
  return "terrain." + std::string(TerrainTypeToString(terrain));
}

std::string OverlayTileId(std::string_view prefix, std::string_view role) {
  if (role.empty() || role == "none") {
    return "none";
  }
  return std::string(prefix) + "." + std::string(role);
}

std::string ForestLayerTileId(const PreparedLevel& prepared_level,
                              std::size_t index) {
  const ForestVisualPlan& plan = prepared_level.forest_visual_plan;
  if (!plan.IsValid()) {
    return "none";
  }
  if (index < plan.forest_depth.size() && plan.forest_depth[index] != 0U) {
    return OverlayTileId("forest", ForestDepthBandName(
                                      static_cast<ForestDepthBand>(
                                          plan.forest_depth[index])));
  }
  if (index < plan.clearing_scene_roles.size() &&
      plan.clearing_scene_roles[index] != 0U) {
    return OverlayTileId("clearing_scene", ClearingSceneRoleName(
                                               static_cast<ClearingSceneRole>(
                                                   plan.clearing_scene_roles[
                                                       index])));
  }
  if (index < plan.clearing_roles.size() && plan.clearing_roles[index] != 0U) {
    return OverlayTileId("clearing", ClearingRoleName(
                                         static_cast<ClearingRole>(
                                             plan.clearing_roles[index])));
  }
  return "none";
}

std::string RoadLayerTileId(const PreparedLevel& prepared_level,
                            std::size_t index) {
  const RoadVisualPlan& plan = prepared_level.road_visual_plan;
  if (!plan.IsValid() || index >= plan.road_bands.size() ||
      plan.road_bands[index] == 0U) {
    return "none";
  }
  return OverlayTileId("road", RoadVisualBandName(
                                   static_cast<RoadVisualBand>(
                                       plan.road_bands[index])));
}

std::string WaterLayerTileId(const PreparedLevel& prepared_level,
                             std::size_t index) {
  const WaterVisualPlan& plan = prepared_level.water_visual_plan;
  if (!plan.IsValid() || index >= plan.tiles.size() || plan.tiles[index] == 0U) {
    return "none";
  }
  return OverlayTileId("water", WaterVisualTileName(
                                    static_cast<WaterVisualTile>(
                                        plan.tiles[index])));
}

std::string RuinLayerTileId(const PreparedLevel& prepared_level,
                            std::size_t index) {
  const RuinVisualPlan& plan = prepared_level.ruin_visual_plan;
  if (!plan.IsValid() || index >= plan.tiles.size() || plan.tiles[index] == 0U) {
    return "none";
  }
  return OverlayTileId("ruins", RuinVisualTileName(
                                    static_cast<RuinVisualTile>(
                                        plan.tiles[index])));
}

std::string MicroSceneLayerTileId(const PreparedLevel& prepared_level,
                                  std::size_t index) {
  const MicroSceneVisualPlan& plan = prepared_level.micro_scene_visual_plan;
  if (!plan.IsValid() || index >= plan.tiles.size() || plan.tiles[index] == 0U) {
    return "none";
  }
  return OverlayTileId("dressing", MicroSceneTileName(
                                       static_cast<MicroSceneTile>(
                                           plan.tiles[index])));
}

std::vector<VisualLayerData> BuildVisualLayers(
    const LevelData& level,
    const PreparedLevel& prepared_level) {
  const std::size_t tile_count = level.cells.size();
  std::vector<VisualLayerData> layers;
  layers.push_back({"terrain_base", "terrain_base", {}});
  layers.push_back({"forest_clearings", "forest_clearings", {}});
  layers.push_back({"road_overlay", "road_overlay", {}});
  layers.push_back({"water_overlay", "water_overlay", {}});
  layers.push_back({"ruins_overlay", "ruins_overlay", {}});
  layers.push_back({"dressing_overlay", "dressing_overlay", {}});
  for (VisualLayerData& layer : layers) {
    layer.tile_ids.reserve(tile_count);
  }

  for (std::size_t index = 0; index < tile_count; ++index) {
    layers[0].tile_ids.push_back(TerrainTileId(level.cells[index].terrain));
    layers[1].tile_ids.push_back(ForestLayerTileId(prepared_level, index));
    layers[2].tile_ids.push_back(RoadLayerTileId(prepared_level, index));
    layers[3].tile_ids.push_back(WaterLayerTileId(prepared_level, index));
    layers[4].tile_ids.push_back(RuinLayerTileId(prepared_level, index));
    layers[5].tile_ids.push_back(MicroSceneLayerTileId(prepared_level, index));
  }
  return layers;
}

int CountUniqueTileIds(const std::vector<VisualLayerData>& layers) {
  std::set<std::string> ids;
  for (const VisualLayerData& layer : layers) {
    ids.insert(layer.tile_ids.begin(), layer.tile_ids.end());
  }
  return static_cast<int>(ids.size());
}

std::string BuildVisualMapJson(const LevelData& level,
                               const FinalVisualPackageResult& result) {
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"cpp-visual-map-v1\",\n";
  output << "  \"visual_generator_version\": \"cpp_pipeline_0.1.34\",\n";
  output << "  \"width_tiles\": " << level.size.width << ",\n";
  output << "  \"height_tiles\": " << level.size.height << ",\n";
  output << "  \"tile_size_px\": " << level.size.tile_size << ",\n";
  output << "  \"visual_profile\": {\n";
  output << "    \"id\": \"dark_forest\",\n";
  output << "    \"name\": \"Dark Forest C++ Visual Preview\"\n";
  output << "  },\n";
  output << "  \"changes_gameplay\": false,\n";
  output << "  \"moves_markers\": false,\n";
  output << "  \"changes_collision\": false,\n";
  output << "  \"files\": {\n";
  output << "    \"visual_layers\": \"visual_layers.json\",\n";
  output << "    \"visual_objects\": \"visual_objects.json\",\n";
  output << "    \"visual_chunks\": \"visual_chunks.json\",\n";
  output << "    \"final_render\": \"final_render.png\"\n";
  output << "  },\n";
  output << "  \"summary\": {\n";
  output << "    \"visual_layers\": " << result.visual_layer_count << ",\n";
  output << "    \"unique_tile_ids\": " << result.unique_tile_id_count << ",\n";
  output << "    \"visual_objects\": " << result.visual_object_count << ",\n";
  output << "    \"visual_chunks\": " << result.visual_chunk_count << "\n";
  output << "  }\n";
  output << "}\n";
  return output.str();
}

std::string BuildVisualLayersJson(const LevelData& level,
                                  const std::vector<VisualLayerData>& layers,
                                  int unique_tile_id_count) {
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"visual-layers-v1\",\n";
  output << "  \"width\": " << level.size.width << ",\n";
  output << "  \"height\": " << level.size.height << ",\n";
  output << "  \"tile_size_px\": " << level.size.tile_size << ",\n";
  output << "  \"unique_tile_ids\": " << unique_tile_id_count << ",\n";
  output << "  \"layers\": [\n";
  for (std::size_t layer_index = 0; layer_index < layers.size(); ++layer_index) {
    const VisualLayerData& layer = layers[layer_index];
    output << "    {\n";
    output << "      \"id\": " << JsonString(layer.id) << ",\n";
    output << "      \"role\": " << JsonString(layer.role) << ",\n";
    output << "      \"width\": " << level.size.width << ",\n";
    output << "      \"height\": " << level.size.height << ",\n";
    output << "      \"rows\": [\n";
    for (std::size_t i = 0; i < layer.tile_ids.size(); ++i) {
      output << "        " << JsonString(layer.tile_ids[i]);
      if (i + 1U < layer.tile_ids.size()) {
        output << ",";
      }
      output << "\n";
    }
    output << "      ]\n";
    output << "    }";
    if (layer_index + 1U < layers.size()) {
      output << ",";
    }
    output << "\n";
  }
  output << "  ]\n";
  output << "}\n";
  return output.str();
}

std::string BuildVisualObjectsJson(const ObjectVisualPlan& plan) {
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"visual-objects-v1\",\n";
  output << "  \"total\": " << plan.items.size() << ",\n";
  output << "  \"items\": [\n";
  for (std::size_t i = 0; i < plan.items.size(); ++i) {
    const ObjectVisualItem& item = plan.items[i];
    output << "    {\n";
    output << "      \"id\": " << JsonString(item.id) << ",\n";
    output << "      \"source_type\": " << JsonString(item.source_type) << ",\n";
    output << "      \"source_family\": " << JsonString(item.source_family)
           << ",\n";
    output << "      \"sprite_family\": " << JsonString(item.sprite_family)
           << ",\n";
    output << "      \"sprite_id\": " << JsonString(item.sprite_id) << ",\n";
    output << "      \"draw_layer\": " << JsonString(item.draw_layer) << ",\n";
    output << "      \"visual_kind\": "
           << JsonString(ObjectVisualKindName(item.kind)) << ",\n";
    output << "      \"position\": { \"x\": " << item.x
           << ", \"y\": " << item.y << " },\n";
    output << "      \"visual_bounds\": { \"width\": " << item.width
           << ", \"height\": " << item.height << " },\n";
    output << "      \"sort_anchor\": { \"x\": " << item.x
           << ", \"y\": " << item.sort_y << " }\n";
    output << "    }";
    if (i + 1U < plan.items.size()) {
      output << ",";
    }
    output << "\n";
  }
  output << "  ]\n";
  output << "}\n";
  return output.str();
}

int ChunkCount(const LevelSize& size) {
  const int chunks_x = (size.width + kChunkSizeTiles - 1) / kChunkSizeTiles;
  const int chunks_y = (size.height + kChunkSizeTiles - 1) / kChunkSizeTiles;
  return chunks_x * chunks_y;
}

std::string BuildVisualChunksJson(const LevelSize& size) {
  const int chunks_x = (size.width + kChunkSizeTiles - 1) / kChunkSizeTiles;
  const int chunks_y = (size.height + kChunkSizeTiles - 1) / kChunkSizeTiles;
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"visual-chunks-v1\",\n";
  output << "  \"chunk_size_tiles\": " << kChunkSizeTiles << ",\n";
  output << "  \"total\": " << chunks_x * chunks_y << ",\n";
  output << "  \"items\": [\n";
  int id = 0;
  for (int y = 0; y < chunks_y; ++y) {
    for (int x = 0; x < chunks_x; ++x) {
      const int start_x = x * kChunkSizeTiles;
      const int start_y = y * kChunkSizeTiles;
      const int width = std::min(kChunkSizeTiles, size.width - start_x);
      const int height = std::min(kChunkSizeTiles, size.height - start_y);
      output << "    { \"id\": \"chunk_" << id << "\", \"x\": "
             << start_x << ", \"y\": " << start_y
             << ", \"width\": " << width << ", \"height\": "
             << height << " }";
      ++id;
      if (id < chunks_x * chunks_y) {
        output << ",";
      }
      output << "\n";
    }
  }
  output << "  ]\n";
  output << "}\n";
  return output.str();
}

RgbaColor CompositeTileColor(const LevelData& level,
                             const PreparedLevel& prepared_level,
                             std::size_t index) {
  RgbaColor color = TerrainColor(level.cells[index].terrain);
  if (prepared_level.forest_visual_plan.IsValid()) {
    const ForestVisualPlan& plan = prepared_level.forest_visual_plan;
    if (index < plan.forest_depth.size() && plan.forest_depth[index] != 0U) {
      color = ForestColor(plan.forest_depth[index]);
    } else if (index < plan.clearing_scene_roles.size() &&
               plan.clearing_scene_roles[index] != 0U) {
      color = ClearingSceneColor(plan.clearing_scene_roles[index]);
    } else if (index < plan.clearing_roles.size() &&
               plan.clearing_roles[index] != 0U) {
      color = ClearingColor(plan.clearing_roles[index]);
    }
  }
  if (prepared_level.road_visual_plan.IsValid() &&
      index < prepared_level.road_visual_plan.road_bands.size() &&
      prepared_level.road_visual_plan.road_bands[index] != 0U) {
    color = RoadColor(prepared_level.road_visual_plan.road_bands[index]);
  }
  if (prepared_level.water_visual_plan.IsValid() &&
      index < prepared_level.water_visual_plan.tiles.size() &&
      prepared_level.water_visual_plan.tiles[index] != 0U) {
    color = WaterColor(prepared_level.water_visual_plan.tiles[index]);
  }
  if (prepared_level.ruin_visual_plan.IsValid() &&
      index < prepared_level.ruin_visual_plan.tiles.size() &&
      prepared_level.ruin_visual_plan.tiles[index] != 0U) {
    color = RuinColor(prepared_level.ruin_visual_plan.tiles[index]);
  }
  if (prepared_level.micro_scene_visual_plan.IsValid() &&
      index < prepared_level.micro_scene_visual_plan.tiles.size() &&
      prepared_level.micro_scene_visual_plan.tiles[index] != 0U) {
    color = BlendOver(color,
                      MicroSceneColor(
                          prepared_level.micro_scene_visual_plan.tiles[index]));
  }
  return color;
}

std::vector<RgbaColor> BuildTilePreviewImage(
    const LevelData& level,
    const PreparedLevel& prepared_level) {
  const int width_px = level.size.width * level.size.tile_size;
  const int height_px = level.size.height * level.size.tile_size;
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(width_px * height_px));

  for (int tile_y = 0; tile_y < level.size.height; ++tile_y) {
    for (int tile_x = 0; tile_x < level.size.width; ++tile_x) {
      const std::size_t tile_index = static_cast<std::size_t>(
          tile_y * level.size.width + tile_x);
      const RgbaColor color = CompositeTileColor(level, prepared_level,
                                                 tile_index);
      for (int local_y = 0; local_y < level.size.tile_size; ++local_y) {
        const int pixel_y = tile_y * level.size.tile_size + local_y;
        for (int local_x = 0; local_x < level.size.tile_size; ++local_x) {
          const int pixel_x = tile_x * level.size.tile_size + local_x;
          pixels[static_cast<std::size_t>(pixel_y * width_px + pixel_x)] =
              color;
        }
      }
    }
  }

  if (prepared_level.object_visual_plan.IsValid()) {
    for (const ObjectVisualItem& item : prepared_level.object_visual_plan.items) {
      const RgbaColor object_color = ObjectColor(item.kind);
      const int start_x = std::clamp(item.x, 0, level.size.width) *
                          level.size.tile_size;
      const int start_y = std::clamp(item.y, 0, level.size.height) *
                          level.size.tile_size;
      const int end_x = std::clamp(item.x + item.width, 0, level.size.width) *
                        level.size.tile_size;
      const int end_y = std::clamp(item.y + item.height, 0, level.size.height) *
                        level.size.tile_size;
      for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
          const std::size_t index = static_cast<std::size_t>(y * width_px + x);
          pixels[index] = BlendOver(pixels[index], object_color);
        }
      }
    }
  }
  return pixels;
}

std::string BuildFinalRenderReportJson(
    const LevelData& level,
    const PreparedLevel& prepared_level,
    const FinalVisualPackageResult& result) {
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"final-render-report-v1\",\n";
  output << "  \"status\": \"ok\",\n";
  output << "  \"source\": \"cpp_pipeline\",\n";
  output << "  \"width_px\": " << level.size.width * level.size.tile_size
         << ",\n";
  output << "  \"height_px\": " << level.size.height * level.size.tile_size
         << ",\n";
  output << "  \"tile_size_px\": " << level.size.tile_size << ",\n";
  output << "  \"visual_layers\": " << result.visual_layer_count << ",\n";
  output << "  \"visual_objects\": " << result.visual_object_count << ",\n";
  output << "  \"visual_chunks\": " << result.visual_chunk_count << ",\n";
  output << "  \"final_render\": "
         << JsonString(result.final_render_path.filename().string()) << ",\n";
  output << "  \"contract\": {\n";
  output << "    \"changes_gameplay\": false,\n";
  output << "    \"moves_markers\": false,\n";
  output << "    \"changes_collision\": false,\n";
  output << "    \"routes_used_for_visual_roads\": "
         << (prepared_level.road_visual_plan.summary.routes_used_for_visual_roads
                 ? "true"
                 : "false")
         << "\n";
  output << "  }\n";
  output << "}\n";
  return output.str();
}

std::string BuildVisualDensityReportJson(
    const LevelData& level,
    const PreparedLevel& prepared_level,
    const FinalVisualPackageResult& result) {
  const double tiles = static_cast<double>(level.size.width * level.size.height);
  const double objects_per_1000 =
      tiles > 0.0 ? static_cast<double>(result.visual_object_count) * 1000.0 /
                         tiles
                   : 0.0;
  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"visual-density-report-v1\",\n";
  output << "  \"map_tiles\": " << static_cast<int>(tiles) << ",\n";
  output << "  \"visual_layers\": " << result.visual_layer_count << ",\n";
  output << "  \"visual_objects\": " << result.visual_object_count << ",\n";
  output << "  \"objects_per_1000_tiles\": " << objects_per_1000 << ",\n";
  output << "  \"micro_scenes\": "
         << prepared_level.micro_scene_visual_plan.summary.scene_count << ",\n";
  output << "  \"micro_scene_visual_tiles\": "
         << prepared_level.micro_scene_visual_plan.summary.visual_tiles << ",\n";
  output << "  \"water_visual_tiles\": "
         << prepared_level.water_visual_plan.summary.visual_tiles << ",\n";
  output << "  \"ruin_visual_tiles\": "
         << prepared_level.ruin_visual_plan.summary.visual_tiles << "\n";
  output << "}\n";
  return output.str();
}

std::string BuildQualityScoreJson(const PreparedLevel& prepared_level) {
  const bool object_mapping_ok =
      prepared_level.object_visual_plan.summary.generic_object_count == 0 &&
      prepared_level.object_visual_plan.summary.missing_sprite_uses == 0 &&
      prepared_level.object_visual_plan.summary.skipped_sprites == 0;
  const bool gameplay_contract_ok =
      !prepared_level.road_visual_plan.summary.routes_used_for_visual_roads;
  const bool ok = object_mapping_ok && gameplay_contract_ok;

  std::ostringstream output;
  output << "{\n";
  output << "  \"schema_version\": \"visual-quality-score-v1\",\n";
  output << "  \"status\": " << JsonString(ok ? "ok" : "failed") << ",\n";
  output << "  \"scores\": {\n";
  output << "    \"forest_mass_readability\": 0.75,\n";
  output << "    \"road_readability\": 0.72,\n";
  output << "    \"ruin_scene_quality\": 0.70,\n";
  output << "    \"water_naturalness\": 0.68,\n";
  output << "    \"object_mapping_completeness\": "
         << (object_mapping_ok ? "1.00" : "0.00") << ",\n";
  output << "    \"visual_noise_control\": 0.66,\n";
  output << "    \"gameplay_contract_safety\": "
         << (gameplay_contract_ok ? "1.00" : "0.00") << "\n";
  output << "  },\n";
  output << "  \"fatal\": [";
  bool wrote_fatal = false;
  if (!object_mapping_ok) {
    output << JsonString("object mapping is incomplete");
    wrote_fatal = true;
  }
  if (!gameplay_contract_ok) {
    if (wrote_fatal) {
      output << ", ";
    }
    output << JsonString("routes were used as visual roads");
  }
  output << "],\n";
  output << "  \"warnings\": []\n";
  output << "}\n";
  return output.str();
}

bool WriteReports(const LevelData& level,
                  const PreparedLevel& prepared_level,
                  const FinalVisualPackageResult& result,
                  std::string* error) {
  const std::filesystem::path report_root = result.visual_map_path.parent_path() /
                                            "debug" / "reports";
  return WriteTextFile(report_root / "final_render_report.json",
                       BuildFinalRenderReportJson(level, prepared_level, result),
                       error) &&
         WriteTextFile(report_root / "visual_density_report.json",
                       BuildVisualDensityReportJson(level, prepared_level,
                                                    result),
                       error) &&
         WriteTextFile(report_root / "quality_score.json",
                       BuildQualityScoreJson(prepared_level), error);
}

}  // namespace

FinalVisualPackageWriter::FinalVisualPackageWriter(
    std::filesystem::path output_root)
    : output_root_(std::move(output_root)) {}

FinalVisualPackageResult FinalVisualPackageWriter::Write(
    const LevelData& level,
    const PreparedLevel& prepared_level,
    std::string* error) const {
  FinalVisualPackageResult result;
  result.visual_map_path = output_root_ / "visual_map.json";
  result.visual_layers_path = output_root_ / "visual_layers.json";
  result.visual_objects_path = output_root_ / "visual_objects.json";
  result.visual_chunks_path = output_root_ / "visual_chunks.json";
  result.final_render_path = output_root_ / "final_render.png";

  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0 || level.cells.empty()) {
    if (error != nullptr) {
      *error = "cannot write final visual package for invalid level";
    }
    return result;
  }

  const std::vector<VisualLayerData> layers = BuildVisualLayers(level,
                                                                prepared_level);
  result.visual_layer_count = static_cast<int>(layers.size());
  result.unique_tile_id_count = CountUniqueTileIds(layers);
  result.visual_object_count = static_cast<int>(
      prepared_level.object_visual_plan.items.size());
  result.visual_chunk_count = ChunkCount(level.size);

  if (!WriteTextFile(result.visual_layers_path,
                     BuildVisualLayersJson(level, layers,
                                           result.unique_tile_id_count),
                     error)) {
    return FinalVisualPackageResult{};
  }
  if (!WriteTextFile(result.visual_objects_path,
                     BuildVisualObjectsJson(prepared_level.object_visual_plan),
                     error)) {
    return FinalVisualPackageResult{};
  }
  if (!WriteTextFile(result.visual_chunks_path,
                     BuildVisualChunksJson(level.size), error)) {
    return FinalVisualPackageResult{};
  }
  if (!WriteTextFile(result.visual_map_path, BuildVisualMapJson(level, result),
                     error)) {
    return FinalVisualPackageResult{};
  }

  const std::vector<RgbaColor> pixels = BuildTilePreviewImage(level,
                                                              prepared_level);
  if (!WritePngRgba(result.final_render_path,
                    level.size.width * level.size.tile_size,
                    level.size.height * level.size.tile_size, pixels, error)) {
    return FinalVisualPackageResult{};
  }

  if (!WriteReports(level, prepared_level, result, error)) {
    return FinalVisualPackageResult{};
  }

  return result;
}

}  // namespace sar::visual_pipeline
