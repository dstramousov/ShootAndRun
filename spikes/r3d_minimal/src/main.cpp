/**
 * @file spikes/r3d_minimal/src/main.cpp
 * @brief Standalone R3D forest/map-slice spike for the ShootAndRun renderer.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <raylib.h>
#include <raymath.h>
#include <r3d/r3d.h>

#include "level/level_loader.h"
#include "level/terrain_type.h"
#include "render3d/model_registry.h"

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kDefaultSliceSizeTiles = 48;
constexpr float kPi = 3.14159265358979323846f;
constexpr std::uint64_t kSpikeSeed = 0xD15EA5E5C0FFEEULL;

enum class ForestRenderMode {
  kRaw,
  kComposition,
};

enum class ForestTileBand : std::uint8_t {
  kNone,
  kEdge,
  kMid,
  kDeep,
};

const char* ForestRenderModeName(ForestRenderMode mode) {
  switch (mode) {
    case ForestRenderMode::kRaw:
      return "raw";
    case ForestRenderMode::kComposition:
      return "composition";
  }
  return "composition";
}

const char* ForestTileBandName(ForestTileBand band) {
  switch (band) {
    case ForestTileBand::kNone:
      return "none";
    case ForestTileBand::kEdge:
      return "edge";
    case ForestTileBand::kMid:
      return "mid";
    case ForestTileBand::kDeep:
      return "deep";
  }
  return "none";
}

ForestRenderMode ParseForestRenderMode(std::string_view value) {
  if (value == "raw") {
    return ForestRenderMode::kRaw;
  }
  if (value == "composition") {
    return ForestRenderMode::kComposition;
  }
  throw std::runtime_error(
      "unsupported --forest-mode value; expected raw or composition");
}

struct SpikeConfig {
  std::filesystem::path project_root;
  std::filesystem::path app_config_path;
  std::filesystem::path map_package_path;
  std::filesystem::path asset_library_path;
  std::filesystem::path tileset_path;
  int slice_size_tiles = kDefaultSliceSizeTiles;
  ForestRenderMode forest_render_mode = ForestRenderMode::kComposition;
};

struct PrimitiveMeshes {
  R3D_Mesh tile;
  R3D_Mesh cube;
  R3D_Mesh trunk;
  R3D_Mesh canopy;
};

struct SceneMaterials {
  R3D_Material open_ground;
  R3D_Material forest_floor;
  R3D_Material forest_edge_floor;
  R3D_Material forest_mid_floor;
  R3D_Material forest_deep_floor;
  R3D_Material forest_canopy_mass;
  R3D_Material road;
  R3D_Material swamp;
  R3D_Material water;
  R3D_Material ruins;
  R3D_Material wall;
  R3D_Material unknown;
  R3D_Material debug_stone;
  R3D_Material fallback_trunk;
  R3D_Material fallback_canopy;
};

struct LoadedModel {
  R3D_Model model{};
  bool valid = false;
};

struct ModelCache {
  std::map<std::string, LoadedModel> models;
};

struct ModelPlacement {
  std::string semantic_key;
  std::string model_id;
  Vector3 position{};
  Quaternion rotation{};
  Vector3 scale{};
  bool fallback = false;
};

struct MapSlice {
  int x0 = 0;
  int y0 = 0;
  int width = 0;
  int height = 0;
  int center_tile_x = 0;
  int center_tile_y = 0;
  int forest_tiles = 0;
  int forest_edge_tiles = 0;
  int forest_mid_tiles = 0;
  int forest_deep_tiles = 0;
  int skipped_tree_tiles = 0;
  int tree_model_instances = 0;
  int underbrush_model_instances = 0;
  int detail_model_instances = 0;
  int fallback_tree_instances = 0;
};

struct OrbitCameraState {
  float yaw_degrees = 45.0f;
  float pitch_degrees = 58.0f;
  float distance = 42.0f;
  Vector3 target = {0.0f, 1.2f, 0.0f};
};

struct SceneData {
  sar::LevelData level;
  sar::render3d::ModelRegistry3D registry;
  MapSlice slice;
  ForestRenderMode forest_render_mode = ForestRenderMode::kComposition;
  std::vector<ForestTileBand> forest_bands;
  std::vector<ModelPlacement> placements;
  ModelCache model_cache;
};

float DegreesToRadians(float degrees) {
  return degrees * kPi / 180.0f;
}

std::uint64_t MixSeed(std::uint64_t value) {
  value ^= value >> 30U;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27U;
  value *= 0x94d049bb133111ebULL;
  value ^= value >> 31U;
  return value;
}

float Hash01(std::uint64_t seed) {
  const std::uint64_t mixed = MixSeed(seed);
  return static_cast<float>(mixed & 0x00ffffffULL) /
         static_cast<float>(0x01000000ULL);
}

int HashPercent(std::uint64_t seed) {
  return static_cast<int>(MixSeed(seed) % 100ULL);
}

std::string ReadTextFile(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("failed to open file: " + path.string());
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

std::optional<std::string> ExtractJsonStringField(std::string_view json,
                                                  std::string_view field_name) {
  const std::string needle = "\"" + std::string(field_name) + "\"";
  const std::size_t key_pos = json.find(needle);
  if (key_pos == std::string_view::npos) {
    return std::nullopt;
  }

  const std::size_t colon_pos = json.find(':', key_pos + needle.size());
  if (colon_pos == std::string_view::npos) {
    return std::nullopt;
  }

  const std::size_t quote_begin = json.find('"', colon_pos + 1U);
  if (quote_begin == std::string_view::npos) {
    return std::nullopt;
  }

  std::string value;
  for (std::size_t pos = quote_begin + 1U; pos < json.size(); ++pos) {
    const char current = json[pos];
    if (current == '"') {
      return value;
    }
    if (current == '\\' && pos + 1U < json.size()) {
      ++pos;
      value.push_back(json[pos]);
      continue;
    }
    value.push_back(current);
  }

  return std::nullopt;
}

std::filesystem::path ResolveProjectPath(const std::filesystem::path& project_root,
                                         const std::filesystem::path& path) {
  if (path.is_absolute()) {
    return path.lexically_normal();
  }
  return (project_root / path).lexically_normal();
}

SpikeConfig LoadSpikeConfig(int argc, char** argv) {
  SpikeConfig config{};
  config.project_root = std::filesystem::path(SAR_PROJECT_ROOT).lexically_normal();
  config.app_config_path = config.project_root / "config/app_config.json";

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    constexpr std::string_view kSlicePrefix = "--slice=";
    constexpr std::string_view kMapPrefix = "--map=";
    constexpr std::string_view kForestModePrefix = "--forest-mode=";
    if (argument.rfind(kSlicePrefix, 0) == 0) {
      config.slice_size_tiles = std::clamp(
          std::stoi(std::string(argument.substr(kSlicePrefix.size()))), 16, 160);
    } else if (argument.rfind(kMapPrefix, 0) == 0) {
      config.map_package_path = std::filesystem::path(std::string(argument.substr(kMapPrefix.size())));
    } else if (argument.rfind(kForestModePrefix, 0) == 0) {
      config.forest_render_mode = ParseForestRenderMode(
          argument.substr(kForestModePrefix.size()));
    }
  }

  const std::string app_config = ReadTextFile(config.app_config_path);
  if (config.map_package_path.empty()) {
    const std::optional<std::string> map_path =
        ExtractJsonStringField(app_config, "map_package_path");
    if (!map_path.has_value()) {
      throw std::runtime_error("map_package_path was not found in config/app_config.json");
    }
    config.map_package_path = *map_path;
  }

  const std::string asset_path = ExtractJsonStringField(
      app_config, "render3d_asset_library_path").value_or(
      "config/render3d/asset_library.json");
  const std::string tileset_path = ExtractJsonStringField(
      app_config, "render3d_tileset_path").value_or(
      "config/render3d/tileset_dark_forest.json");

  config.map_package_path = ResolveProjectPath(config.project_root, config.map_package_path);
  config.asset_library_path = ResolveProjectPath(config.project_root, asset_path);
  config.tileset_path = ResolveProjectPath(config.project_root, tileset_path);
  return config;
}

R3D_Material MakeMaterial(Color color, float roughness) {
  R3D_Material material = R3D_GetDefaultMaterial();
  material.albedo.color = color;
  material.orm.roughness = roughness;
  material.orm.metalness = 0.0f;
  return material;
}

PrimitiveMeshes LoadPrimitiveMeshes() {
  PrimitiveMeshes meshes{};
  meshes.tile = R3D_GenMeshCube(1.0f, 0.06f, 1.0f);
  meshes.cube = R3D_GenMeshCube(1.0f, 1.0f, 1.0f);
  meshes.trunk = R3D_GenMeshCylinder(0.12f, 1.15f, 10);
  meshes.canopy = R3D_GenMeshSphere(0.55f, 10, 14);
  return meshes;
}

void UnloadPrimitiveMeshes(const PrimitiveMeshes& meshes) {
  R3D_UnloadMesh(meshes.canopy);
  R3D_UnloadMesh(meshes.trunk);
  R3D_UnloadMesh(meshes.cube);
  R3D_UnloadMesh(meshes.tile);
}

SceneMaterials LoadSceneMaterials() {
  SceneMaterials materials{};
  materials.open_ground = MakeMaterial(Color{64, 86, 52, 255}, 0.96f);
  materials.forest_floor = MakeMaterial(Color{28, 51, 34, 255}, 0.98f);
  materials.forest_edge_floor = MakeMaterial(Color{34, 64, 40, 255}, 0.98f);
  materials.forest_mid_floor = MakeMaterial(Color{24, 48, 32, 255}, 0.99f);
  materials.forest_deep_floor = MakeMaterial(Color{13, 31, 21, 255}, 1.0f);
  materials.forest_canopy_mass = MakeMaterial(Color{7, 22, 14, 185}, 1.0f);
  materials.road = MakeMaterial(Color{95, 78, 50, 255}, 0.92f);
  materials.swamp = MakeMaterial(Color{42, 73, 55, 255}, 0.96f);
  materials.water = MakeMaterial(Color{34, 59, 68, 255}, 0.88f);
  materials.ruins = MakeMaterial(Color{91, 91, 82, 255}, 0.90f);
  materials.wall = MakeMaterial(Color{64, 63, 59, 255}, 0.92f);
  materials.unknown = MakeMaterial(Color{80, 40, 80, 255}, 0.95f);
  materials.debug_stone = MakeMaterial(Color{95, 92, 84, 255}, 0.90f);
  materials.fallback_trunk = MakeMaterial(Color{72, 45, 29, 255}, 0.94f);
  materials.fallback_canopy = MakeMaterial(Color{19, 78, 37, 255}, 0.98f);
  return materials;
}

const R3D_Material& MaterialForTerrain(const SceneMaterials& materials,
                                       sar::TerrainType terrain) {
  switch (terrain) {
    case sar::TerrainType::kOpenGround:
      return materials.open_ground;
    case sar::TerrainType::kForest:
      return materials.forest_floor;
    case sar::TerrainType::kRoad:
      return materials.road;
    case sar::TerrainType::kSwamp:
      return materials.swamp;
    case sar::TerrainType::kWater:
      return materials.water;
    case sar::TerrainType::kRuins:
      return materials.ruins;
    case sar::TerrainType::kWall:
      return materials.wall;
    case sar::TerrainType::kUnknown:
      return materials.unknown;
  }
  return materials.unknown;
}

int CellIndex(const sar::LevelData& level, int x, int y) {
  return y * level.size.width + x;
}

Vector3 TileToLocalPosition(const MapSlice& slice, int tile_x, int tile_y, float elevation_y) {
  return Vector3{
      static_cast<float>(tile_x - slice.x0) - static_cast<float>(slice.width) * 0.5f + 0.5f,
      elevation_y,
      static_cast<float>(tile_y - slice.y0) - static_cast<float>(slice.height) * 0.5f + 0.5f,
  };
}

float ElevationToY(std::int8_t height) {
  return static_cast<float>(height) * 0.18f;
}


bool IsInsideLevel(const sar::LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

bool IsForestCell(const sar::LevelData& level, int x, int y) {
  if (!IsInsideLevel(level, x, y)) {
    return false;
  }
  return level.cells[static_cast<std::size_t>(CellIndex(level, x, y))].terrain ==
         sar::TerrainType::kForest;
}

bool HasForestRadius(const sar::LevelData& level, int tile_x, int tile_y, int radius) {
  for (int y = tile_y - radius; y <= tile_y + radius; ++y) {
    for (int x = tile_x - radius; x <= tile_x + radius; ++x) {
      if (!IsForestCell(level, x, y)) {
        return false;
      }
    }
  }
  return true;
}

ForestTileBand ClassifyForestTile(const sar::LevelData& level, int tile_x, int tile_y) {
  if (!IsForestCell(level, tile_x, tile_y)) {
    return ForestTileBand::kNone;
  }

  constexpr std::array<std::pair<int, int>, 4> kCardinalOffsets{{
      {0, -1},
      {1, 0},
      {0, 1},
      {-1, 0},
  }};
  constexpr std::array<std::pair<int, int>, 8> kNeighborOffsets{{
      {-1, -1},
      {0, -1},
      {1, -1},
      {-1, 0},
      {1, 0},
      {-1, 1},
      {0, 1},
      {1, 1},
  }};

  int cardinal_neighbors = 0;
  for (const auto& [dx, dy] : kCardinalOffsets) {
    if (IsForestCell(level, tile_x + dx, tile_y + dy)) {
      ++cardinal_neighbors;
    }
  }

  int eight_neighbors = 0;
  for (const auto& [dx, dy] : kNeighborOffsets) {
    if (IsForestCell(level, tile_x + dx, tile_y + dy)) {
      ++eight_neighbors;
    }
  }

  if (cardinal_neighbors < 4 || eight_neighbors < 7) {
    return ForestTileBand::kEdge;
  }

  if (HasForestRadius(level, tile_x, tile_y, 2)) {
    return ForestTileBand::kDeep;
  }

  return ForestTileBand::kMid;
}

std::size_t SliceIndex(const MapSlice& slice, int tile_x, int tile_y) {
  const int local_x = tile_x - slice.x0;
  const int local_y = tile_y - slice.y0;
  return static_cast<std::size_t>(local_y * slice.width + local_x);
}

ForestTileBand ForestBandAt(const SceneData& scene, int tile_x, int tile_y) {
  if (tile_x < scene.slice.x0 || tile_y < scene.slice.y0 ||
      tile_x >= scene.slice.x0 + scene.slice.width ||
      tile_y >= scene.slice.y0 + scene.slice.height) {
    return ForestTileBand::kNone;
  }
  const std::size_t index = SliceIndex(scene.slice, tile_x, tile_y);
  if (index >= scene.forest_bands.size()) {
    return ForestTileBand::kNone;
  }
  return scene.forest_bands[index];
}

const R3D_Material& MaterialForForestBand(const SceneMaterials& materials,
                                          ForestTileBand band) {
  switch (band) {
    case ForestTileBand::kEdge:
      return materials.forest_edge_floor;
    case ForestTileBand::kMid:
      return materials.forest_mid_floor;
    case ForestTileBand::kDeep:
      return materials.forest_deep_floor;
    case ForestTileBand::kNone:
      return materials.forest_floor;
  }
  return materials.forest_floor;
}

std::optional<std::pair<int, int>> FindPreferredCenter(const sar::LevelData& level) {
  for (const sar::Marker& marker : level.markers) {
    if (marker.type == "player_spawn" || marker.type == "start") {
      return std::pair<int, int>{marker.x, marker.y};
    }
  }
  for (const sar::Place& place : level.places) {
    if (place.type == "start" || place.id.find("start") != std::string::npos) {
      return std::pair<int, int>{place.x, place.y};
    }
  }
  return std::nullopt;
}

MapSlice BuildSlice(const sar::LevelData& level, int requested_size) {
  const int width = std::max(1, level.size.width);
  const int height = std::max(1, level.size.height);
  const int slice_width = std::min(width, requested_size);
  const int slice_height = std::min(height, requested_size);

  const std::optional<std::pair<int, int>> preferred_center = FindPreferredCenter(level);
  const int center_x = preferred_center.has_value() ? preferred_center->first : width / 2;
  const int center_y = preferred_center.has_value() ? preferred_center->second : height / 2;

  MapSlice slice{};
  slice.width = slice_width;
  slice.height = slice_height;
  slice.center_tile_x = std::clamp(center_x, 0, width - 1);
  slice.center_tile_y = std::clamp(center_y, 0, height - 1);
  slice.x0 = std::clamp(slice.center_tile_x - slice_width / 2, 0, width - slice_width);
  slice.y0 = std::clamp(slice.center_tile_y - slice_height / 2, 0, height - slice_height);
  return slice;
}

bool ProbabilityPass(std::uint64_t seed, float probability) {
  const float clamped = std::clamp(probability, 0.0f, 1.0f);
  return Hash01(seed) <= clamped;
}

float RandomRange(std::uint64_t seed, float min_value, float max_value) {
  return min_value + (max_value - min_value) * Hash01(seed);
}

LoadedModel* LoadModelIfNeeded(ModelCache* cache,
                               const sar::render3d::ModelRegistry3D& registry,
                               const std::filesystem::path& project_root,
                               std::string_view model_id) {
  auto found = cache->models.find(std::string(model_id));
  if (found != cache->models.end()) {
    return &found->second;
  }

  LoadedModel loaded{};
  const sar::render3d::ModelAsset3D* asset = registry.FindAsset(model_id);
  if (asset != nullptr) {
    const std::filesystem::path model_path = ResolveProjectPath(project_root, asset->path);
    if (std::filesystem::exists(model_path)) {
      loaded.model = R3D_LoadModel(model_path.string().c_str());
      loaded.valid = loaded.model.meshCount > 0;
      if (!loaded.valid) {
        TraceLog(LOG_WARNING, "R3D model has no meshes: %s", model_path.string().c_str());
      }
    } else {
      TraceLog(LOG_WARNING, "Model path does not exist: %s", model_path.string().c_str());
    }
  }

  const auto [inserted, _] = cache->models.emplace(std::string(model_id), loaded);
  return &inserted->second;
}

void UnloadModelCache(ModelCache* cache) {
  for (auto& [_, loaded] : cache->models) {
    if (loaded.valid) {
      R3D_UnloadModel(loaded.model, true);
      loaded.valid = false;
    }
  }
  cache->models.clear();
}

std::optional<ModelPlacement> TryCreatePlacement(
    const sar::render3d::ModelRegistry3D& registry,
    std::string_view semantic_key,
    int tile_x,
    int tile_y,
    Vector3 tile_position,
    std::int8_t height) {
  const sar::render3d::ModelBinding3D* binding = registry.FindBinding(semantic_key);
  if (binding == nullptr) {
    return std::nullopt;
  }

  const std::uint64_t seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, semantic_key, tile_x, tile_y);
  if (!ProbabilityPass(seed, binding->spawn_chance)) {
    return std::nullopt;
  }

  const std::string model_id = registry.SelectModelId(semantic_key, kSpikeSeed, tile_x, tile_y);
  if (model_id.empty()) {
    return std::nullopt;
  }

  const float offset_radius = RandomRange(seed ^ 0xA53A9ULL, binding->offset_min, binding->offset_max);
  const float offset_angle = RandomRange(seed ^ 0x514B2ULL, 0.0f, kPi * 2.0f);
  const float yaw = binding->random_rotation
                        ? RandomRange(seed ^ 0xCAFEULL, 0.0f, kPi * 2.0f)
                        : 0.0f;
  const float scale_value = RandomRange(seed ^ 0xBEEFULL, binding->scale_min, binding->scale_max);

  ModelPlacement placement{};
  placement.semantic_key = std::string(semantic_key);
  placement.model_id = model_id;
  placement.position = Vector3{
      tile_position.x + std::cos(offset_angle) * offset_radius,
      ElevationToY(height) + binding->vertical_offset,
      tile_position.z + std::sin(offset_angle) * offset_radius,
  };
  placement.rotation = QuaternionFromAxisAngle(Vector3{0.0f, 1.0f, 0.0f}, yaw);
  placement.scale = Vector3{scale_value, scale_value, scale_value};
  return placement;
}

void AddPlacementIfAvailable(SceneData* scene,
                             std::string_view semantic_key,
                             int tile_x,
                             int tile_y,
                             Vector3 tile_position,
                             std::int8_t height,
                             int* counter) {
  std::optional<ModelPlacement> placement = TryCreatePlacement(
      scene->registry, semantic_key, tile_x, tile_y, tile_position, height);
  if (!placement.has_value()) {
    return;
  }
  ++(*counter);
  scene->placements.push_back(std::move(*placement));
}

void AddRawForestPlacements(SceneData* scene,
                            int tile_x,
                            int tile_y,
                            Vector3 tile_position,
                            std::int8_t height) {
  if (HashPercent(sar::render3d::DeterministicAssetSeed(
          kSpikeSeed, "forest_density", tile_x, tile_y)) < 72) {
    const int before = scene->slice.tree_model_instances;
    AddPlacementIfAvailable(
        scene,
        "forest_tree_blocker",
        tile_x,
        tile_y,
        tile_position,
        height,
        &scene->slice.tree_model_instances);
    if (scene->slice.tree_model_instances == before) {
      ++scene->slice.fallback_tree_instances;
    }
  }

  if (HashPercent(sar::render3d::DeterministicAssetSeed(
          kSpikeSeed, "underbrush_density", tile_x, tile_y)) < 28) {
    AddPlacementIfAvailable(
        scene,
        "forest_underbrush",
        tile_x,
        tile_y,
        tile_position,
        height,
        &scene->slice.underbrush_model_instances);
  }
}

float ForestTreeDensity(ForestTileBand band) {
  switch (band) {
    case ForestTileBand::kEdge:
      return 0.82f;
    case ForestTileBand::kMid:
      return 0.46f;
    case ForestTileBand::kDeep:
      return 0.18f;
    case ForestTileBand::kNone:
      return 0.0f;
  }
  return 0.0f;
}

float ForestUnderbrushDensity(ForestTileBand band) {
  switch (band) {
    case ForestTileBand::kEdge:
      return 0.30f;
    case ForestTileBand::kMid:
      return 0.18f;
    case ForestTileBand::kDeep:
      return 0.08f;
    case ForestTileBand::kNone:
      return 0.0f;
  }
  return 0.0f;
}

void ApplyCompositionTransform(ModelPlacement* placement,
                               ForestTileBand band,
                               int tile_x,
                               int tile_y) {
  const std::uint64_t seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, std::string("forest_composition_") + ForestTileBandName(band), tile_x, tile_y);
  const float extra_scale = RandomRange(
      seed ^ 0x51F00DULL,
      band == ForestTileBand::kEdge ? 1.00f : 0.86f,
      band == ForestTileBand::kDeep ? 1.18f : 1.28f);
  placement->scale.x *= extra_scale;
  placement->scale.y *= extra_scale;
  placement->scale.z *= extra_scale;

  const float jitter_radius = band == ForestTileBand::kEdge ? 0.22f : 0.34f;
  const float jitter_angle = RandomRange(seed ^ 0xA11CEULL, 0.0f, kPi * 2.0f);
  placement->position.x += std::cos(jitter_angle) * jitter_radius;
  placement->position.z += std::sin(jitter_angle) * jitter_radius;
}

void AddCompositionForestPlacements(SceneData* scene,
                                    ForestTileBand band,
                                    int tile_x,
                                    int tile_y,
                                    Vector3 tile_position,
                                    std::int8_t height) {
  const float tree_density = ForestTreeDensity(band);
  const std::uint64_t tree_seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_composition_tree_density", tile_x, tile_y);
  if (ProbabilityPass(tree_seed, tree_density)) {
    std::optional<ModelPlacement> placement = TryCreatePlacement(
        scene->registry, "forest_tree_blocker", tile_x, tile_y, tile_position, height);
    if (placement.has_value()) {
      ApplyCompositionTransform(&(*placement), band, tile_x, tile_y);
      ++scene->slice.tree_model_instances;
      scene->placements.push_back(std::move(*placement));
    } else {
      ++scene->slice.fallback_tree_instances;
    }
  } else {
    ++scene->slice.skipped_tree_tiles;
  }

  const float underbrush_density = ForestUnderbrushDensity(band);
  const std::uint64_t underbrush_seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_composition_underbrush_density", tile_x, tile_y);
  if (ProbabilityPass(underbrush_seed, underbrush_density)) {
    AddPlacementIfAvailable(
        scene,
        "forest_underbrush",
        tile_x,
        tile_y,
        tile_position,
        height,
        &scene->slice.underbrush_model_instances);
  }
}

SceneData LoadSceneData(const SpikeConfig& config) {
  sar::LevelLoader loader;
  sar::LevelLoadResult level_result = loader.LoadBasicPackage(config.map_package_path);
  if (!level_result.ok) {
    throw std::runtime_error("failed to load map package: " + level_result.error);
  }

  sar::render3d::LoadModelRegistry3DResult registry_result =
      sar::render3d::LoadModelRegistry3D(config.asset_library_path, config.tileset_path);
  if (!registry_result.ok) {
    throw std::runtime_error("failed to load 3D registry: " + registry_result.error);
  }

  SceneData scene{};
  scene.level = std::move(level_result.level);
  scene.registry = std::move(registry_result.registry);
  scene.slice = BuildSlice(scene.level, config.slice_size_tiles);
  scene.forest_render_mode = config.forest_render_mode;
  scene.forest_bands.assign(
      static_cast<std::size_t>(scene.slice.width * scene.slice.height),
      ForestTileBand::kNone);

  for (int y = scene.slice.y0; y < scene.slice.y0 + scene.slice.height; ++y) {
    for (int x = scene.slice.x0; x < scene.slice.x0 + scene.slice.width; ++x) {
      const sar::RuntimeCell& cell = scene.level.cells[static_cast<std::size_t>(CellIndex(scene.level, x, y))];
      const Vector3 tile_position = TileToLocalPosition(scene.slice, x, y, ElevationToY(cell.height));

      if (cell.terrain == sar::TerrainType::kForest) {
        ++scene.slice.forest_tiles;
        const ForestTileBand band = ClassifyForestTile(scene.level, x, y);
        scene.forest_bands[SliceIndex(scene.slice, x, y)] = band;
        switch (band) {
          case ForestTileBand::kEdge:
            ++scene.slice.forest_edge_tiles;
            break;
          case ForestTileBand::kMid:
            ++scene.slice.forest_mid_tiles;
            break;
          case ForestTileBand::kDeep:
            ++scene.slice.forest_deep_tiles;
            break;
          case ForestTileBand::kNone:
            break;
        }

        if (scene.forest_render_mode == ForestRenderMode::kRaw) {
          AddRawForestPlacements(&scene, x, y, tile_position, cell.height);
        } else {
          AddCompositionForestPlacements(&scene, band, x, y, tile_position, cell.height);
        }
      } else if (cell.terrain == sar::TerrainType::kOpenGround ||
                 cell.terrain == sar::TerrainType::kRoad) {
        if (HashPercent(sar::render3d::DeterministicAssetSeed(kSpikeSeed, "ground_detail_density", x, y)) < 6) {
          std::optional<ModelPlacement> placement = TryCreatePlacement(
              scene.registry, "open_ground_detail", x, y, tile_position, cell.height);
          if (placement.has_value()) {
            ++scene.slice.detail_model_instances;
            scene.placements.push_back(std::move(*placement));
          }
        }
      }
    }
  }

  return scene;
}

void DrawFallbackTree(const PrimitiveMeshes& meshes,
                      const SceneMaterials& materials,
                      Vector3 base_position,
                      float scale) {
  R3D_DrawMesh(
      meshes.trunk,
      materials.fallback_trunk,
      Vector3{base_position.x, base_position.y + 0.58f * scale, base_position.z},
      scale);
  R3D_DrawMesh(
      meshes.canopy,
      materials.fallback_canopy,
      Vector3{base_position.x, base_position.y + 1.28f * scale, base_position.z},
      scale);
}

void DrawTerrain(const SceneData& scene,
                 const PrimitiveMeshes& meshes,
                 const SceneMaterials& materials) {
  for (int y = scene.slice.y0; y < scene.slice.y0 + scene.slice.height; ++y) {
    for (int x = scene.slice.x0; x < scene.slice.x0 + scene.slice.width; ++x) {
      const sar::RuntimeCell& cell = scene.level.cells[static_cast<std::size_t>(CellIndex(scene.level, x, y))];
      const Vector3 position = TileToLocalPosition(scene.slice, x, y, ElevationToY(cell.height) - 0.04f);
      if (cell.terrain == sar::TerrainType::kForest &&
          scene.forest_render_mode == ForestRenderMode::kComposition) {
        const ForestTileBand band = ForestBandAt(scene, x, y);
        R3D_DrawMesh(meshes.tile, MaterialForForestBand(materials, band), position, 1.0f);
        if (band == ForestTileBand::kDeep) {
          R3D_DrawMeshEx(
              meshes.tile,
              materials.forest_canopy_mass,
              Vector3{position.x, position.y + 0.06f, position.z},
              QuaternionIdentity(),
              Vector3{1.02f, 0.55f, 1.02f});
        }
      } else {
        R3D_DrawMesh(meshes.tile, MaterialForTerrain(materials, cell.terrain), position, 1.0f);
      }

      if (cell.terrain == sar::TerrainType::kWall ||
          (cell.collision && cell.terrain == sar::TerrainType::kRuins)) {
        R3D_DrawMeshEx(
            meshes.cube,
            materials.debug_stone,
            Vector3{position.x, position.y + 0.58f, position.z},
            QuaternionIdentity(),
            Vector3{0.92f, 1.1f, 0.92f});
      }
    }
  }
}

void DrawPlacements(SceneData* scene,
                    const SpikeConfig& config,
                    const PrimitiveMeshes& meshes,
                    const SceneMaterials& materials) {
  for (ModelPlacement& placement : scene->placements) {
    LoadedModel* loaded = LoadModelIfNeeded(
        &scene->model_cache, scene->registry, config.project_root, placement.model_id);
    if (loaded != nullptr && loaded->valid) {
      const sar::render3d::ModelAsset3D* asset = scene->registry.FindAsset(placement.model_id);
      Vector3 scale = placement.scale;
      Vector3 position = placement.position;
      if (asset != nullptr) {
        scale.x *= asset->default_scale;
        scale.y *= asset->default_scale;
        scale.z *= asset->default_scale;
        position.y += asset->vertical_offset;
      }
      R3D_DrawModelEx(loaded->model, position, placement.rotation, scale);
    } else if (placement.semantic_key == "forest_tree_blocker") {
      DrawFallbackTree(meshes, materials, placement.position, 1.0f);
    } else {
      R3D_DrawMeshEx(
          meshes.cube,
          materials.debug_stone,
          Vector3{placement.position.x, placement.position.y + 0.08f, placement.position.z},
          placement.rotation,
          Vector3{0.22f, 0.16f, 0.22f});
    }
  }
}

void DrawRuntimeObjects(const SceneData& scene,
                        const PrimitiveMeshes& meshes,
                        const SceneMaterials& materials) {
  for (const sar::RuntimeObject& object : scene.level.objects) {
    const bool in_slice = object.x + object.width > scene.slice.x0 &&
                          object.y + object.height > scene.slice.y0 &&
                          object.x < scene.slice.x0 + scene.slice.width &&
                          object.y < scene.slice.y0 + scene.slice.height;
    if (!in_slice) {
      continue;
    }
    const Vector3 base = TileToLocalPosition(
        scene.slice,
        object.x + object.width / 2,
        object.y + object.height / 2,
        ElevationToY(object.elevation));
    const Vector3 scale{
        std::max(0.55f, static_cast<float>(object.width) * 0.75f),
        object.blocks_vision ? 1.25f : 0.45f,
        std::max(0.55f, static_cast<float>(object.height) * 0.75f),
    };
    R3D_DrawMeshEx(
        meshes.cube,
        materials.debug_stone,
        Vector3{base.x, base.y + scale.y * 0.5f, base.z},
        QuaternionFromAxisAngle(Vector3{0.0f, 1.0f, 0.0f}, DegreesToRadians(static_cast<float>(object.rotation))),
        scale);
  }
}

void DrawScene(SceneData* scene,
               const SpikeConfig& config,
               const PrimitiveMeshes& meshes,
               const SceneMaterials& materials) {
  DrawTerrain(*scene, meshes, materials);
  DrawRuntimeObjects(*scene, meshes, materials);
  DrawPlacements(scene, config, meshes, materials);
}

void UpdateOrbitCamera(OrbitCameraState* state, Camera3D* camera) {
  const float dt = GetFrameTime();

  if (IsKeyDown(KEY_Q)) {
    state->yaw_degrees -= 60.0f * dt;
  }
  if (IsKeyDown(KEY_E)) {
    state->yaw_degrees += 60.0f * dt;
  }
  if (IsKeyDown(KEY_R)) {
    state->pitch_degrees += 45.0f * dt;
  }
  if (IsKeyDown(KEY_F)) {
    state->pitch_degrees -= 45.0f * dt;
  }

  state->pitch_degrees = std::clamp(state->pitch_degrees, 25.0f, 78.0f);
  state->distance = std::clamp(
      state->distance - GetMouseWheelMove() * 2.0f,
      12.0f,
      82.0f);

  const float yaw = DegreesToRadians(state->yaw_degrees);
  const float pitch = DegreesToRadians(state->pitch_degrees);
  const float horizontal_distance = std::cos(pitch) * state->distance;
  const float vertical_distance = std::sin(pitch) * state->distance;

  camera->target = state->target;
  camera->position = Vector3{
      state->target.x - std::sin(yaw) * horizontal_distance,
      state->target.y + vertical_distance,
      state->target.z - std::cos(yaw) * horizontal_distance,
  };
  camera->up = Vector3{0.0f, 1.0f, 0.0f};
  camera->fovy = 50.0f;
  camera->projection = CAMERA_PERSPECTIVE;
}

void DrawOverlay(const OrbitCameraState& camera_state,
                 const SpikeConfig& config,
                 const SceneData& scene) {
  DrawRectangle(8, 8, 800, 202, Color{0, 0, 0, 178});
  DrawText("ShootAndRun R3D forest composition spike", 18, 18, 20, RAYWHITE);
  DrawText(TextFormat("FPS: %d", GetFPS()), 18, 42, 18, RAYWHITE);
  DrawText(
      TextFormat(
          "map %dx%d  slice %dx%d at [%d,%d]  center tile [%d,%d]",
          scene.level.size.width,
          scene.level.size.height,
          scene.slice.width,
          scene.slice.height,
          scene.slice.x0,
          scene.slice.y0,
          scene.slice.center_tile_x,
          scene.slice.center_tile_y),
      18,
      64,
      16,
      RAYWHITE);
  DrawText(
      TextFormat(
          "forest mode %s  tiles %d edge=%d mid=%d deep=%d skipped=%d",
          ForestRenderModeName(scene.forest_render_mode),
          scene.slice.forest_tiles,
          scene.slice.forest_edge_tiles,
          scene.slice.forest_mid_tiles,
          scene.slice.forest_deep_tiles,
          scene.slice.skipped_tree_tiles),
      18,
      84,
      16,
      RAYWHITE);
  DrawText(
      TextFormat(
          "model instances trees=%d underbrush=%d detail=%d fallback=%d loaded models=%d",
          scene.slice.tree_model_instances,
          scene.slice.underbrush_model_instances,
          scene.slice.detail_model_instances,
          scene.slice.fallback_tree_instances,
          static_cast<int>(scene.model_cache.models.size())),
      18,
      104,
      16,
      RAYWHITE);
  DrawText(
      TextFormat(
          "camera yaw %.1f pitch %.1f distance %.1f",
          camera_state.yaw_degrees,
          camera_state.pitch_degrees,
          camera_state.distance),
      18,
      124,
      16,
      RAYWHITE);
  DrawText(TextFormat("map: %s", config.map_package_path.string().c_str()), 18, 144, 14, LIGHTGRAY);
  DrawText("Q/E yaw  R/F pitch  Wheel zoom  Esc exit  --forest-mode=raw|composition", 18, 164, 14, LIGHTGRAY);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const SpikeConfig config = LoadSpikeConfig(argc, argv);

    SetTraceLogLevel(LOG_INFO);
    InitWindow(kScreenWidth, kScreenHeight, "ShootAndRun - R3D forest slice spike");
    SetTargetFPS(60);

    R3D_Init(kScreenWidth, kScreenHeight);

    const PrimitiveMeshes meshes = LoadPrimitiveMeshes();
    const SceneMaterials materials = LoadSceneMaterials();
    SceneData scene = LoadSceneData(config);

    R3D_Light sun = R3D_CreateLight(R3D_LIGHT_DIR);
    R3D_SetLightDirection(sun, Vector3{-0.45f, -1.0f, -0.35f});
    R3D_SetLightActive(sun, true);

    R3D_Light fill = R3D_CreateLight(R3D_LIGHT_DIR);
    R3D_SetLightDirection(fill, Vector3{0.30f, -0.45f, 0.65f});
    R3D_SetLightActive(fill, true);

    OrbitCameraState camera_state{};
    camera_state.distance = std::max(30.0f, static_cast<float>(scene.slice.width) * 0.88f);
    Camera3D camera{};

    TraceLog(LOG_INFO, "R3D forest slice initialized");
    TraceLog(LOG_INFO, "map_package=%s", config.map_package_path.string().c_str());
    TraceLog(LOG_INFO, "asset_library=%s", config.asset_library_path.string().c_str());
    TraceLog(LOG_INFO, "tileset=%s", config.tileset_path.string().c_str());
    TraceLog(LOG_INFO, "forest_mode=%s", ForestRenderModeName(config.forest_render_mode));

    while (!WindowShouldClose()) {
      UpdateOrbitCamera(&camera_state, &camera);

      BeginDrawing();
      ClearBackground(Color{9, 11, 13, 255});

      R3D_Begin(camera);
      DrawScene(&scene, config, meshes, materials);
      R3D_End();

      DrawOverlay(camera_state, config, scene);
      EndDrawing();
    }

    UnloadModelCache(&scene.model_cache);
    UnloadPrimitiveMeshes(meshes);
    R3D_Close();
    CloseWindow();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "sar_r3d_minimal failed: " << error.what() << '\n';
    return 1;
  }
}
