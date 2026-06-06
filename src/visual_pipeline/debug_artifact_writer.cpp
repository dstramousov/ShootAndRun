#include "visual_pipeline/debug_artifact_writer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "level/terrain_type.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/micro_scene_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/water_visual_plan.h"

namespace sar::visual_pipeline {
namespace {

struct RgbaColor {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

constexpr RgbaColor kBlack{0, 0, 0, 255};
constexpr RgbaColor kWhite{255, 255, 255, 255};
constexpr RgbaColor kForest{34, 91, 47, 255};
constexpr RgbaColor kOpenGround{127, 143, 82, 255};
constexpr RgbaColor kRoad{150, 116, 70, 255};
constexpr RgbaColor kSwamp{66, 98, 84, 255};
constexpr RgbaColor kWater{42, 93, 130, 255};
constexpr RgbaColor kRuins{128, 126, 116, 255};
constexpr RgbaColor kWall{83, 79, 74, 255};
constexpr RgbaColor kUnknown{190, 42, 160, 255};

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

bool StartsWith(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
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
  for (char value : type) {
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

  // zlib header: deflate, 32K window, fastest compression hint.
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
    stream.insert(stream.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset),
                  raw.begin() + static_cast<std::ptrdiff_t>(offset + block_size));
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
    raw.push_back(0U);  // Filter type: None.
    for (int x = 0; x < width; ++x) {
      const RgbaColor color =
          pixels[static_cast<std::size_t>(y * width + x)];
      raw.push_back(color.r);
      raw.push_back(color.g);
      raw.push_back(color.b);
      raw.push_back(color.a);
    }
  }

  std::vector<std::uint8_t> ihdr;
  AppendBigEndian32(static_cast<std::uint32_t>(width), &ihdr);
  AppendBigEndian32(static_cast<std::uint32_t>(height), &ihdr);
  ihdr.push_back(8U);  // Bit depth.
  ihdr.push_back(6U);  // RGBA.
  ihdr.push_back(0U);  // Compression.
  ihdr.push_back(0U);  // Filter.
  ihdr.push_back(0U);  // Interlace.

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

std::vector<RgbaColor> BuildBinaryMaskImage(
    const std::vector<std::uint8_t>& mask, const LevelSize& size,
    RgbaColor on_color) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(size.width) *
                                static_cast<std::size_t>(size.height),
                                kBlack);
  for (std::size_t index = 0; index < pixels.size(); ++index) {
    if (mask[index] != 0) {
      pixels[index] = on_color;
    }
  }
  return pixels;
}

std::vector<RgbaColor> BuildCompositeMaskImage(const SemanticMasks& masks) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(masks.size.width) *
                                static_cast<std::size_t>(masks.size.height),
                                kBlack);
  for (std::size_t index = 0; index < pixels.size(); ++index) {
    if (masks.open_ground[index] != 0) {
      pixels[index] = kOpenGround;
    }
    if (masks.forest[index] != 0) {
      pixels[index] = kForest;
    }
    if (masks.road[index] != 0) {
      pixels[index] = kRoad;
    }
    if (masks.swamp[index] != 0) {
      pixels[index] = kSwamp;
    }
    if (masks.water[index] != 0) {
      pixels[index] = kWater;
    }
    if (masks.ruins[index] != 0) {
      pixels[index] = kRuins;
    }
    if (masks.wall[index] != 0) {
      pixels[index] = kWall;
    }
    if (masks.unknown[index] != 0) {
      pixels[index] = kUnknown;
    }
  }
  return pixels;
}

RgbaColor RegionColor(TerrainType type, int region_id) {
  const std::uint8_t variation = static_cast<std::uint8_t>(
      32 + ((region_id * 37) % 96));
  switch (type) {
    case TerrainType::kOpenGround:
      return RgbaColor{static_cast<std::uint8_t>(100 + variation / 2),
                       static_cast<std::uint8_t>(120 + variation / 2), 70,
                       255};
    case TerrainType::kForest:
      return RgbaColor{20, static_cast<std::uint8_t>(75 + variation), 35, 255};
    case TerrainType::kRoad:
      return RgbaColor{static_cast<std::uint8_t>(120 + variation),
                       static_cast<std::uint8_t>(85 + variation / 3), 50, 255};
    case TerrainType::kSwamp:
      return RgbaColor{40, static_cast<std::uint8_t>(80 + variation / 2),
                       static_cast<std::uint8_t>(70 + variation / 2), 255};
    case TerrainType::kWater:
      return RgbaColor{35, static_cast<std::uint8_t>(70 + variation / 2),
                       static_cast<std::uint8_t>(110 + variation), 255};
    case TerrainType::kRuins:
      return RgbaColor{static_cast<std::uint8_t>(100 + variation / 2),
                       static_cast<std::uint8_t>(98 + variation / 2),
                       static_cast<std::uint8_t>(92 + variation / 2), 255};
    case TerrainType::kWall:
      return RgbaColor{70, 66, 62, 255};
    case TerrainType::kUnknown:
      return kUnknown;
  }
  return kUnknown;
}

std::vector<RgbaColor> BuildRegionImage(const TerrainRegions& regions,
                                        TerrainType type) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(regions.size.width) *
                                static_cast<std::size_t>(regions.size.height),
                                kBlack);
  for (const TerrainRegion& region : regions.regions) {
    if (region.type != type) {
      continue;
    }
    const RgbaColor fill = RegionColor(type, region.id);
    for (const int index : region.tile_indices) {
      pixels[static_cast<std::size_t>(index)] = fill;
    }
    for (const int index : region.border_tile_indices) {
      pixels[static_cast<std::size_t>(index)] = kWhite;
    }
  }
  return pixels;
}


void SetPixel(int x, int y, const LevelSize& size, RgbaColor color,
              std::vector<RgbaColor>* pixels) {
  if (x < 0 || y < 0 || x >= size.width || y >= size.height) {
    return;
  }
  (*pixels)[static_cast<std::size_t>(y * size.width + x)] = color;
}

void DrawPoint(int x, int y, int radius, const LevelSize& size,
               RgbaColor color, std::vector<RgbaColor>* pixels) {
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy <= radius * radius) {
        SetPixel(x + dx, y + dy, size, color, pixels);
      }
    }
  }
}

void DrawLine(int x0, int y0, int x1, int y1, int thickness,
              const LevelSize& size, RgbaColor color,
              std::vector<RgbaColor>* pixels) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error_value = dx + dy;

  int x = x0;
  int y = y0;
  while (true) {
    DrawPoint(x, y, thickness, size, color, pixels);
    if (x == x1 && y == y1) {
      break;
    }
    const int doubled_error = 2 * error_value;
    if (doubled_error >= dy) {
      error_value += dy;
      x += sx;
    }
    if (doubled_error <= dx) {
      error_value += dx;
      y += sy;
    }
  }
}

