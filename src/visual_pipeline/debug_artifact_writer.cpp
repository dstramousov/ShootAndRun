#include "visual_pipeline/debug_artifact_writer.h"

#include <algorithm>
#include <array>
#include <cstdint>
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
