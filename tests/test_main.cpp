#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "app/project_config.h"
#include "developer/developer_config.h"
#include "level/level_loader.h"
#include "level/terrain_type.h"
#include "ui/main_menu.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/visual_preparation_pipeline.h"
#include "window/window_layout.h"

namespace {

void Expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

void WriteTextFile(const std::filesystem::path& path,
                   std::string_view content) {
  std::ofstream output(path);
  output << content;
}

void TestWindowLayout() {
  const sar::MonitorInfo monitor{0, 0, 1920, 1080};
  const sar::WindowConfig config;
  const sar::WindowState state = sar::CalculateWindowState(monitor, config);

  Expect(state.width == 1600, "window width should use preferred width");
  Expect(state.height == 900, "window height should use preferred height");
  Expect(state.x == 160, "window x should be centered");
  Expect(state.y == 90, "window y should be centered");
  Expect(state.ui_scale == 1.25F,
         "ui scale should grow from the reference size");
}

void TestSmallWindowLayout() {
  const sar::MonitorInfo monitor{0, 0, 1024, 768};
  const sar::WindowConfig config;
  const sar::WindowState state = sar::CalculateWindowState(monitor, config);

  Expect(state.width <= 921, "window width should fit small monitor limit");
  Expect(state.height <= 691, "window height should fit small monitor limit");
  Expect(state.ui_scale >= config.ui_scale_min,
         "ui scale should not go below minimum");
}

void TestMenuNavigationSkipsDisabledItems() {
  sar::MainMenu menu;
  Expect(menu.selected_index() == 0, "default selected item should be New Game");

  menu.SelectNext();
  Expect(menu.selected_index() == 3,
         "next selection should skip disabled Load and Save items");

  menu.SelectNext();
  Expect(menu.selected_index() == 4, "next selection should move to Exit");

  menu.SelectNext();
  Expect(menu.selected_index() == 0, "selection should wrap to New Game");
}

void TestProjectConfigLoader() {
  const std::filesystem::path config_path =
      std::filesystem::temp_directory_path() / "shoot_and_run_test_config.json";

  WriteTextFile(config_path,
                "{\n"
                "  \"map_package_path\": \"data/maps/sample_level\",\n"
                "  \"ui_font_path\": \"data/fonts/test.ttf\",\n"
                "  \"ui_font_size\": 22,\n"
                "  \"raylib_log_level\": \"none\",\n"
                "  \"window\": {\n"
                "    \"preferred_width\": 1700,\n"
                "    \"preferred_height\": 950,\n"
                "    \"fallback_width\": 1200,\n"
                "    \"fallback_height\": 675,\n"
                "    \"max_monitor_fraction\": 0.85,\n"
                "    \"resizable\": false\n"
                "  },\n"
                "  \"service_info\": {\n"
                "    \"enabled\": false,\n"
                "    \"show_memory\": false,\n"
                "    \"update_interval_ms\": 2000\n"
                "  }\n"
                "}\n");

  const sar::ProjectConfigResult result = sar::LoadProjectConfig(config_path);
  Expect(result.ok, "project config should load successfully");
  Expect(result.config.map_package_path == "data/maps/sample_level",
         "map package path should be read from project config");
  Expect(result.config.ui_font_path == "data/fonts/test.ttf",
         "font path should be read from project config");
  Expect(result.config.ui_font_size == 22,
         "font size should be read from project config");
  Expect(result.config.raylib_log_level == sar::RaylibLogLevel::kNone,
         "raylib log level should be read from project config");
  Expect(result.config.window_config.preferred_width == 1700,
         "preferred window width should be read from project config");
  Expect(result.config.window_config.preferred_height == 950,
         "preferred window height should be read from project config");
  Expect(result.config.window_config.fallback_width == 1200,
         "fallback window width should be read from project config");
  Expect(result.config.window_config.fallback_height == 675,
         "fallback window height should be read from project config");
  Expect(result.config.window_config.max_monitor_fraction == 0.85F,
         "window monitor fraction should be read from project config");
  Expect(!result.config.window_config.resizable,
         "window resizable setting should be read from project config");
  Expect(!result.config.service_info.enabled,
         "service info enabled flag should be read from project config");
  Expect(!result.config.service_info.show_memory,
         "service info memory flag should be read from project config");
  Expect(result.config.service_info.update_interval_ms == 2000,
         "service info update interval should be read from project config");

  std::filesystem::remove(config_path);
}


void TestDeveloperConfigLoader() {
  const std::filesystem::path config_path =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_developer_config.json";

  WriteTextFile(config_path,
                "{\n"
                "  \"log\": {\n"
                "    \"enabled\": true,\n"
                "    \"color_enabled\": false,\n"
                "    \"show_execution_context\": false,\n"
                "    \"visual_pipeline_diagnostics\": false,\n"
                "    \"highlight_rules\": [\n"
                "      {\n"
                "        \"name\": \"numbers\",\n"
                "        \"regex\": \"\\\\b[0-9]+\\\\b\",\n"
                "        \"color\": \"orange\",\n"
                "        \"scope\": \"message\",\n"
                "        \"case_sensitive\": true\n"
                "      }\n"
                "    ]\n"
                "  }\n"
                "}\n");

  const sar::DeveloperConfigResult result = sar::LoadDeveloperConfig(
      config_path);
  Expect(result.ok, "developer config should load successfully");
  Expect(result.found, "developer config should report existing file");
  Expect(!result.config.log.color_enabled,
         "developer config should load color flag");
  Expect(!result.config.log.show_execution_context,
         "developer config should load execution context flag");
  Expect(!result.config.log.visual_pipeline_diagnostics,
         "developer config should load pipeline diagnostics flag");
  Expect(result.config.log.highlight_rules.size() == 1,
         "developer config should load highlight rules");
  Expect(result.config.log.highlight_rules[0].color == "orange",
         "developer config should load highlight color");

  std::filesystem::remove(config_path);
}


void TestProjectConfigLoaderUsesFontDefaults() {
  const std::filesystem::path config_path =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_config_defaults.json";

  WriteTextFile(config_path,
                "{\n"
                "  \"map_package_path\": \"data/maps/sample_level\"\n"
                "}\n");

  const sar::ProjectConfigResult result = sar::LoadProjectConfig(config_path);
  Expect(result.ok, "project config should load with default font settings");
  Expect(result.config.ui_font_size == 24,
         "default font size should be available");

  std::filesystem::remove(config_path);
}


void TestVisualPreparationPipelineSkeleton() {
  sar::LevelData level;
  level.size.width = 4;
  level.size.height = 4;
  level.size.tile_size = 16;
  level.cells.resize(16);

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  Expect(pipeline.running(), "visual pipeline should start running");
  Expect(pipeline.progress().total_steps == 10,
         "visual pipeline should expose default step count");

  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  Expect(pipeline.finished(), "visual pipeline should finish successfully");
  Expect(pipeline.prepared_level().ready,
         "prepared level should become ready after pipeline completion");
  Expect(pipeline.prepared_level().semantic_mask_count == 14,
         "prepared level should expose semantic mask count");
  Expect(pipeline.prepared_level().semantic_masks.IsValid(),
         "prepared level should contain valid semantic masks");
  Expect(pipeline.prepared_level().semantic_masks.summary.total_tiles == 16,
         "semantic mask summary should count all tiles");
  Expect(pipeline.prepared_level().terrain_regions.IsValid(),
         "prepared level should contain valid terrain regions");
  Expect(pipeline.prepared_level().terrain_region_count == 1,
         "prepared level should expose one unknown terrain region");
  Expect(pipeline.prepared_level().render_cache_entry_count == 16,
         "prepared level should expose placeholder render cache size");
  Expect(pipeline.last_step_report().step_name == "Build render cache",
         "pipeline should keep the last executed step report");
  Expect(pipeline.last_step_report().success,
         "last pipeline report should mark successful step completion");
  Expect(!pipeline.last_step_report().summaries.empty(),
         "last pipeline report should expose diagnostic summaries");
}


void TestVisualPreparationSemanticMasks() {
  sar::LevelData level;
  level.size.width = 3;
  level.size.height = 2;
  level.size.tile_size = 16;
  level.cells.resize(6);
  level.cells[0].terrain = sar::TerrainType::kForest;
  level.cells[0].collision = true;
  level.cells[0].blocks_vision = true;
  level.cells[1].terrain = sar::TerrainType::kRoad;
  level.cells[1].walkable = true;
  level.cells[2].terrain = sar::TerrainType::kWater;
  level.cells[2].walkable = true;
  level.cells[2].height = -1;
  level.cells[3].terrain = sar::TerrainType::kRuins;
  level.cells[3].cover = 1;
  level.cells[4].terrain = sar::TerrainType::kSwamp;
  level.cells[4].concealment = 1;
  level.cells[5].terrain = sar::TerrainType::kOpenGround;
  level.cells[5].height = 2;

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::PreparedLevel& prepared =
      pipeline.prepared_level();
  Expect(prepared.semantic_masks.IsValid(),
         "semantic masks should be valid for a complete level");
  Expect(prepared.semantic_masks.summary.forest_tiles == 1,
         "semantic masks should count forest tiles");
  Expect(prepared.semantic_masks.summary.road_tiles == 1,
         "semantic masks should count road tiles");
  Expect(prepared.semantic_masks.summary.water_tiles == 1,
         "semantic masks should count water tiles");
  Expect(prepared.semantic_masks.summary.ruins_tiles == 1,
         "semantic masks should count ruins tiles");
  Expect(prepared.semantic_masks.summary.swamp_tiles == 1,
         "semantic masks should count swamp tiles");
  Expect(prepared.semantic_masks.summary.open_ground_tiles == 1,
         "semantic masks should count open ground tiles");
  Expect(prepared.semantic_masks.summary.blocked_tiles == 1,
         "semantic masks should count blocked tiles");
  Expect(prepared.semantic_masks.summary.walkable_tiles == 2,
         "semantic masks should count walkable tiles");
  Expect(prepared.semantic_masks.summary.low_ground_tiles == 1,
         "semantic masks should count negative height tiles");
  Expect(prepared.semantic_masks.summary.elevated_tiles == 1,
         "semantic masks should count elevated tiles");
  Expect(pipeline.last_step_report().success,
         "semantic mask pipeline run should finish with a successful report");
}


void TestVisualPreparationTerrainRegions() {
  sar::LevelData level;
  level.size.width = 4;
  level.size.height = 3;
  level.size.tile_size = 16;
  level.cells.resize(12);

  level.cells[0].terrain = sar::TerrainType::kForest;
  level.cells[1].terrain = sar::TerrainType::kForest;
  level.cells[2].terrain = sar::TerrainType::kRoad;
  level.cells[3].terrain = sar::TerrainType::kOpenGround;
  level.cells[4].terrain = sar::TerrainType::kForest;
  level.cells[5].terrain = sar::TerrainType::kOpenGround;
  level.cells[6].terrain = sar::TerrainType::kRoad;
  level.cells[7].terrain = sar::TerrainType::kOpenGround;
  level.cells[8].terrain = sar::TerrainType::kWater;
  level.cells[9].terrain = sar::TerrainType::kWater;
  level.cells[10].terrain = sar::TerrainType::kRoad;
  level.cells[11].terrain = sar::TerrainType::kRuins;

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::TerrainRegions& regions =
      pipeline.prepared_level().terrain_regions;
  Expect(regions.IsValid(), "terrain regions should be valid");
  Expect(regions.summary.total_regions == 6,
         "terrain region builder should find all connected components");
  Expect(regions.summary.forest_regions == 1,
         "terrain region builder should count forest regions");
  Expect(regions.summary.open_ground_regions == 2,
         "terrain region builder should count disconnected open regions");
  Expect(regions.summary.road_regions == 1,
         "terrain region builder should count road regions");
  Expect(regions.summary.water_regions == 1,
         "terrain region builder should count water patches");
  Expect(regions.summary.ruins_regions == 1,
         "terrain region builder should count ruins clusters");
  Expect(regions.summary.largest_forest_area == 3,
         "terrain region builder should track largest forest area");
  Expect(regions.regions[0].border_tile_count > 0,
         "terrain regions should store border tile counts");
}

void TestLevelLoaderManifestPackage() {
  const std::filesystem::path package_path =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_manifest_level_package";
  std::filesystem::create_directories(package_path / "layers");

  WriteTextFile(package_path / "map.json",
                "{\n"
                "  \"dimensions\": {\n"
                "    \"width_tiles\": 2,\n"
                "    \"height_tiles\": 2,\n"
                "    \"tile_size_px\": 16\n"
                "  },\n"
                "  \"runtime_grids\": \"runtime_grids.json\",\n"
                "  \"markers\": \"markers.json\",\n"
                "  \"layers\": {\n"
                "    \"terrain\": \"layers/terrain.json\"\n"
                "  }\n"
                "}\n");

  WriteTextFile(package_path / "layers" / "terrain.json",
                "{\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"rows\": [\n"
                "    [\"tree_blocker\", \"grass\"],\n"
                "    [\"grass\", \"water_slow\"]\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "runtime_grids.json",
                "{\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"grids\": {\n"
                "    \"movement_grid\": { \"rows\": [[null, 1], [1, 1]] },\n"
                "    \"collision_grid\": { \"rows\": [\"10\", \"00\"] },\n"
                "    \"projectile_block_grid\": { \"rows\": [\"10\", \"00\"] },\n"
                "    \"vision_block_grid\": { \"rows\": [\"10\", \"00\"] },\n"
                "    \"cover_grid\": { \"rows\": [[0, 0], [0, 0]] },\n"
                "    \"concealment_grid\": { \"rows\": [[1, 0], [0, 1]] },\n"
                "    \"height_grid\": { \"rows\": [[0, 0], [0, -1]] }\n"
                "  }\n"
                "}\n");

  WriteTextFile(package_path / "markers.json",
                "{\n"
                "  \"markers\": [\n"
                "    {\n"
                "      \"id\": \"marker_player_spawn_001\",\n"
                "      \"type\": \"player_spawn\",\n"
                "      \"x\": 1,\n"
                "      \"y\": 0,\n"
                "      \"elevation\": 0\n"
                "    }\n"
                "  ]\n"
                "}\n");

  const sar::LevelLoader loader;
  const sar::LevelLoadResult result = loader.LoadBasicPackage(package_path);
  Expect(result.ok, "manifest level package should load successfully");
  Expect(result.summary.size.width == 2,
         "manifest level width should be loaded");
  Expect(result.summary.size.height == 2,
         "manifest level height should be loaded");
  Expect(result.summary.size.tile_size == 16,
         "manifest tile size should be loaded");
  Expect(result.summary.validated_runtime_grid_count == 7,
         "manifest runtime grids should be validated");
  Expect(result.summary.marker_count == 1,
         "manifest markers should be loaded");
  Expect(result.level.markers.size() == 1,
         "manifest marker list should be available");
  Expect(result.level.markers[0].type == "player_spawn",
         "player spawn marker should be parsed");
  Expect(result.level.markers[0].x == 1 && result.level.markers[0].y == 0,
         "player spawn marker coordinates should be parsed");

  WriteTextFile(package_path / "markers.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"start\",\n"
                "      \"type\": \"start\",\n"
                "      \"position\": { \"x\": 0, \"y\": 1 }\n"
                "    }\n"
                "  ]\n"
                "}\n");

  const sar::LevelLoadResult items_result =
      loader.LoadBasicPackage(package_path);
  Expect(items_result.ok, "items markers schema should load successfully");
  Expect(items_result.level.markers.size() == 1,
         "items marker list should be available");
  Expect(items_result.level.markers[0].id == "start",
         "items marker id should be parsed");
  Expect(items_result.level.markers[0].x == 0 &&
             items_result.level.markers[0].y == 1,
         "nested marker position should be parsed");

  Expect(result.level.cells.size() == 4,
         "manifest terrain cells should be loaded");
  Expect(result.level.cells[0].terrain == sar::TerrainType::kForest,
         "tree_blocker should render as forest terrain");
  Expect(result.level.cells[1].terrain == sar::TerrainType::kOpenGround,
         "grass should render as open ground terrain");
  Expect(result.level.cells[3].terrain == sar::TerrainType::kWater,
         "water_slow should render as water terrain");

  std::filesystem::remove_all(package_path);
}

void TestTerrainMapping() {
  Expect(sar::TerrainTypeFromString("forest") == sar::TerrainType::kForest,
         "forest should map to TerrainType::kForest");
  Expect(sar::TerrainTypeFromString("broken") == sar::TerrainType::kUnknown,
         "unknown terrain should map to TerrainType::kUnknown");
  Expect(sar::TerrainTypeToString(sar::TerrainType::kSwamp) == "swamp",
         "swamp enum should map to swamp identifier");
}

void TestLevelLoaderBasicPackage() {
  const std::filesystem::path package_path =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_level_package";
  std::filesystem::create_directories(package_path);

  WriteTextFile(package_path / "terrain.json",
                "{\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"tile_size\": 16,\n"
                "  \"terrain_grid\": [\n"
                "    [\"forest\", \"road\"],\n"
                "    [\"open_ground\", \"swamp\"]\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "runtime_grids.json",
                "{\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"movement_grid\": [[1, 1], [1, 1]],\n"
                "  \"collision_grid\": [[0, 0], [0, 0]],\n"
                "  \"projectile_block_grid\": [[0, 0], [0, 0]],\n"
                "  \"vision_block_grid\": [[0, 0], [0, 0]],\n"
                "  \"cover_grid\": [[0, 0], [0, 0]],\n"
                "  \"concealment_grid\": [[1, 0], [0, 1]],\n"
                "  \"height_grid\": [[0, 0], [0, -1]]\n"
                "}\n");

  const sar::LevelLoader loader;
  const sar::LevelLoadResult result = loader.LoadBasicPackage(package_path);
  Expect(result.ok, "level package should load successfully");
  Expect(result.summary.size.width == 2, "level width should be loaded");
  Expect(result.summary.size.height == 2, "level height should be loaded");
  Expect(result.summary.size.tile_size == 16, "tile size should be loaded");
  Expect(result.summary.validated_runtime_grid_count == 7,
         "all runtime grids should be validated");
  Expect(result.summary.marker_count == 0,
         "basic package without markers should report zero markers");
  Expect(result.level.cells.size() == 4,
         "terrain cells should be loaded for rendering");
  Expect(result.level.cells[0].terrain == sar::TerrainType::kForest,
         "first terrain cell should be forest");
  Expect(result.level.cells[1].terrain == sar::TerrainType::kRoad,
         "second terrain cell should be road");

  std::filesystem::remove_all(package_path);
}

}  // namespace

int main() {
  TestWindowLayout();
  TestSmallWindowLayout();
  TestMenuNavigationSkipsDisabledItems();
  TestTerrainMapping();
  TestProjectConfigLoader();
  TestDeveloperConfigLoader();
  TestProjectConfigLoaderUsesFontDefaults();
  TestVisualPreparationPipelineSkeleton();
  TestVisualPreparationSemanticMasks();
  TestVisualPreparationTerrainRegions();
  TestLevelLoaderBasicPackage();
  TestLevelLoaderManifestPackage();
  std::cout << "All tests passed.\n";
  return 0;
}