bool RouteIsMain(const Route& route) {
  if (route.type.find("main") != std::string::npos) {
    return true;
  }
  for (const std::string& tag : route.tags) {
    if (tag == "primary" || tag == "main_road" || tag == "critical") {
      return true;
    }
  }
  return false;
}

std::vector<RgbaColor> BuildRouteImage(const LevelData& level,
                                       bool influence_mode) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(level.size.width) *
                                static_cast<std::size_t>(level.size.height),
                                kBlack);
  for (const Route& route : level.routes) {
    if (route.waypoints.empty()) {
      continue;
    }
    const bool main_route = RouteIsMain(route);
    const RgbaColor color = main_route ? RgbaColor{230, 184, 80, 255}
                                      : RgbaColor{124, 104, 72, 255};
    const int thickness = influence_mode ? (main_route ? 3 : 2) : 0;
    for (std::size_t i = 1; i < route.waypoints.size(); ++i) {
      const RoutePoint& previous = route.waypoints[i - 1];
      const RoutePoint& current = route.waypoints[i];
      DrawLine(previous.x, previous.y, current.x, current.y, thickness,
               level.size, color, &pixels);
    }
    for (const RoutePoint& point : route.waypoints) {
      DrawPoint(point.x, point.y, main_route ? 2 : 1, level.size, kWhite,
                &pixels);
    }
  }
  return pixels;
}

std::vector<RgbaColor> BuildPlacesImage(const LevelData& level) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(level.size.width) *
                                static_cast<std::size_t>(level.size.height),
                                kBlack);
  for (const Place& place : level.places) {
    const int radius = std::max(1, std::min(place.radius, 8));
    DrawPoint(place.x, place.y, radius, level.size,
              RgbaColor{88, 120, 210, 255}, &pixels);
    DrawPoint(place.x, place.y, 1, level.size, kWhite, &pixels);
  }
  return pixels;
}

std::vector<RgbaColor> BuildObjectFootprintImage(const LevelData& level) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(level.size.width) *
                                static_cast<std::size_t>(level.size.height),
                                kBlack);
  for (const RuntimeObject& object : level.objects) {
    const RgbaColor color = object.blocks_movement ? RgbaColor{210, 80, 52, 255}
                                                   : RgbaColor{190, 130, 62, 255};
    for (int y = object.y; y < object.y + object.height; ++y) {
      for (int x = object.x; x < object.x + object.width; ++x) {
        SetPixel(x, y, level.size, color, &pixels);
      }
    }
    SetPixel(object.x, object.y, level.size, kWhite, &pixels);
  }
  return pixels;
}

RgbaColor RoadBandColor(std::uint8_t value) {
  const RoadVisualBand band = static_cast<RoadVisualBand>(value);
  switch (band) {
    case RoadVisualBand::kRoadCore:
      return RgbaColor{168, 118, 62, 255};
    case RoadVisualBand::kRoadSide:
      return RgbaColor{132, 112, 70, 255};
    case RoadVisualBand::kTrampledGrass:
      return RgbaColor{104, 122, 66, 255};
    case RoadVisualBand::kMudPatch:
      return RgbaColor{92, 70, 50, 255};
    case RoadVisualBand::kRuinApproach:
      return RgbaColor{152, 126, 86, 255};
    case RoadVisualBand::kNone:
      return kBlack;
  }
  return kBlack;
}

std::vector<RgbaColor> BuildRoadBandImage(const RoadVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.road_bands.size(); ++i) {
    pixels[i] = RoadBandColor(plan.road_bands[i]);
  }
  return pixels;
}

std::vector<RgbaColor> BuildRoadDressingInfluenceImage(const RoadVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.route_influence.size(); ++i) {
    switch (plan.route_influence[i]) {
      case 1:
        pixels[i] = RgbaColor{92, 116, 74, 255};
        break;
      case 2:
        pixels[i] = RgbaColor{142, 112, 70, 255};
        break;
      case 3:
        pixels[i] = RgbaColor{226, 172, 72, 255};
        break;
      default:
        break;
    }
  }
  return pixels;
}

RgbaColor RuinVisualColor(std::uint8_t value) {
  const RuinVisualTile tile = static_cast<RuinVisualTile>(value);
  switch (tile) {
    case RuinVisualTile::kCrackedFloor:
      return RgbaColor{106, 102, 84, 255};
    case RuinVisualTile::kOvergrownFloor:
      return RgbaColor{82, 110, 68, 255};
    case RuinVisualTile::kWallIntact:
      return RgbaColor{58, 52, 44, 255};
    case RuinVisualTile::kWallBroken:
      return RgbaColor{82, 72, 58, 255};
    case RuinVisualTile::kWallCorner:
      return RgbaColor{66, 58, 48, 255};
    case RuinVisualTile::kWallEndcap:
      return RgbaColor{92, 80, 62, 255};
    case RuinVisualTile::kRubble:
      return RgbaColor{128, 108, 78, 255};
    case RuinVisualTile::kEntrance:
      return RgbaColor{154, 124, 74, 255};
    case RuinVisualTile::kNone:
      return kBlack;
  }
  return kBlack;
}

std::vector<RgbaColor> BuildRuinCompositionImage(const RuinVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.tiles.size(); ++i) {
    pixels[i] = RuinVisualColor(plan.tiles[i]);
  }
  return pixels;
}

std::vector<RgbaColor> BuildRuinSiteImage(const RuinVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.site_ids.size(); ++i) {
    const std::uint16_t site_id = plan.site_ids[i];
    if (site_id == 0) {
      continue;
    }
    pixels[i] = RgbaColor{
        static_cast<std::uint8_t>(70 + (site_id * 43U) % 150U),
        static_cast<std::uint8_t>(70 + (site_id * 73U) % 130U),
        static_cast<std::uint8_t>(60 + (site_id * 29U) % 100U), 255};
  }
  return pixels;
}

RgbaColor WaterVisualColor(std::uint8_t value) {
  const WaterVisualTile tile = static_cast<WaterVisualTile>(value);
  switch (tile) {
    case WaterVisualTile::kWaterCore:
      return RgbaColor{28, 86, 112, 255};
    case WaterVisualTile::kWaterEdge:
      return RgbaColor{38, 104, 122, 255};
    case WaterVisualTile::kMudRing:
      return RgbaColor{78, 66, 48, 255};
    case WaterVisualTile::kWetGrass:
      return RgbaColor{54, 102, 72, 255};
    case WaterVisualTile::kReedZone:
      return RgbaColor{68, 126, 76, 255};
    case WaterVisualTile::kCrossing:
      return RgbaColor{112, 98, 68, 255};
    case WaterVisualTile::kNone:
      return kBlack;
  }
  return kBlack;
}

