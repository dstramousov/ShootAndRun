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
#include <limits>
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

enum class R3DRenderProfile {
  kBasic,
  kAdvanced,
};

enum class BenchmarkMode {
  kForest,
  kInstancing,
  kCulling,
  kLighting,
  kPbr,
  kTerrainMesh,
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

const char* R3DRenderProfileName(R3DRenderProfile profile) {
  switch (profile) {
    case R3DRenderProfile::kBasic:
      return "basic";
    case R3DRenderProfile::kAdvanced:
      return "advanced";
  }
  return "advanced";
}

const char* BenchmarkModeName(BenchmarkMode mode) {
  switch (mode) {
    case BenchmarkMode::kForest:
      return "forest";
    case BenchmarkMode::kInstancing:
      return "instancing";
    case BenchmarkMode::kCulling:
      return "culling";
    case BenchmarkMode::kLighting:
      return "lighting";
    case BenchmarkMode::kPbr:
      return "pbr";
    case BenchmarkMode::kTerrainMesh:
      return "terrain-mesh";
  }
  return "forest";
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

R3DRenderProfile ParseR3DRenderProfile(std::string_view value) {
  if (value == "basic") {
    return R3DRenderProfile::kBasic;
  }
  if (value == "advanced") {
    return R3DRenderProfile::kAdvanced;
  }
  throw std::runtime_error(
      "unsupported --r3d-profile value; expected basic or advanced");
}

BenchmarkMode ParseBenchmarkMode(std::string_view value) {
  if (value == "forest") {
    return BenchmarkMode::kForest;
  }
  if (value == "instancing") {
    return BenchmarkMode::kInstancing;
  }
  if (value == "culling") {
    return BenchmarkMode::kCulling;
  }
  if (value == "lighting") {
    return BenchmarkMode::kLighting;
  }
  if (value == "pbr") {
    return BenchmarkMode::kPbr;
  }
  if (value == "terrain-mesh") {
    return BenchmarkMode::kTerrainMesh;
  }
  throw std::runtime_error(
      "unsupported --benchmark value; expected forest, instancing, culling, lighting, pbr, or terrain-mesh");
}

float ParseClampedFloat(std::string_view value, float min_value, float max_value) {
  return std::clamp(std::stof(std::string(value)), min_value, max_value);
}

struct ForestCompositionTuning {
  float edge_tree_density = 0.74f;
  float mid_tree_density = 0.34f;
  float deep_tree_density = 0.10f;
  float edge_underbrush_density = 0.24f;
  float mid_underbrush_density = 0.13f;
  float deep_underbrush_density = 0.045f;
  float jitter = 0.48f;
  float shadow = 0.42f;
};

struct R3DFeatureTuning {
  R3DRenderProfile profile = R3DRenderProfile::kAdvanced;
  bool instancing = true;
  bool chunks = true;
  bool lod = true;
  bool shadows = true;
  bool fog = true;
  bool atmosphere = true;
  int chunk_size_tiles = 16;
  float underbrush_lod_distance = 56.0f;
  float detail_lod_distance = 44.0f;
  float shadow_opacity = 0.58f;
  float fog_density = 0.018f;
  bool coarse_terrain_chunks = false;
  bool pbr_probe = false;
  bool extra_lights = false;
};

struct SpikeConfig {
  std::filesystem::path project_root;
  std::filesystem::path app_config_path;
  std::filesystem::path map_package_path;
  std::filesystem::path asset_library_path;
  std::filesystem::path tileset_path;
  int slice_size_tiles = kDefaultSliceSizeTiles;
  ForestRenderMode forest_render_mode = ForestRenderMode::kComposition;
  ForestCompositionTuning forest_tuning;
  BenchmarkMode benchmark_mode = BenchmarkMode::kForest;
  R3DFeatureTuning r3d_features;
};

void ApplyBenchmarkPreset(SpikeConfig* config, BenchmarkMode mode) {
  config->benchmark_mode = mode;

  switch (mode) {
    case BenchmarkMode::kForest:
      break;
    case BenchmarkMode::kInstancing:
      config->r3d_features.profile = R3DRenderProfile::kAdvanced;
      config->r3d_features.instancing = true;
      config->r3d_features.chunks = false;
      config->r3d_features.lod = false;
      config->r3d_features.shadows = false;
      config->r3d_features.fog = false;
      config->r3d_features.atmosphere = false;
      break;
    case BenchmarkMode::kCulling:
      config->r3d_features.profile = R3DRenderProfile::kAdvanced;
      config->r3d_features.instancing = true;
      config->r3d_features.chunks = true;
      config->r3d_features.lod = false;
      config->r3d_features.shadows = false;
      config->r3d_features.fog = false;
      config->r3d_features.atmosphere = false;
      config->r3d_features.chunk_size_tiles = 16;
      break;
    case BenchmarkMode::kLighting:
      config->r3d_features.profile = R3DRenderProfile::kAdvanced;
      config->r3d_features.instancing = true;
      config->r3d_features.chunks = true;
      config->r3d_features.lod = true;
      config->r3d_features.shadows = true;
      config->r3d_features.fog = true;
      config->r3d_features.atmosphere = true;
      config->r3d_features.extra_lights = true;
      config->r3d_features.shadow_opacity = 0.52f;
      config->r3d_features.fog_density = 0.014f;
      break;
    case BenchmarkMode::kPbr:
      config->r3d_features.profile = R3DRenderProfile::kAdvanced;
      config->r3d_features.instancing = true;
      config->r3d_features.chunks = true;
      config->r3d_features.lod = true;
      config->r3d_features.shadows = true;
      config->r3d_features.fog = true;
      config->r3d_features.atmosphere = true;
      config->r3d_features.extra_lights = true;
      config->r3d_features.pbr_probe = true;
      config->r3d_features.shadow_opacity = 0.48f;
      config->r3d_features.fog_density = 0.010f;
      break;
    case BenchmarkMode::kTerrainMesh:
      config->r3d_features.profile = R3DRenderProfile::kAdvanced;
      config->r3d_features.instancing = true;
      config->r3d_features.chunks = true;
      config->r3d_features.lod = true;
      config->r3d_features.shadows = false;
      config->r3d_features.fog = false;
      config->r3d_features.atmosphere = false;
      config->r3d_features.coarse_terrain_chunks = true;
      config->r3d_features.chunk_size_tiles = 8;
      break;
  }
}

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
  R3D_Material pbr_rough_stone;
  R3D_Material pbr_wet_mud;
  R3D_Material pbr_metal;
  R3D_Material pbr_glossy_water;
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
  bool instanced = false;
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
  int instance_batches = 0;
  int instanced_model_instances = 0;
  int lod_hidden_instances = 0;
  int terrain_clusters = 0;
};

struct OrbitCameraState {
  float yaw_degrees = 45.0f;
  float pitch_degrees = 58.0f;
  float distance = 42.0f;
  Vector3 target = {0.0f, 1.2f, 0.0f};
};

struct InstanceBatch {
  std::string semantic_key;
  std::string model_id;
  R3D_InstanceBuffer buffer{};
  int count = 0;
  BoundingBox bounds{};
  float max_lod_distance = std::numeric_limits<float>::infinity();
};

struct InstanceBatchBuildData {
  std::string semantic_key;
  std::string model_id;
  int chunk_x = 0;
  int chunk_y = 0;
  std::vector<Vector3> positions;
  std::vector<Quaternion> rotations;
  std::vector<Vector3> scales;
  BoundingBox bounds{
      Vector3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      Vector3{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()},
  };
};

struct SceneData {
  sar::LevelData level;
  sar::render3d::ModelRegistry3D registry;
  MapSlice slice;
  ForestRenderMode forest_render_mode = ForestRenderMode::kComposition;
  ForestCompositionTuning forest_tuning;
  R3DFeatureTuning r3d_features;
  std::vector<ForestTileBand> forest_bands;
  std::vector<ModelPlacement> placements;
  std::vector<InstanceBatch> instance_batches;
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
    constexpr std::string_view kForestEdgeDensityPrefix = "--forest-edge-density=";
    constexpr std::string_view kForestMidDensityPrefix = "--forest-mid-density=";
    constexpr std::string_view kForestDeepDensityPrefix = "--forest-deep-density=";
    constexpr std::string_view kForestJitterPrefix = "--forest-jitter=";
    constexpr std::string_view kForestShadowPrefix = "--forest-shadow=";
    constexpr std::string_view kBenchmarkPrefix = "--benchmark=";
    constexpr std::string_view kR3DProfilePrefix = "--r3d-profile=";
    constexpr std::string_view kR3DChunkSizePrefix = "--r3d-chunk-size=";
    constexpr std::string_view kR3DFogDensityPrefix = "--r3d-fog-density=";
    constexpr std::string_view kR3DShadowOpacityPrefix = "--r3d-shadow-opacity=";
    constexpr std::string_view kDisableShadows = "--no-shadows";
    constexpr std::string_view kDisableFog = "--no-fog";
    constexpr std::string_view kDisableInstancing = "--no-instancing";
    constexpr std::string_view kDisableLod = "--no-lod";
    if (argument.rfind(kSlicePrefix, 0) == 0) {
      config.slice_size_tiles = std::clamp(
          std::stoi(std::string(argument.substr(kSlicePrefix.size()))), 16, 160);
    } else if (argument.rfind(kMapPrefix, 0) == 0) {
      config.map_package_path = std::filesystem::path(std::string(argument.substr(kMapPrefix.size())));
    } else if (argument.rfind(kForestModePrefix, 0) == 0) {
      config.forest_render_mode = ParseForestRenderMode(
          argument.substr(kForestModePrefix.size()));
    } else if (argument.rfind(kForestEdgeDensityPrefix, 0) == 0) {
      config.forest_tuning.edge_tree_density = ParseClampedFloat(
          argument.substr(kForestEdgeDensityPrefix.size()), 0.0f, 1.0f);
    } else if (argument.rfind(kForestMidDensityPrefix, 0) == 0) {
      config.forest_tuning.mid_tree_density = ParseClampedFloat(
          argument.substr(kForestMidDensityPrefix.size()), 0.0f, 1.0f);
    } else if (argument.rfind(kForestDeepDensityPrefix, 0) == 0) {
      config.forest_tuning.deep_tree_density = ParseClampedFloat(
          argument.substr(kForestDeepDensityPrefix.size()), 0.0f, 1.0f);
    } else if (argument.rfind(kForestJitterPrefix, 0) == 0) {
      config.forest_tuning.jitter = ParseClampedFloat(
          argument.substr(kForestJitterPrefix.size()), 0.0f, 0.85f);
    } else if (argument.rfind(kForestShadowPrefix, 0) == 0) {
      config.forest_tuning.shadow = ParseClampedFloat(
          argument.substr(kForestShadowPrefix.size()), 0.0f, 1.0f);
    } else if (argument.rfind(kBenchmarkPrefix, 0) == 0) {
      ApplyBenchmarkPreset(
          &config, ParseBenchmarkMode(argument.substr(kBenchmarkPrefix.size())));
    } else if (argument.rfind(kR3DProfilePrefix, 0) == 0) {
      config.r3d_features.profile = ParseR3DRenderProfile(
          argument.substr(kR3DProfilePrefix.size()));
    } else if (argument.rfind(kR3DChunkSizePrefix, 0) == 0) {
      config.r3d_features.chunk_size_tiles = std::clamp(
          std::stoi(std::string(argument.substr(kR3DChunkSizePrefix.size()))), 4, 64);
    } else if (argument.rfind(kR3DFogDensityPrefix, 0) == 0) {
      config.r3d_features.fog_density = ParseClampedFloat(
          argument.substr(kR3DFogDensityPrefix.size()), 0.0f, 0.08f);
    } else if (argument.rfind(kR3DShadowOpacityPrefix, 0) == 0) {
      config.r3d_features.shadow_opacity = ParseClampedFloat(
          argument.substr(kR3DShadowOpacityPrefix.size()), 0.0f, 1.0f);
    } else if (argument == kDisableShadows) {
      config.r3d_features.shadows = false;
    } else if (argument == kDisableFog) {
      config.r3d_features.fog = false;
    } else if (argument == kDisableInstancing) {
      config.r3d_features.instancing = false;
    } else if (argument == kDisableLod) {
      config.r3d_features.lod = false;
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

  if (config.r3d_features.profile == R3DRenderProfile::kBasic) {
    config.r3d_features.instancing = false;
    config.r3d_features.chunks = false;
    config.r3d_features.lod = false;
    config.r3d_features.shadows = false;
    config.r3d_features.fog = false;
    config.r3d_features.atmosphere = false;
    config.r3d_features.coarse_terrain_chunks = false;
    config.r3d_features.pbr_probe = false;
    config.r3d_features.extra_lights = false;
  }

  config.map_package_path = ResolveProjectPath(config.project_root, config.map_package_path);
  config.asset_library_path = ResolveProjectPath(config.project_root, asset_path);
  config.tileset_path = ResolveProjectPath(config.project_root, tileset_path);
  return config;
}

R3D_Material MakeMaterial(Color color, float roughness, float specular = 0.35f, float metalness = 0.0f) {
  R3D_Material material = R3D_GetDefaultMaterial();
  material.albedo.color = color;
  material.orm.roughness = roughness;
  material.orm.specular = specular;
  material.orm.metalness = metalness;
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

SceneMaterials LoadSceneMaterials(const ForestCompositionTuning& tuning) {
  SceneMaterials materials{};
  materials.open_ground = MakeMaterial(Color{64, 86, 52, 255}, 0.94f, 0.18f);
  materials.forest_floor = MakeMaterial(Color{28, 51, 34, 255}, 0.98f, 0.12f);
  materials.forest_edge_floor = MakeMaterial(Color{34, 64, 40, 255}, 0.98f, 0.14f);
  materials.forest_mid_floor = MakeMaterial(Color{28, 54, 36, 255}, 0.99f, 0.10f);
  materials.forest_deep_floor = MakeMaterial(Color{18, 42, 27, 255}, 1.0f, 0.08f);
  const unsigned char canopy_alpha = static_cast<unsigned char>(
      std::clamp(70.0f + tuning.shadow * 105.0f, 40.0f, 190.0f));
  materials.forest_canopy_mass = MakeMaterial(Color{7, 22, 14, canopy_alpha}, 1.0f, 0.04f);
  materials.road = MakeMaterial(Color{95, 78, 50, 255}, 0.90f, 0.16f);
  materials.swamp = MakeMaterial(Color{42, 73, 55, 255}, 0.82f, 0.22f);
  materials.water = MakeMaterial(Color{34, 59, 68, 255}, 0.42f, 0.70f);
  materials.ruins = MakeMaterial(Color{91, 91, 82, 255}, 0.74f, 0.28f);
  materials.wall = MakeMaterial(Color{64, 63, 59, 255}, 0.78f, 0.24f);
  materials.unknown = MakeMaterial(Color{80, 40, 80, 255}, 0.95f);
  materials.debug_stone = MakeMaterial(Color{95, 92, 84, 255}, 0.90f);
  materials.fallback_trunk = MakeMaterial(Color{72, 45, 29, 255}, 0.94f);
  materials.fallback_canopy = MakeMaterial(Color{19, 78, 37, 255}, 0.98f);
  materials.pbr_rough_stone = MakeMaterial(Color{116, 112, 102, 255}, 0.96f, 0.18f, 0.0f);
  materials.pbr_wet_mud = MakeMaterial(Color{63, 47, 32, 255}, 0.34f, 0.72f, 0.0f);
  materials.pbr_metal = MakeMaterial(Color{118, 102, 78, 255}, 0.30f, 0.92f, 0.85f);
  materials.pbr_glossy_water = MakeMaterial(Color{36, 67, 78, 210}, 0.12f, 0.96f, 0.0f);
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

int TerrainBucketIndex(sar::TerrainType terrain) {
  switch (terrain) {
    case sar::TerrainType::kOpenGround:
      return 0;
    case sar::TerrainType::kForest:
      return 1;
    case sar::TerrainType::kRoad:
      return 2;
    case sar::TerrainType::kSwamp:
      return 3;
    case sar::TerrainType::kWater:
      return 4;
    case sar::TerrainType::kRuins:
      return 5;
    case sar::TerrainType::kWall:
      return 6;
    case sar::TerrainType::kUnknown:
      return 0;
  }
  return 0;
}

sar::TerrainType TerrainFromBucketIndex(int bucket) {
  switch (bucket) {
    case 0:
      return sar::TerrainType::kOpenGround;
    case 1:
      return sar::TerrainType::kForest;
    case 2:
      return sar::TerrainType::kRoad;
    case 3:
      return sar::TerrainType::kSwamp;
    case 4:
      return sar::TerrainType::kWater;
    case 5:
      return sar::TerrainType::kRuins;
    case 6:
      return sar::TerrainType::kWall;
    default:
      return sar::TerrainType::kOpenGround;
  }
}

sar::TerrainType DominantTerrainInChunk(
    const SceneData& scene, int x0, int y0, int x1, int y1) {
  std::array<int, 7> counts{};
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const sar::RuntimeCell& cell = scene.level.cells[
          static_cast<std::size_t>(CellIndex(scene.level, x, y))];
      ++counts[static_cast<std::size_t>(TerrainBucketIndex(cell.terrain))];
    }
  }

  int best_index = 0;
  for (int index = 1; index < static_cast<int>(counts.size()); ++index) {
    if (counts[static_cast<std::size_t>(index)] >
        counts[static_cast<std::size_t>(best_index)]) {
      best_index = index;
    }
  }
  return TerrainFromBucketIndex(best_index);
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

float Distance2D(Vector3 a, Vector3 b) {
  const float dx = a.x - b.x;
  const float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

void ExpandBounds(BoundingBox* bounds, Vector3 position, Vector3 half_extent) {
  bounds->min.x = std::min(bounds->min.x, position.x - half_extent.x);
  bounds->min.y = std::min(bounds->min.y, position.y - half_extent.y);
  bounds->min.z = std::min(bounds->min.z, position.z - half_extent.z);
  bounds->max.x = std::max(bounds->max.x, position.x + half_extent.x);
  bounds->max.y = std::max(bounds->max.y, position.y + half_extent.y);
  bounds->max.z = std::max(bounds->max.z, position.z + half_extent.z);
}

BoundingBox MakeTileChunkBounds(const MapSlice& slice, int x0, int y0, int x1, int y1) {
  const Vector3 min_pos = TileToLocalPosition(slice, x0, y0, -0.25f);
  const Vector3 max_pos = TileToLocalPosition(slice, x1 - 1, y1 - 1, 2.25f);
  return BoundingBox{
      Vector3{min_pos.x - 0.6f, -0.45f, min_pos.z - 0.6f},
      Vector3{max_pos.x + 0.6f, 2.75f, max_pos.z + 0.6f},
  };
}

int ChunkCoordForTile(int tile_coord, int slice_origin, int chunk_size) {
  return (tile_coord - slice_origin) / std::max(1, chunk_size);
}

float LodDistanceForSemantic(std::string_view semantic_key, const R3DFeatureTuning& features) {
  if (semantic_key == "forest_underbrush") {
    return features.underbrush_lod_distance;
  }
  if (semantic_key == "open_ground_detail") {
    return features.detail_lod_distance;
  }
  return std::numeric_limits<float>::infinity();
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

float ForestTreeDensity(ForestTileBand band, const ForestCompositionTuning& tuning) {
  switch (band) {
    case ForestTileBand::kEdge:
      return tuning.edge_tree_density;
    case ForestTileBand::kMid:
      return tuning.mid_tree_density;
    case ForestTileBand::kDeep:
      return tuning.deep_tree_density;
    case ForestTileBand::kNone:
      return 0.0f;
  }
  return 0.0f;
}

float ForestUnderbrushDensity(ForestTileBand band, const ForestCompositionTuning& tuning) {
  switch (band) {
    case ForestTileBand::kEdge:
      return tuning.edge_underbrush_density;
    case ForestTileBand::kMid:
      return tuning.mid_underbrush_density;
    case ForestTileBand::kDeep:
      return tuning.deep_underbrush_density;
    case ForestTileBand::kNone:
      return 0.0f;
  }
  return 0.0f;
}

float ForestOrganicDensityMultiplier(ForestTileBand band, int tile_x, int tile_y) {
  if (band == ForestTileBand::kNone) {
    return 0.0f;
  }

  const int macro_x = tile_x >= 0 ? tile_x / 5 : (tile_x - 4) / 5;
  const int macro_y = tile_y >= 0 ? tile_y / 5 : (tile_y - 4) / 5;
  const float macro = Hash01(sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_macro_patch", macro_x, macro_y));
  const float micro = Hash01(sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_micro_patch", tile_x, tile_y));

  if (band == ForestTileBand::kDeep && macro < 0.22f) {
    return 0.10f + micro * 0.18f;
  }
  if (band == ForestTileBand::kMid && macro < 0.13f) {
    return 0.28f + micro * 0.22f;
  }
  if (band == ForestTileBand::kEdge && micro < 0.08f) {
    return 0.45f;
  }

  return 0.58f + macro * 0.58f + micro * 0.20f;
}

bool ShouldDrawDeepCanopyMass(int tile_x, int tile_y) {
  const int macro_x = tile_x >= 0 ? tile_x / 3 : (tile_x - 2) / 3;
  const int macro_y = tile_y >= 0 ? tile_y / 3 : (tile_y - 2) / 3;
  const float macro = Hash01(sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_canopy_mass_patch", macro_x, macro_y));
  const float micro = Hash01(sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_canopy_mass_micro", tile_x, tile_y));
  return macro > 0.16f && micro > 0.10f;
}

void ApplyCompositionTransform(ModelPlacement* placement,
                               ForestTileBand band,
                               const ForestCompositionTuning& tuning,
                               int tile_x,
                               int tile_y) {
  const std::uint64_t seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, std::string("forest_composition_v2_") + ForestTileBandName(band), tile_x, tile_y);
  const float extra_scale = RandomRange(
      seed ^ 0x51F00DULL,
      band == ForestTileBand::kEdge ? 0.92f : 0.80f,
      band == ForestTileBand::kDeep ? 1.12f : 1.30f);
  placement->scale.x *= extra_scale;
  placement->scale.y *= extra_scale;
  placement->scale.z *= extra_scale;

  const float band_jitter =
      band == ForestTileBand::kEdge ? tuning.jitter * 0.82f :
      band == ForestTileBand::kMid ? tuning.jitter : tuning.jitter * 1.12f;
  placement->position.x += RandomRange(seed ^ 0xA11CEULL, -band_jitter, band_jitter);
  placement->position.z += RandomRange(seed ^ 0xB00DAULL, -band_jitter, band_jitter);
}

void AddCompositionForestPlacements(SceneData* scene,
                                    ForestTileBand band,
                                    int tile_x,
                                    int tile_y,
                                    Vector3 tile_position,
                                    std::int8_t height) {
  const float organic_multiplier = ForestOrganicDensityMultiplier(band, tile_x, tile_y);
  const float tree_density =
      ForestTreeDensity(band, scene->forest_tuning) * organic_multiplier;
  const std::uint64_t tree_seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_composition_v2_tree_density", tile_x, tile_y);
  if (ProbabilityPass(tree_seed, tree_density)) {
    std::optional<ModelPlacement> placement = TryCreatePlacement(
        scene->registry, "forest_tree_blocker", tile_x, tile_y, tile_position, height);
    if (placement.has_value()) {
      ApplyCompositionTransform(
          &(*placement), band, scene->forest_tuning, tile_x, tile_y);
      ++scene->slice.tree_model_instances;
      scene->placements.push_back(std::move(*placement));
    } else {
      ++scene->slice.fallback_tree_instances;
    }
  } else {
    ++scene->slice.skipped_tree_tiles;
  }

  const float underbrush_density =
      ForestUnderbrushDensity(band, scene->forest_tuning) *
      std::clamp(organic_multiplier * 1.15f, 0.0f, 1.35f);
  const std::uint64_t underbrush_seed = sar::render3d::DeterministicAssetSeed(
      kSpikeSeed, "forest_composition_v2_underbrush_density", tile_x, tile_y);
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
  scene.forest_tuning = config.forest_tuning;
  scene.r3d_features = config.r3d_features;
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

void UnloadInstanceBatches(SceneData* scene) {
  for (InstanceBatch& batch : scene->instance_batches) {
    if (batch.buffer.capacity > 0) {
      R3D_UnloadInstanceBuffer(batch.buffer);
    }
  }
  scene->instance_batches.clear();
}

void BuildInstanceBatches(SceneData* scene, const SpikeConfig& config) {
  if (!config.r3d_features.instancing) {
    return;
  }

  std::map<std::string, InstanceBatchBuildData> pending_batches;
  for (ModelPlacement& placement : scene->placements) {
    if (placement.model_id.empty() || placement.fallback) {
      continue;
    }

    LoadedModel* loaded = LoadModelIfNeeded(
        &scene->model_cache, scene->registry, config.project_root, placement.model_id);
    if (loaded == nullptr || !loaded->valid) {
      continue;
    }

    const sar::render3d::ModelAsset3D* asset = scene->registry.FindAsset(placement.model_id);
    Vector3 position = placement.position;
    Vector3 scale = placement.scale;
    if (asset != nullptr) {
      scale.x *= asset->default_scale;
      scale.y *= asset->default_scale;
      scale.z *= asset->default_scale;
      position.y += asset->vertical_offset;
    }

    const int tile_x = static_cast<int>(std::floor(
        placement.position.x + static_cast<float>(scene->slice.width) * 0.5f)) + scene->slice.x0;
    const int tile_y = static_cast<int>(std::floor(
        placement.position.z + static_cast<float>(scene->slice.height) * 0.5f)) + scene->slice.y0;
    const int chunk_x = ChunkCoordForTile(tile_x, scene->slice.x0, config.r3d_features.chunk_size_tiles);
    const int chunk_y = ChunkCoordForTile(tile_y, scene->slice.y0, config.r3d_features.chunk_size_tiles);
    const std::string key = placement.semantic_key + "|" + placement.model_id + "|" +
                            std::to_string(chunk_x) + "|" + std::to_string(chunk_y);

    InstanceBatchBuildData& batch = pending_batches[key];
    if (batch.model_id.empty()) {
      batch.semantic_key = placement.semantic_key;
      batch.model_id = placement.model_id;
      batch.chunk_x = chunk_x;
      batch.chunk_y = chunk_y;
    }
    batch.positions.push_back(position);
    batch.rotations.push_back(placement.rotation);
    batch.scales.push_back(scale);
    ExpandBounds(&batch.bounds, position, Vector3{1.8f, 2.6f, 1.8f});
    placement.instanced = true;
  }

  for (const auto& [_, build] : pending_batches) {
    const int count = static_cast<int>(build.positions.size());
    if (count <= 0) {
      continue;
    }

    R3D_InstanceBuffer buffer = R3D_LoadInstanceBuffer(
        count, R3D_INSTANCE_POSITION | R3D_INSTANCE_ROTATION | R3D_INSTANCE_SCALE);
    if (buffer.capacity <= 0) {
      TraceLog(LOG_WARNING, "failed to allocate R3D instance buffer for model=%s", build.model_id.c_str());
      continue;
    }

    R3D_UploadInstances(buffer, R3D_INSTANCE_POSITION, 0, count, build.positions.data(), true);
    R3D_UploadInstances(buffer, R3D_INSTANCE_ROTATION, 0, count, build.rotations.data(), true);
    R3D_UploadInstances(buffer, R3D_INSTANCE_SCALE, 0, count, build.scales.data(), true);

    InstanceBatch batch{};
    batch.semantic_key = build.semantic_key;
    batch.model_id = build.model_id;
    batch.buffer = buffer;
    batch.count = count;
    batch.bounds = build.bounds;
    batch.max_lod_distance = LodDistanceForSemantic(build.semantic_key, config.r3d_features);
    scene->slice.instanced_model_instances += count;
    scene->instance_batches.push_back(std::move(batch));
  }

  scene->slice.instance_batches = static_cast<int>(scene->instance_batches.size());
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

void DrawTerrainTile(const SceneData& scene,
                     const PrimitiveMeshes& meshes,
                     const SceneMaterials& materials,
                     int x,
                     int y) {
  const sar::RuntimeCell& cell = scene.level.cells[static_cast<std::size_t>(CellIndex(scene.level, x, y))];
  const Vector3 position = TileToLocalPosition(scene.slice, x, y, ElevationToY(cell.height) - 0.04f);
  if (cell.terrain == sar::TerrainType::kForest &&
      scene.forest_render_mode == ForestRenderMode::kComposition) {
    const ForestTileBand band = ForestBandAt(scene, x, y);
    R3D_DrawMesh(meshes.tile, MaterialForForestBand(materials, band), position, 1.0f);
    if (band == ForestTileBand::kDeep && ShouldDrawDeepCanopyMass(x, y)) {
      R3D_DrawMeshEx(
          meshes.tile,
          materials.forest_canopy_mass,
          Vector3{position.x, position.y + 0.06f, position.z},
          QuaternionIdentity(),
          Vector3{1.02f, 0.42f, 1.02f});
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

void DrawCoarseTerrainChunks(const SceneData& scene,
                             const PrimitiveMeshes& meshes,
                             const SceneMaterials& materials) {
  const int chunk_size = std::max(1, scene.r3d_features.chunk_size_tiles);
  for (int chunk_y = scene.slice.y0; chunk_y < scene.slice.y0 + scene.slice.height; chunk_y += chunk_size) {
    for (int chunk_x = scene.slice.x0; chunk_x < scene.slice.x0 + scene.slice.width; chunk_x += chunk_size) {
      const int x_end = std::min(scene.slice.x0 + scene.slice.width, chunk_x + chunk_size);
      const int y_end = std::min(scene.slice.y0 + scene.slice.height, chunk_y + chunk_size);
      const float width = static_cast<float>(x_end - chunk_x);
      const float height = static_cast<float>(y_end - chunk_y);
      const Vector3 corner = TileToLocalPosition(scene.slice, chunk_x, chunk_y, -0.05f);
      const Vector3 center{
          corner.x + width * 0.5f - 0.5f,
          corner.y,
          corner.z + height * 0.5f - 0.5f,
      };
      const sar::TerrainType dominant = DominantTerrainInChunk(
          scene, chunk_x, chunk_y, x_end, y_end);
      R3D_BeginCluster(MakeTileChunkBounds(scene.slice, chunk_x, chunk_y, x_end, y_end));
      R3D_DrawMeshEx(
          meshes.tile,
          MaterialForTerrain(materials, dominant),
          center,
          QuaternionIdentity(),
          Vector3{width, 1.0f, height});
      R3D_EndCluster();
    }
  }
}

void DrawTerrain(const SceneData& scene,
                 const PrimitiveMeshes& meshes,
                 const SceneMaterials& materials) {
  if (scene.r3d_features.coarse_terrain_chunks) {
    DrawCoarseTerrainChunks(scene, meshes, materials);
    return;
  }

  const int chunk_size = scene.r3d_features.chunks
                             ? std::max(1, scene.r3d_features.chunk_size_tiles)
                             : std::max(scene.slice.width, scene.slice.height);
  for (int chunk_y = scene.slice.y0; chunk_y < scene.slice.y0 + scene.slice.height; chunk_y += chunk_size) {
    for (int chunk_x = scene.slice.x0; chunk_x < scene.slice.x0 + scene.slice.width; chunk_x += chunk_size) {
      const int x_end = std::min(scene.slice.x0 + scene.slice.width, chunk_x + chunk_size);
      const int y_end = std::min(scene.slice.y0 + scene.slice.height, chunk_y + chunk_size);
      if (scene.r3d_features.chunks) {
        R3D_BeginCluster(MakeTileChunkBounds(scene.slice, chunk_x, chunk_y, x_end, y_end));
      }
      for (int y = chunk_y; y < y_end; ++y) {
        for (int x = chunk_x; x < x_end; ++x) {
          DrawTerrainTile(scene, meshes, materials, x, y);
        }
      }
      if (scene.r3d_features.chunks) {
        R3D_EndCluster();
      }
    }
  }
}

bool ShouldHideByLod(std::string_view semantic_key,
                     const R3DFeatureTuning& features,
                     Vector3 camera_target,
                     Vector3 position) {
  if (!features.lod) {
    return false;
  }
  const float max_distance = LodDistanceForSemantic(semantic_key, features);
  return Distance2D(camera_target, position) > max_distance;
}

void DrawPlacements(SceneData* scene,
                    const SpikeConfig& config,
                    const PrimitiveMeshes& meshes,
                    const SceneMaterials& materials,
                    Vector3 camera_target) {
  for (ModelPlacement& placement : scene->placements) {
    if (placement.instanced) {
      continue;
    }
    if (ShouldHideByLod(
            placement.semantic_key, scene->r3d_features, camera_target, placement.position)) {
      ++scene->slice.lod_hidden_instances;
      continue;
    }
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

void DrawInstanceBatches(SceneData* scene,
                         const SpikeConfig& config,
                         Vector3 camera_target) {
  if (!config.r3d_features.instancing) {
    return;
  }

  for (const InstanceBatch& batch : scene->instance_batches) {
    const Vector3 center{
        (batch.bounds.min.x + batch.bounds.max.x) * 0.5f,
        (batch.bounds.min.y + batch.bounds.max.y) * 0.5f,
        (batch.bounds.min.z + batch.bounds.max.z) * 0.5f,
    };
    if (config.r3d_features.lod && Distance2D(camera_target, center) > batch.max_lod_distance) {
      scene->slice.lod_hidden_instances += batch.count;
      continue;
    }

    LoadedModel* loaded = LoadModelIfNeeded(
        &scene->model_cache, scene->registry, config.project_root, batch.model_id);
    if (loaded == nullptr || !loaded->valid) {
      continue;
    }

    if (config.r3d_features.chunks) {
      R3D_BeginCluster(batch.bounds);
    }
    R3D_DrawModelInstanced(loaded->model, batch.buffer, batch.count);
    if (config.r3d_features.chunks) {
      R3D_EndCluster();
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

void DrawBenchmarkShowcase(const SpikeConfig& config,
                           const PrimitiveMeshes& meshes,
                           const SceneMaterials& materials) {
  if (config.r3d_features.extra_lights) {
    R3D_DrawMeshEx(
        meshes.cube,
        materials.pbr_wet_mud,
        Vector3{-7.0f, 0.10f, -7.0f},
        QuaternionIdentity(),
        Vector3{1.2f, 0.20f, 1.2f});
    R3D_DrawMesh(
        meshes.canopy,
        materials.pbr_metal,
        Vector3{-7.0f, 0.85f, -7.0f},
        0.48f);
  }

  if (!config.r3d_features.pbr_probe) {
    return;
  }

  constexpr float kZ = -10.0f;
  R3D_DrawMeshEx(
      meshes.cube,
      materials.pbr_rough_stone,
      Vector3{-7.5f, 0.62f, kZ},
      QuaternionIdentity(),
      Vector3{1.35f, 1.20f, 1.35f});
  R3D_DrawMeshEx(
      meshes.cube,
      materials.pbr_wet_mud,
      Vector3{-4.6f, 0.13f, kZ},
      QuaternionIdentity(),
      Vector3{2.35f, 0.18f, 2.35f});
  R3D_DrawMesh(
      meshes.canopy,
      materials.pbr_metal,
      Vector3{-1.4f, 0.88f, kZ},
      0.90f);
  R3D_DrawMeshEx(
      meshes.tile,
      materials.pbr_glossy_water,
      Vector3{2.4f, 0.02f, kZ},
      QuaternionIdentity(),
      Vector3{2.7f, 1.0f, 2.7f});
}

void DrawScene(SceneData* scene,
               const SpikeConfig& config,
               const PrimitiveMeshes& meshes,
               const SceneMaterials& materials,
               Vector3 camera_target) {
  scene->slice.lod_hidden_instances = 0;
  DrawTerrain(*scene, meshes, materials);
  DrawRuntimeObjects(*scene, meshes, materials);
  DrawBenchmarkShowcase(config, meshes, materials);
  DrawInstanceBatches(scene, config, camera_target);
  DrawPlacements(scene, config, meshes, materials, camera_target);
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
  DrawRectangle(8, 8, 920, 266, Color{0, 0, 0, 178});
  DrawText("ShootAndRun R3D feature benchmark spike", 18, 18, 20, RAYWHITE);
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
  DrawText(
      TextFormat(
          "density edge=%.2f mid=%.2f deep=%.2f jitter=%.2f shadow=%.2f",
          config.forest_tuning.edge_tree_density,
          config.forest_tuning.mid_tree_density,
          config.forest_tuning.deep_tree_density,
          config.forest_tuning.jitter,
          config.forest_tuning.shadow),
      18,
      144,
      14,
      LIGHTGRAY);
  DrawText(
      TextFormat(
          "benchmark=%s profile=%s instancing=%s batches=%d instanced=%d chunks=%s lod-hidden=%d",
          BenchmarkModeName(config.benchmark_mode),
          R3DRenderProfileName(config.r3d_features.profile),
          config.r3d_features.instancing ? "on" : "off",
          scene.slice.instance_batches,
          scene.slice.instanced_model_instances,
          config.r3d_features.chunks ? "on" : "off",
          scene.slice.lod_hidden_instances),
      18,
      164,
      14,
      LIGHTGRAY);
  DrawText(
      TextFormat(
          "shadows=%s fog=%s fog-density=%.3f chunk-size=%d shadow-opacity=%.2f coarse-terrain=%s pbr-probe=%s",
          config.r3d_features.shadows ? "on" : "off",
          config.r3d_features.fog ? "on" : "off",
          config.r3d_features.fog_density,
          config.r3d_features.chunk_size_tiles,
          config.r3d_features.shadow_opacity,
          config.r3d_features.coarse_terrain_chunks ? "on" : "off",
          config.r3d_features.pbr_probe ? "on" : "off"),
      18,
      184,
      14,
      LIGHTGRAY);
  DrawText(TextFormat("map: %s", config.map_package_path.string().c_str()), 18, 204, 14, LIGHTGRAY);
  DrawText("Q/E yaw  R/F pitch  Wheel zoom  Esc exit", 18, 224, 14, LIGHTGRAY);
  DrawText("--benchmark=forest|instancing|culling|lighting|pbr|terrain-mesh", 18, 242, 14, LIGHTGRAY);
}

void ConfigureR3DEnvironment(const SpikeConfig& config, const MapSlice& slice) {
  R3D_Environment* environment = R3D_GetEnvironment();
  environment->background.color = Color{68, 74, 73, 255};
  environment->background.energy = config.r3d_features.atmosphere ? 0.62f : 1.0f;
  environment->ambient.color = Color{58, 70, 61, 255};
  environment->ambient.energy = config.r3d_features.atmosphere ? 0.34f : 0.80f;
  environment->fog.mode = config.r3d_features.fog ? R3D_FOG_EXP2 : R3D_FOG_DISABLED;
  environment->fog.color = Color{57, 69, 61, 255};
  environment->fog.start = 18.0f;
  environment->fog.end = std::max(54.0f, static_cast<float>(slice.width) * 0.72f);
  environment->fog.density = config.r3d_features.fog_density;
  environment->fog.skyAffect = 0.72f;
  environment->ssao.enabled = config.r3d_features.atmosphere;
  environment->ssao.sampleCount = 16;
  environment->ssao.radius = 1.15f;
  environment->ssao.intensity = 0.95f;
  environment->ssao.power = 1.45f;
  environment->tonemap.mode = R3D_TONEMAP_AGX;
  environment->tonemap.exposure = config.r3d_features.atmosphere ? 0.86f : 1.0f;
  environment->color.contrast = config.r3d_features.atmosphere ? 1.08f : 1.0f;
  environment->color.saturation = config.r3d_features.atmosphere ? 0.92f : 1.0f;
}

void ConfigureForestLight(R3D_Light light, const SpikeConfig& config, const MapSlice& slice) {
  R3D_SetLightColor(light, Color{255, 235, 202, 255});
  R3D_SetLightEnergy(light, config.r3d_features.atmosphere ? 1.85f : 1.15f);
  R3D_SetLightSpecular(light, 0.28f);
  R3D_SetLightRange(light, std::max(48.0f, static_cast<float>(slice.width) * 0.92f));
  if (config.r3d_features.shadows) {
    R3D_EnableShadow(light);
    R3D_SetShadowUpdateMode(light, R3D_SHADOW_UPDATE_INTERVAL);
    R3D_SetShadowUpdateFrequency(light, 250);
    R3D_SetShadowSoftness(light, 5.0f);
    R3D_SetShadowOpacity(light, config.r3d_features.shadow_opacity);
    R3D_SetShadowDepthBias(light, 0.006f);
    R3D_SetShadowSlopeBias(light, 0.85f);
    R3D_UpdateShadowMap(light);
  }
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
    const SceneMaterials materials = LoadSceneMaterials(config.forest_tuning);
    SceneData scene = LoadSceneData(config);
    ConfigureR3DEnvironment(config, scene.slice);
    BuildInstanceBatches(&scene, config);

    R3D_Light sun = R3D_CreateLight(R3D_LIGHT_DIR);
    R3D_SetLightDirection(sun, Vector3{-0.52f, -1.0f, -0.34f});
    ConfigureForestLight(sun, config, scene.slice);
    R3D_SetLightActive(sun, true);

    R3D_Light fill = R3D_CreateLight(R3D_LIGHT_DIR);
    R3D_SetLightDirection(fill, Vector3{0.34f, -0.55f, 0.72f});
    R3D_SetLightColor(fill, Color{118, 143, 170, 255});
    R3D_SetLightEnergy(fill, config.r3d_features.atmosphere ? 0.34f : 0.62f);
    R3D_SetLightSpecular(fill, 0.06f);
    R3D_SetLightActive(fill, true);

    R3D_Light campfire = -1;
    R3D_Light ruin_spot = -1;
    if (config.r3d_features.extra_lights) {
      campfire = R3D_CreateLight(R3D_LIGHT_OMNI);
      R3D_SetLightPosition(campfire, Vector3{-7.0f, 1.1f, -7.0f});
      R3D_SetLightColor(campfire, Color{255, 135, 64, 255});
      R3D_SetLightEnergy(campfire, 4.8f);
      R3D_SetLightSpecular(campfire, 0.46f);
      R3D_SetLightRange(campfire, 14.0f);
      R3D_SetLightAttenuation(campfire, 2.0f);
      R3D_SetLightActive(campfire, true);

      ruin_spot = R3D_CreateLight(R3D_LIGHT_SPOT);
      R3D_LightLookAt(ruin_spot, Vector3{8.0f, 6.0f, -8.0f}, Vector3{2.0f, 0.0f, -2.0f});
      R3D_SetLightColor(ruin_spot, Color{164, 190, 255, 255});
      R3D_SetLightEnergy(ruin_spot, 2.2f);
      R3D_SetLightSpecular(ruin_spot, 0.32f);
      R3D_SetLightRange(ruin_spot, 24.0f);
      R3D_SetLightInnerCutOff(ruin_spot, 18.0f);
      R3D_SetLightOuterCutOff(ruin_spot, 32.0f);
      R3D_SetLightActive(ruin_spot, true);
    }

    OrbitCameraState camera_state{};
    camera_state.distance = std::max(30.0f, static_cast<float>(scene.slice.width) * 0.88f);
    Camera3D camera{};

    TraceLog(LOG_INFO, "R3D forest slice initialized");
    TraceLog(LOG_INFO, "map_package=%s", config.map_package_path.string().c_str());
    TraceLog(LOG_INFO, "asset_library=%s", config.asset_library_path.string().c_str());
    TraceLog(LOG_INFO, "tileset=%s", config.tileset_path.string().c_str());
    TraceLog(LOG_INFO, "forest_mode=%s", ForestRenderModeName(config.forest_render_mode));
    TraceLog(LOG_INFO, "benchmark=%s", BenchmarkModeName(config.benchmark_mode));
    TraceLog(LOG_INFO, "r3d_profile=%s instancing=%d shadows=%d fog=%d chunks=%d lod=%d",
             R3DRenderProfileName(config.r3d_features.profile),
             config.r3d_features.instancing ? 1 : 0,
             config.r3d_features.shadows ? 1 : 0,
             config.r3d_features.fog ? 1 : 0,
             config.r3d_features.chunks ? 1 : 0,
             config.r3d_features.lod ? 1 : 0);
    TraceLog(
        LOG_INFO,
        "forest_tuning=edge %.2f mid %.2f deep %.2f jitter %.2f shadow %.2f",
        config.forest_tuning.edge_tree_density,
        config.forest_tuning.mid_tree_density,
        config.forest_tuning.deep_tree_density,
        config.forest_tuning.jitter,
        config.forest_tuning.shadow);

    while (!WindowShouldClose()) {
      UpdateOrbitCamera(&camera_state, &camera);

      BeginDrawing();
      ClearBackground(Color{9, 11, 13, 255});

      R3D_Begin(camera);
      DrawScene(&scene, config, meshes, materials, camera.target);
      R3D_End();

      DrawOverlay(camera_state, config, scene);
      EndDrawing();
    }

    if (campfire >= 0) {
      R3D_DestroyLight(campfire);
    }
    if (ruin_spot >= 0) {
      R3D_DestroyLight(ruin_spot);
    }
    R3D_DestroyLight(fill);
    R3D_DestroyLight(sun);

    UnloadInstanceBatches(&scene);
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