std::vector<RgbaColor> BuildWaterVisualImage(const WaterVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.tiles.size(); ++i) {
    pixels[i] = WaterVisualColor(plan.tiles[i]);
  }
  return pixels;
}

std::vector<RgbaColor> BuildWaterRegionImage(const WaterVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.region_ids.size(); ++i) {
    const std::uint16_t region_id = plan.region_ids[i];
    if (region_id == 0) {
      continue;
    }
    pixels[i] = RgbaColor{
        static_cast<std::uint8_t>(26 + (region_id * 29U) % 70U),
        static_cast<std::uint8_t>(82 + (region_id * 47U) % 110U),
        static_cast<std::uint8_t>(104 + (region_id * 61U) % 110U), 255};
  }
  return pixels;
}

RgbaColor ObjectVisualColor(ObjectVisualKind kind) {
  switch (kind) {
    case ObjectVisualKind::kVegetation:
      return RgbaColor{42, 112, 58, 255};
    case ObjectVisualKind::kWood:
      return RgbaColor{132, 92, 48, 255};
    case ObjectVisualKind::kStone:
      return RgbaColor{142, 134, 116, 255};
    case ObjectVisualKind::kScrap:
      return RgbaColor{126, 104, 92, 255};
    case ObjectVisualKind::kCamp:
      return RgbaColor{174, 124, 64, 255};
    case ObjectVisualKind::kCache:
      return RgbaColor{218, 176, 72, 255};
    case ObjectVisualKind::kStructure:
      return RgbaColor{102, 92, 76, 255};
    case ObjectVisualKind::kRuin:
      return RgbaColor{116, 106, 88, 255};
    case ObjectVisualKind::kElevation:
      return RgbaColor{104, 86, 64, 255};
    case ObjectVisualKind::kLandmark:
      return RgbaColor{184, 138, 82, 255};
    case ObjectVisualKind::kCover:
      return RgbaColor{88, 76, 58, 255};
    case ObjectVisualKind::kTypedFallback:
      return RgbaColor{166, 132, 84, 255};
    case ObjectVisualKind::kUnknown:
      return RgbaColor{206, 46, 180, 255};
  }
  return RgbaColor{206, 46, 180, 255};
}

void DrawObjectFootprint(const ObjectVisualItem& item, const LevelSize& size,
                         RgbaColor color, std::vector<RgbaColor>* pixels) {
  if (pixels == nullptr) {
    return;
  }

  for (int y = item.y; y < item.y + item.height; ++y) {
    for (int x = item.x; x < item.x + item.width; ++x) {
      SetPixel(x, y, size, color, pixels);
    }
  }
  SetPixel(item.x, item.y, size, kWhite, pixels);
}

std::vector<RgbaColor> BuildObjectMappingImage(const ObjectVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (const ObjectVisualItem& item : plan.items) {
    DrawObjectFootprint(item, plan.size, ObjectVisualColor(item.kind), &pixels);
  }
  return pixels;
}

std::vector<RgbaColor> BuildObjectFallbackImage(const ObjectVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (const ObjectVisualItem& item : plan.items) {
    if (StartsWith(item.sprite_family, "object.fallback") ||
        StartsWith(item.sprite_family, "structure.fallback")) {
      DrawObjectFootprint(item, plan.size, RgbaColor{238, 156, 64, 255},
                          &pixels);
    }
  }
  return pixels;
}

RgbaColor MicroSceneKindColor(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return RgbaColor{198, 136, 62, 255};
    case MicroSceneKind::kRoadsideDebris:
      return RgbaColor{164, 122, 72, 255};
    case MicroSceneKind::kLoggingSpot:
      return RgbaColor{118, 82, 42, 255};
    case MicroSceneKind::kRuinDebrisCluster:
      return RgbaColor{146, 134, 108, 255};
    case MicroSceneKind::kSwampCrossingDetail:
      return RgbaColor{66, 138, 84, 255};
    case MicroSceneKind::kObjectSceneDressing:
      return RgbaColor{126, 112, 92, 255};
    case MicroSceneKind::kCacheHint:
      return RgbaColor{218, 176, 72, 255};
    case MicroSceneKind::kNone:
      return kBlack;
  }
  return kBlack;
}

RgbaColor MicroSceneTileColor(MicroSceneTile tile) {
  switch (tile) {
    case MicroSceneTile::kGroundDetail:
      return RgbaColor{126, 112, 68, 255};
    case MicroSceneTile::kSmallDebris:
      return RgbaColor{144, 118, 78, 255};
    case MicroSceneTile::kSecondaryProp:
      return RgbaColor{164, 116, 68, 255};
    case MicroSceneTile::kPrimaryProp:
      return RgbaColor{214, 154, 76, 255};
    case MicroSceneTile::kVegetationDetail:
      return RgbaColor{68, 132, 62, 255};
    case MicroSceneTile::kStoneDetail:
      return RgbaColor{150, 140, 118, 255};
    case MicroSceneTile::kWetDetail:
      return RgbaColor{58, 124, 92, 255};
    case MicroSceneTile::kNone:
      return kBlack;
  }
  return kBlack;
}

std::vector<RgbaColor> BuildMicroSceneTileImage(
    const MicroSceneVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.tiles.size(); ++i) {
    pixels[i] = MicroSceneTileColor(static_cast<MicroSceneTile>(plan.tiles[i]));
  }
  return pixels;
}

std::vector<RgbaColor> BuildMicroSceneKindImage(
    const MicroSceneVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (const MicroSceneItem& scene : plan.scenes) {
    DrawPoint(scene.x, scene.y, scene.radius, plan.size,
              MicroSceneKindColor(scene.kind), &pixels);
    SetPixel(scene.x, scene.y, plan.size, kWhite, &pixels);
  }
  return pixels;
}

RgbaColor ForestDepthColor(std::uint8_t value) {
  const ForestDepthBand band = static_cast<ForestDepthBand>(value);
  switch (band) {
    case ForestDepthBand::kEdge:
      return RgbaColor{44, 96, 52, 255};
    case ForestDepthBand::kMid:
      return RgbaColor{24, 73, 42, 255};
    case ForestDepthBand::kDeep:
      return RgbaColor{9, 42, 28, 255};
    case ForestDepthBand::kNone:
      return kBlack;
  }
  return kBlack;
}

RgbaColor ClearingRoleColor(std::uint8_t value) {
  const ClearingRole role = static_cast<ClearingRole>(value);
  switch (role) {
    case ClearingRole::kMainClearing:
      return RgbaColor{150, 152, 76, 255};
    case ClearingRole::kSideClearing:
      return RgbaColor{92, 132, 66, 255};
    case ClearingRole::kConnectorCorridor:
      return RgbaColor{156, 122, 72, 255};
    case ClearingRole::kMicroClearing:
      return RgbaColor{112, 152, 84, 255};
    case ClearingRole::kSceneSpace:
      return RgbaColor{146, 126, 86, 255};
    case ClearingRole::kNone:
      return kBlack;
  }
  return kBlack;
}

RgbaColor ClearingSceneRoleColor(std::uint8_t value) {
  const ClearingSceneRole role = static_cast<ClearingSceneRole>(value);
  switch (role) {
    case ClearingSceneRole::kRuinsScene:
      return RgbaColor{156, 104, 88, 255};
    case ClearingSceneRole::kRoadApproach:
      return RgbaColor{196, 146, 74, 255};
    case ClearingSceneRole::kObjectScene:
      return RgbaColor{128, 104, 166, 255};
    case ClearingSceneRole::kGenericScene:
      return RgbaColor{96, 122, 154, 255};
    case ClearingSceneRole::kNone:
      return kBlack;
  }
  return kBlack;
}

std::vector<RgbaColor> BuildForestDepthImage(const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.forest_depth.size(); ++i) {
    pixels[i] = ForestDepthColor(plan.forest_depth[i]);
  }
  return pixels;
}

std::vector<RgbaColor> BuildForestEdgeImage(const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.forest_edges.size(); ++i) {
    if (plan.forest_edges[i] != 0) {
      pixels[i] = RgbaColor{226, 178, 76, 255};
    }
  }
  return pixels;
}

std::vector<RgbaColor> BuildForestMassGroupImage(
    const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.forest_mass_groups.size(); ++i) {
    const std::uint16_t group = plan.forest_mass_groups[i];
    if (group == 0) {
      continue;
    }
    pixels[i] = RgbaColor{
        static_cast<std::uint8_t>(24 + (group * 37U) % 80U),
        static_cast<std::uint8_t>(78 + (group * 53U) % 120U),
        static_cast<std::uint8_t>(42 + (group * 19U) % 70U), 255};
  }
  return pixels;
}

std::vector<RgbaColor> BuildCanopyCandidateImage(
    const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.canopy_candidates.size(); ++i) {
    if (plan.canopy_candidates[i] != 0) {
      pixels[i] = RgbaColor{18, 120, 58, 255};
    }
  }
  return pixels;
}

std::vector<RgbaColor> BuildForestMassImage(const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels = BuildForestDepthImage(plan);
  for (std::size_t i = 0; i < plan.route_influence.size(); ++i) {
    if (plan.route_influence[i] >= 3 && pixels[i].a != 0) {
      pixels[i] = RgbaColor{34, 72, 38, 255};
    }
  }
  return pixels;
}

std::vector<RgbaColor> BuildClearingRoleImage(const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.clearing_roles.size(); ++i) {
    pixels[i] = ClearingRoleColor(plan.clearing_roles[i]);
  }
  return pixels;
}

std::vector<RgbaColor> BuildClearingSceneRoleImage(
    const ForestVisualPlan& plan) {
  std::vector<RgbaColor> pixels(static_cast<std::size_t>(plan.size.width) *
                                static_cast<std::size_t>(plan.size.height),
                                kBlack);
  for (std::size_t i = 0; i < plan.clearing_scene_roles.size(); ++i) {
    pixels[i] = ClearingSceneRoleColor(plan.clearing_scene_roles[i]);
  }
  return pixels;
}

std::map<std::string, int> CountObjectsByType(const LevelData& level) {
  std::map<std::string, int> counts;
  for (const RuntimeObject& object : level.objects) {
    ++counts[object.type];
  }
  return counts;
}

std::map<std::string, int> CountPlacesByType(const LevelData& level) {
  std::map<std::string, int> counts;
  for (const Place& place : level.places) {
    ++counts[place.type];
  }
  return counts;
}

std::map<std::string, int> CountRoutesByType(const LevelData& level) {
  std::map<std::string, int> counts;
  for (const Route& route : level.routes) {
    ++counts[route.type];
  }
  return counts;
}

std::map<std::string, int> CountZonesByType(const LevelData& level) {
  std::map<std::string, int> counts;
  for (const GameplayZone& zone : level.zones) {
    ++counts[zone.type];
  }
  return counts;
}

void AppendJsonIntMap(std::ostringstream* json,
                      const std::map<std::string, int>& counts,
                      int indent_spaces) {
  std::string indent(static_cast<std::size_t>(indent_spaces), ' ');
  std::size_t emitted = 0;
  for (const auto& [name, count] : counts) {
    *json << indent << JsonString(name) << ": " << count;
    ++emitted;
    *json << (emitted < counts.size() ? "," : "") << "\n";
  }
}

std::string PercentString(int count, int total) {
  const double percent = total > 0 ? static_cast<double>(count) * 100.0 /
                                        static_cast<double>(total)
                                  : 0.0;
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << percent;
  return stream.str();
}

void AppendJsonCountField(std::ostringstream* json, std::string_view name,
                          int count, int total, bool comma) {
  *json << "    \"" << name << "\": {\"count\": " << count
        << ", \"percent\": " << PercentString(count, total) << "}";
  if (comma) {
    *json << ",";
  }
  *json << "\n";
}

bool IsObjectInside(const RuntimeObject& object, const LevelSize& size) {
  return object.x >= 0 && object.y >= 0 && object.width > 0 &&
         object.height > 0 && object.x + object.width <= size.width &&
         object.y + object.height <= size.height;
}

bool IsMarkerInside(const Marker& marker, const LevelSize& size) {
  return marker.x >= 0 && marker.y >= 0 && marker.x < size.width &&
         marker.y < size.height;
}

bool IsPlaceInside(const Place& place, const LevelSize& size) {
  return place.x >= 0 && place.y >= 0 && place.x < size.width &&
         place.y < size.height;
}

std::map<std::string, int> CountRegionsByType(const TerrainRegions& regions) {
  std::map<std::string, int> counts;
  for (const TerrainRegion& region : regions.regions) {
    ++counts[std::string(TerrainTypeToString(region.type))];
  }
  return counts;
}

}  // namespace

DebugArtifactWriter::DebugArtifactWriter(std::filesystem::path output_root)
    : output_root_(std::move(output_root)) {}

bool DebugArtifactWriter::WriteInputValidationReport(
    const LevelData& level, std::string* error) const {
  const int expected_cells = level.size.width * level.size.height;
  int out_of_bounds_objects = 0;
  for (const RuntimeObject& object : level.objects) {
    if (!IsObjectInside(object, level.size)) {
      ++out_of_bounds_objects;
    }
  }

  int out_of_bounds_markers = 0;
  for (const Marker& marker : level.markers) {
    if (!IsMarkerInside(marker, level.size)) {
      ++out_of_bounds_markers;
    }
  }

  int out_of_bounds_places = 0;
  for (const Place& place : level.places) {
    if (!IsPlaceInside(place, level.size)) {
      ++out_of_bounds_places;
    }
  }

  const bool size_valid = level.size.width > 0 && level.size.height > 0 &&
                          level.size.tile_size > 0;
  const bool cells_valid = expected_cells > 0 &&
                           level.cells.size() ==
                               static_cast<std::size_t>(expected_cells);
  const bool ok = size_valid && cells_valid && out_of_bounds_objects == 0 &&
                  out_of_bounds_markers == 0 && out_of_bounds_places == 0;

  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-input-v1\",\n";
  json << "  \"status\": \"" << (ok ? "ok" : "passed_with_warnings")
       << "\",\n";
  json << "  \"map\": {\n";
  json << "    \"width\": " << level.size.width << ",\n";
  json << "    \"height\": " << level.size.height << ",\n";
  json << "    \"tile_size\": " << level.size.tile_size << ",\n";
  json << "    \"expected_cells\": " << expected_cells << ",\n";
  json << "    \"actual_cells\": " << level.cells.size() << "\n";
  json << "  },\n";
  json << "  \"counts\": {\n";
  json << "    \"objects\": " << level.objects.size() << ",\n";
  json << "    \"places\": " << level.places.size() << ",\n";
  json << "    \"markers\": " << level.markers.size() << ",\n";
  json << "    \"routes\": " << level.routes.size() << ",\n";
  json << "    \"zones\": " << level.zones.size() << ",\n";
  json << "    \"graph_nodes\": " << level.world_graph.nodes.size() << ",\n";
  json << "    \"graph_edges\": " << level.world_graph.edges.size()
       << "\n";
  json << "  },\n";
  json << "  \"checks\": {\n";
  json << "    \"size_valid\": " << (size_valid ? "true" : "false")
       << ",\n";
  json << "    \"cell_count_valid\": " << (cells_valid ? "true" : "false")
       << ",\n";
  json << "    \"out_of_bounds_objects\": " << out_of_bounds_objects
       << ",\n";
  json << "    \"out_of_bounds_markers\": " << out_of_bounds_markers
       << ",\n";
  json << "    \"out_of_bounds_places\": " << out_of_bounds_places
       << "\n";
  json << "  }\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" /
                           "00_input_validation.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteSemanticMaskArtifacts(
    const SemanticMasks& masks, std::string* error) const {
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "semantic_masks";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"forest.png", BuildBinaryMaskImage(masks.forest, masks.size, kForest)},
      {"open_ground.png",
       BuildBinaryMaskImage(masks.open_ground, masks.size, kOpenGround)},
      {"road.png", BuildBinaryMaskImage(masks.road, masks.size, kRoad)},
      {"ruins.png", BuildBinaryMaskImage(masks.ruins, masks.size, kRuins)},
      {"swamp.png", BuildBinaryMaskImage(masks.swamp, masks.size, kSwamp)},
      {"water.png", BuildBinaryMaskImage(masks.water, masks.size, kWater)},
      {"collision.png",
       BuildBinaryMaskImage(masks.blocked, masks.size, kWhite)},
      {"walkable.png", BuildBinaryMaskImage(masks.walkable, masks.size,
                                             RgbaColor{115, 170, 90, 255})},
      {"composite.png", BuildCompositeMaskImage(masks)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, masks.size.width,
                      masks.size.height, pixels, error)) {
      return false;
    }
  }

  const SemanticMaskSummary& summary = masks.summary;
  const int total = summary.total_tiles;
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-semantic-masks-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << masks.size.width << ",\n";
  json << "  \"height\": " << masks.size.height << ",\n";
  json << "  \"tile_size\": " << masks.size.tile_size << ",\n";
  json << "  \"total_tiles\": " << total << ",\n";
  json << "  \"masks\": {\n";
  AppendJsonCountField(&json, "forest", summary.forest_tiles, total, true);
  AppendJsonCountField(&json, "open_ground", summary.open_ground_tiles, total,
                       true);
  AppendJsonCountField(&json, "road", summary.road_tiles, total, true);
  AppendJsonCountField(&json, "swamp", summary.swamp_tiles, total, true);
  AppendJsonCountField(&json, "water", summary.water_tiles, total, true);
  AppendJsonCountField(&json, "ruins", summary.ruins_tiles, total, true);
  AppendJsonCountField(&json, "wall", summary.wall_tiles, total, true);
  AppendJsonCountField(&json, "unknown", summary.unknown_tiles, total, true);
  AppendJsonCountField(&json, "walkable", summary.walkable_tiles, total,
                       true);
  AppendJsonCountField(&json, "blocked", summary.blocked_tiles, total, true);
  AppendJsonCountField(&json, "vision_blocked",
                       summary.vision_blocked_tiles, total, true);
  AppendJsonCountField(&json, "projectile_blocked",
                       summary.projectile_blocked_tiles, total, true);
  AppendJsonCountField(&json, "cover", summary.cover_tiles, total, true);
  AppendJsonCountField(&json, "concealment", summary.concealment_tiles, total,
                       false);
  json << "  },\n";
  json << "  \"artifacts\": [\n";
  for (std::size_t i = 0; i < images.size(); ++i) {
    json << "    " << JsonString("semantic_masks/" + images[i].first);
    json << (i + 1 < images.size() ? "," : "") << "\n";
  }
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" /
                           "01_semantic_masks.json",
                       json.str(), error);
}


bool DebugArtifactWriter::WriteSemanticLinkArtifacts(
    const LevelData& level, std::string* error) const {
  const std::filesystem::path directory = output_root_ / "semantic_masks";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"routes.png", BuildRouteImage(level, false)},
      {"route_influence.png", BuildRouteImage(level, true)},
      {"places.png", BuildPlacesImage(level)},
      {"object_footprints.png", BuildObjectFootprintImage(level)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, level.size.width,
                      level.size.height, pixels, error)) {
      return false;
    }
  }

  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-semantic-links-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"counts\": {\n";
  json << "    \"objects\": " << level.objects.size() << ",\n";
  json << "    \"places\": " << level.places.size() << ",\n";
  json << "    \"routes\": " << level.routes.size() << ",\n";
  json << "    \"markers\": " << level.markers.size() << ",\n";
  json << "    \"routes\": " << level.routes.size() << ",\n";
  json << "    \"zones\": " << level.zones.size() << ",\n";
  json << "    \"graph_nodes\": " << level.world_graph.nodes.size() << ",\n";
  json << "    \"graph_edges\": " << level.world_graph.edges.size() << "\n";
  json << "  },\n";

  json << "  \"object_types\": {\n";
  AppendJsonIntMap(&json, CountObjectsByType(level), 4);
  json << "  },\n";

  json << "  \"place_types\": {\n";
  AppendJsonIntMap(&json, CountPlacesByType(level), 4);
  json << "  },\n";

  json << "  \"route_types\": {\n";
  AppendJsonIntMap(&json, CountRoutesByType(level), 4);
  json << "  },\n";

  json << "  \"zone_types\": {\n";
  AppendJsonIntMap(&json, CountZonesByType(level), 4);
  json << "  },\n";

  json << "  \"artifacts\": [\n";
  for (std::size_t i = 0; i < images.size(); ++i) {
    json << "    " << JsonString("semantic_masks/" + images[i].first);
    json << (i + 1 < images.size() ? "," : "") << "\n";
  }
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "03_semantic_links.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteRuinVisualArtifacts(
    const RuinVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "ruin visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"06_ruin_regions.png", BuildRuinSiteImage(plan)},
      {"06_ruin_compositions.png", BuildRuinCompositionImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const RuinVisualSummary& summary = plan.summary;
  const int total = std::max(summary.visual_tiles, 1);
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-ruins-pass-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << plan.size.width << ",\n";
  json << "  \"height\": " << plan.size.height << ",\n";
  json << "  \"sites\": " << summary.site_count << ",\n";
  json << "  \"source\": {\n";
  json << "    \"ruin_floor_tiles\": " << summary.source_ruin_tiles
       << ",\n";
  json << "    \"wall_tiles\": " << summary.source_wall_tiles << "\n";
  json << "  },\n";
  json << "  \"visual_tiles\": {\n";
  AppendJsonCountField(&json, "cracked_floor",
                       summary.cracked_floor_tiles, total, true);
  AppendJsonCountField(&json, "overgrown_floor",
                       summary.overgrown_floor_tiles, total, true);
  AppendJsonCountField(&json, "wall_intact",
                       summary.wall_intact_tiles, total, true);
  AppendJsonCountField(&json, "wall_broken",
                       summary.wall_broken_tiles, total, true);
  AppendJsonCountField(&json, "wall_corner",
                       summary.wall_corner_tiles, total, true);
  AppendJsonCountField(&json, "wall_endcap",
                       summary.wall_endcap_tiles, total, true);
  AppendJsonCountField(&json, "rubble", summary.rubble_tiles, total, true);
  AppendJsonCountField(&json, "entrance", summary.entrance_tiles, total,
                       false);
  json << "  },\n";
  json << "  \"artifacts\": [\n";
  json << "    \"passes/06_ruin_regions.png\",\n";
  json << "    \"passes/06_ruin_compositions.png\"\n";
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "06_ruins_pass.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteWaterVisualArtifacts(
    const WaterVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "water visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"07_water_regions.png", BuildWaterRegionImage(plan)},
      {"07_water_visual.png", BuildWaterVisualImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const WaterVisualSummary& summary = plan.summary;
  const int total = std::max(summary.visual_tiles, 1);
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-water-pass-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << plan.size.width << ",\n";
  json << "  \"height\": " << plan.size.height << ",\n";
  json << "  \"source\": {\n";
  json << "    \"water_tiles\": " << summary.source_water_tiles
       << ",\n";
  json << "    \"swamp_tiles\": " << summary.source_swamp_tiles
       << ",\n";
  json << "    \"water_like_tiles\": " << summary.water_like_tiles
       << ",\n";
  json << "    \"regions\": " << summary.water_region_count << "\n";
  json << "  },\n";
  json << "  \"visual_tiles\": {\n";
  AppendJsonCountField(&json, "water_core", summary.water_core_tiles,
                       total, true);
  AppendJsonCountField(&json, "water_edge", summary.water_edge_tiles,
                       total, true);
  AppendJsonCountField(&json, "mud_ring", summary.mud_ring_tiles,
                       total, true);
  AppendJsonCountField(&json, "wet_grass", summary.wet_grass_tiles,
                       total, true);
  AppendJsonCountField(&json, "reed_zone", summary.reed_zone_tiles,
                       total, true);
  AppendJsonCountField(&json, "crossing", summary.crossing_tiles,
                       total, false);
  json << "  },\n";
  json << "  \"artifacts\": [\n";
  json << "    \"passes/07_water_regions.png\",\n";
  json << "    \"passes/07_water_visual.png\"\n";
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "07_water_pass.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteObjectVisualArtifacts(
    const ObjectVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "object visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"08_object_mapping.png", BuildObjectMappingImage(plan)},
      {"08_object_fallbacks.png", BuildObjectFallbackImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const ObjectVisualSummary& summary = plan.summary;
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-object-mapping-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << plan.size.width << ",\n";
  json << "  \"height\": " << plan.size.height << ",\n";
  json << "  \"summary\": {\n";
  json << "    \"source_object_count\": "
       << summary.source_object_count << ",\n";
  json << "    \"mapped_object_count\": "
       << summary.mapped_object_count << ",\n";
  json << "    \"typed_fallback_count\": "
       << summary.typed_fallback_count << ",\n";
  json << "    \"object_generic\": " << summary.generic_object_count
       << ",\n";
  json << "    \"missing_sprite_uses\": "
       << summary.missing_sprite_uses << ",\n";
  json << "    \"skipped_sprites\": " << summary.skipped_sprites << "\n";
  json << "  },\n";

  json << "  \"visual_kind_counts\": {\n";
  AppendJsonIntMap(&json, summary.visual_kind_counts, 4);
  json << "  },\n";

  json << "  \"sprite_family_counts\": {\n";
  AppendJsonIntMap(&json, summary.sprite_family_counts, 4);
  json << "  },\n";

  json << "  \"source_type_counts\": {\n";
  AppendJsonIntMap(&json, summary.source_type_counts, 4);
  json << "  },\n";

  json << "  \"artifacts\": [\n";
  json << "    \"passes/08_object_mapping.png\",\n";
  json << "    \"passes/08_object_fallbacks.png\"\n";
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "08_object_mapping.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteMicroSceneVisualArtifacts(
    const MicroSceneVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "micro-scene visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"09_micro_scenes.png", BuildMicroSceneKindImage(plan)},
      {"09_micro_scene_dressing.png", BuildMicroSceneTileImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const MicroSceneSummary& summary = plan.summary;
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-micro-scenes-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << plan.size.width << ",\n";
  json << "  \"height\": " << plan.size.height << ",\n";
  json << "  \"summary\": {\n";
  json << "    \"scene_count\": " << summary.scene_count << ",\n";
  json << "    \"visual_tiles\": " << summary.visual_tiles << ",\n";
  json << "    \"camp_scene\": " << summary.camp_scene_count << ",\n";
  json << "    \"roadside_debris\": "
       << summary.roadside_debris_count << ",\n";
  json << "    \"logging_spot\": " << summary.logging_spot_count << ",\n";
  json << "    \"ruin_debris_cluster\": "
       << summary.ruin_debris_cluster_count << ",\n";
  json << "    \"swamp_crossing_detail\": "
       << summary.swamp_crossing_detail_count << ",\n";
  json << "    \"object_scene_dressing\": "
       << summary.object_scene_dressing_count << ",\n";
  json << "    \"cache_hint\": " << summary.cache_hint_count << "\n";
  json << "  },\n";

  json << "  \"tile_roles\": {\n";
  json << "    \"primary_prop\": " << summary.primary_prop_tiles << ",\n";
  json << "    \"secondary_prop\": " << summary.secondary_prop_tiles
       << ",\n";
  json << "    \"small_debris\": " << summary.small_debris_tiles << ",\n";
  json << "    \"ground_detail\": " << summary.ground_detail_tiles << ",\n";
  json << "    \"vegetation_detail\": "
       << summary.vegetation_detail_tiles << ",\n";
  json << "    \"stone_detail\": " << summary.stone_detail_tiles << ",\n";
  json << "    \"wet_detail\": " << summary.wet_detail_tiles << "\n";
  json << "  },\n";

  json << "  \"theme_counts\": {\n";
  AppendJsonIntMap(&json, summary.theme_counts, 4);
  json << "  },\n";

  json << "  \"artifacts\": [\n";
  json << "    \"passes/09_micro_scenes.png\",\n";
  json << "    \"passes/09_micro_scene_dressing.png\"\n";
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "09_micro_scenes.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteRoadVisualArtifacts(
    const RoadVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "road visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"05_road_dressing_influence.png", BuildRoadDressingInfluenceImage(plan)},
      {"05_road_visual.png", BuildRoadBandImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const RoadVisualSummary& summary = plan.summary;
  const int total = summary.road_core_tiles + summary.road_dressing_tiles;
  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-road-pass-v2\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << plan.size.width << ",\n";
  json << "  \"height\": " << plan.size.height << ",\n";
  json << "  \"source\": \"terrain_road\",\n";
  json << "  \"routes_used_for_visual_roads\": "
       << (summary.routes_used_for_visual_roads ? "true" : "false")
       << ",\n";
  json << "  \"routes\": {\n";
  json << "    \"total\": " << summary.route_count << ",\n";
  json << "    \"main\": " << summary.main_route_count << ",\n";
  json << "    \"side\": " << summary.side_route_count << ",\n";
  json << "    \"hidden\": " << summary.hidden_route_count << "\n";
  json << "  },\n";
  json << "  \"terrain_road_tiles\": " << summary.terrain_road_tiles
       << ",\n";
  json << "  \"road_dressing_tiles\": " << summary.road_dressing_tiles
       << ",\n";
  json << "  \"road_bands\": {\n";
  AppendJsonCountField(&json, "road_core", summary.road_core_tiles, total,
                       true);
  AppendJsonCountField(&json, "road_side", summary.road_side_tiles, total,
                       true);
  AppendJsonCountField(&json, "trampled_grass",
                       summary.trampled_grass_tiles, total, true);
  AppendJsonCountField(&json, "mud_patch", summary.mud_patch_tiles, total,
                       true);
  AppendJsonCountField(&json, "ruin_approach",
                       summary.ruin_approach_tiles, total, false);
  json << "  },\n";
  json << "  \"road_influenced_tiles\": "
       << summary.route_influenced_tiles << ",\n";
  json << "  \"artifacts\": [\n";
  json << "    \"passes/05_road_dressing_influence.png\",\n";
  json << "    \"passes/05_road_visual.png\"\n";
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "05_road_pass.json",
                       json.str(), error);
}

bool DebugArtifactWriter::WriteForestVisualArtifacts(
    const ForestVisualPlan& plan, std::string* error) const {
  if (!plan.IsValid()) {
    if (error != nullptr) {
      *error = "forest visual plan is invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "passes";
  const std::vector<std::pair<std::string, std::vector<RgbaColor>>> images = {
      {"03_forest_depth.png", BuildForestDepthImage(plan)},
      {"03_forest_edges.png", BuildForestEdgeImage(plan)},
      {"03_forest_mass.png", BuildForestMassImage(plan)},
      {"03_forest_mass_groups.png", BuildForestMassGroupImage(plan)},
      {"03_canopy_candidates.png", BuildCanopyCandidateImage(plan)},
      {"04_clearing_roles.png", BuildClearingRoleImage(plan)},
      {"04_clearing_roles_detailed.png",
       BuildClearingSceneRoleImage(plan)},
  };

  for (const auto& [filename, pixels] : images) {
    if (!WritePngRgba(directory / filename, plan.size.width,
                      plan.size.height, pixels, error)) {
      return false;
    }
  }

  const ForestVisualSummary& summary = plan.summary;
  const int total_forest = summary.forest_tiles;
  std::ostringstream forest_json;
  forest_json << "{\n";
  forest_json << "  \"schema_version\": \"visual-debug-forest-normalization-v1\",\n";
  forest_json << "  \"status\": \"ok\",\n";
  forest_json << "  \"width\": " << plan.size.width << ",\n";
  forest_json << "  \"height\": " << plan.size.height << ",\n";
  forest_json << "  \"forest_depth\": {\n";
  AppendJsonCountField(&forest_json, "edge", summary.forest_edge_tiles,
                       total_forest, true);
  AppendJsonCountField(&forest_json, "mid", summary.forest_mid_tiles,
                       total_forest, true);
  AppendJsonCountField(&forest_json, "deep", summary.forest_deep_tiles,
                       total_forest, false);
  forest_json << "  },\n";
  forest_json << "  \"route_influenced_tiles\": "
              << summary.route_influenced_tiles << ",\n";
  forest_json << "  \"suppressed_tiny_forest_tiles\": "
              << summary.suppressed_tiny_forest_tiles << ",\n";
  forest_json << "  \"canopy_candidate_tiles\": "
              << summary.canopy_candidate_tiles << ",\n";
  forest_json << "  \"forest_mass_group_count\": "
              << summary.forest_mass_group_count << ",\n";
  forest_json << "  \"artifacts\": [\n";
  forest_json << "    \"passes/03_forest_depth.png\",\n";
  forest_json << "    \"passes/03_forest_edges.png\",\n";
  forest_json << "    \"passes/03_forest_mass.png\",\n";
  forest_json << "    \"passes/03_forest_mass_groups.png\",\n";
  forest_json << "    \"passes/03_canopy_candidates.png\"\n";
  forest_json << "  ]\n";
  forest_json << "}\n";
  if (!WriteTextFile(output_root_ / "reports" /
                         "03_forest_normalization.json",
                     forest_json.str(), error)) {
    return false;
  }

  const int total_clearings = summary.main_clearing_tiles +
                              summary.side_clearing_tiles +
                              summary.connector_corridor_tiles +
                              summary.micro_clearing_tiles +
                              summary.scene_space_tiles;
  std::ostringstream clearing_json;
  clearing_json << "{\n";
  clearing_json << "  \"schema_version\": \"visual-debug-clearing-normalization-v1\",\n";
  clearing_json << "  \"status\": \"ok\",\n";
  clearing_json << "  \"width\": " << plan.size.width << ",\n";
  clearing_json << "  \"height\": " << plan.size.height << ",\n";
  clearing_json << "  \"clearing_roles\": {\n";
  AppendJsonCountField(&clearing_json, "main_clearing",
                       summary.main_clearing_tiles, total_clearings, true);
  AppendJsonCountField(&clearing_json, "side_clearing",
                       summary.side_clearing_tiles, total_clearings, true);
  AppendJsonCountField(&clearing_json, "connector_corridor",
                       summary.connector_corridor_tiles, total_clearings,
                       true);
  AppendJsonCountField(&clearing_json, "micro_clearing",
                       summary.micro_clearing_tiles, total_clearings, true);
  AppendJsonCountField(&clearing_json, "scene_space",
                       summary.scene_space_tiles, total_clearings, false);
  clearing_json << "  },\n";
  clearing_json << "  \"artifacts\": [\n";
  clearing_json << "    \"passes/04_clearing_roles.png\",\n";
  clearing_json << "    \"passes/04_clearing_roles_detailed.png\"\n";
  clearing_json << "  ]\n";
  clearing_json << "}\n";

  if (!WriteTextFile(output_root_ / "reports" /
                         "04_clearing_normalization.json",
                     clearing_json.str(), error)) {
    return false;
  }

  std::ostringstream detailed_json;
  detailed_json << "{\n";
  detailed_json << "  \"schema_version\": "
                << "\"visual-debug-clearing-roles-detailed-v1\",\n";
  detailed_json << "  \"status\": \"ok\",\n";
  detailed_json << "  \"width\": " << plan.size.width << ",\n";
  detailed_json << "  \"height\": " << plan.size.height << ",\n";
  detailed_json << "  \"scene_roles\": {\n";
  AppendJsonCountField(&detailed_json, "ruins_scene",
                       summary.ruins_scene_tiles, total_clearings, true);
  AppendJsonCountField(&detailed_json, "road_approach",
                       summary.road_approach_scene_tiles, total_clearings,
                       true);
  AppendJsonCountField(&detailed_json, "object_scene",
                       summary.object_scene_tiles, total_clearings, true);
  AppendJsonCountField(&detailed_json, "generic_scene",
                       summary.generic_scene_tiles, total_clearings, false);
  detailed_json << "  },\n";
  detailed_json << "  \"artifacts\": [\n";
  detailed_json << "    \"passes/04_clearing_roles_detailed.png\"\n";
  detailed_json << "  ]\n";
  detailed_json << "}\n";

  return WriteTextFile(output_root_ / "reports" /
                           "04_clearing_roles_detailed.json",
                       detailed_json.str(), error);
}

bool DebugArtifactWriter::WriteTerrainRegionArtifacts(
    const TerrainRegions& regions, std::string* error) const {
  if (!regions.IsValid()) {
    if (error != nullptr) {
      *error = "terrain regions are invalid for debug artifact writing";
    }
    return false;
  }

  const std::filesystem::path directory = output_root_ / "regions";
  const std::vector<std::pair<std::string, TerrainType>> image_specs = {
      {"forest_regions.png", TerrainType::kForest},
      {"open_regions.png", TerrainType::kOpenGround},
      {"road_components.png", TerrainType::kRoad},
      {"ruin_regions.png", TerrainType::kRuins},
      {"swamp_regions.png", TerrainType::kSwamp},
      {"water_regions.png", TerrainType::kWater},
  };

  for (const auto& [filename, type] : image_specs) {
    if (!WritePngRgba(directory / filename, regions.size.width,
                      regions.size.height, BuildRegionImage(regions, type),
                      error)) {
      return false;
    }
  }

  std::ostringstream json;
  json << "{\n";
  json << "  \"schema_version\": \"visual-debug-regions-v1\",\n";
  json << "  \"status\": \"ok\",\n";
  json << "  \"width\": " << regions.size.width << ",\n";
  json << "  \"height\": " << regions.size.height << ",\n";
  json << "  \"summary\": {\n";
  json << "    \"total_regions\": " << regions.summary.total_regions
       << ",\n";
  json << "    \"tiny_regions\": " << regions.summary.tiny_regions
       << ",\n";
  json << "    \"largest_region_area\": "
       << regions.summary.largest_region_area << ",\n";
  json << "    \"largest_forest_area\": "
       << regions.summary.largest_forest_area << ",\n";
  json << "    \"largest_open_ground_area\": "
       << regions.summary.largest_open_ground_area << "\n";
  json << "  },\n";
  json << "  \"region_counts\": {\n";
  const std::map<std::string, int> counts = CountRegionsByType(regions);
  std::size_t emitted = 0;
  for (const auto& [name, count] : counts) {
    json << "    " << JsonString(name) << ": " << count;
    ++emitted;
    json << (emitted < counts.size() ? "," : "") << "\n";
  }
  json << "  },\n";
  json << "  \"regions\": [\n";
  for (std::size_t i = 0; i < regions.regions.size(); ++i) {
    const TerrainRegion& region = regions.regions[i];
    json << "    {\"id\": " << region.id << ", \"type\": "
         << JsonString(TerrainTypeToString(region.type))
         << ", \"area\": " << region.area << ", \"bounds\": {"
         << "\"min_x\": " << region.min_x << ", \"min_y\": "
         << region.min_y << ", \"max_x\": " << region.max_x
         << ", \"max_y\": " << region.max_y << "}, "
         << "\"inner_tiles\": " << region.inner_tile_count
         << ", \"border_tiles\": " << region.border_tile_count << "}";
    json << (i + 1 < regions.regions.size() ? "," : "") << "\n";
  }
  json << "  ],\n";
  json << "  \"artifacts\": [\n";
  for (std::size_t i = 0; i < image_specs.size(); ++i) {
    json << "    " << JsonString("regions/" + image_specs[i].first);
    json << (i + 1 < image_specs.size() ? "," : "") << "\n";
  }
  json << "  ]\n";
  json << "}\n";

  return WriteTextFile(output_root_ / "reports" / "02_regions.json",
                       json.str(), error);
}

}  // namespace sar::visual_pipeline
