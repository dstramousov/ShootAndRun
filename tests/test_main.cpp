#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "app/project_config.h"
#include "developer/developer_config.h"
#include "level/level_loader.h"
#include "level/terrain_type.h"
#include "render3d/level_3d_player_controller.h"
#include "ui/main_menu.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/visual_preparation_pipeline.h"
#include "visual_pipeline/visual_map_loader.h"
#include "window/window_layout.h"

namespace {

sar::LevelData BuildFlatTestLevel(int width, int height,
                                  const std::vector<std::int8_t>& elevations) {
  sar::LevelData level;
  level.size.width = width;
  level.size.height = height;
  level.size.tile_size = 16;
  level.cells.resize(elevations.size());
  for (std::size_t index = 0; index < elevations.size(); ++index) {
    level.cells[index].terrain = sar::TerrainType::kOpenGround;
    level.cells[index].walkable = true;
    level.cells[index].collision = false;
    level.cells[index].blocks_projectiles = false;
    level.cells[index].blocks_vision = false;
    level.cells[index].cover = 0;
    level.cells[index].concealment = 0;
    level.cells[index].height = elevations[index];
    level.cells[index].movement_multiplier = 1.0F;
  }
  return level;
}

sar::render3d::Level3DPlayerState MakeTestPlayer(float tile_x, float tile_y,
                                                 std::int8_t elevation,
                                                 float facing_x,
                                                 float facing_y) {
  sar::render3d::Level3DPlayerState state;
  state.tile_x = tile_x;
  state.tile_y = tile_y;
  state.elevation = elevation;
  state.facing_x = facing_x;
  state.facing_y = facing_y;
  state.max_hp = 100;
  state.current_hp = 100;
  state.fall_damage_per_level = 5;
  state.allowed_step_down_height = 1;
  state.current_movement_multiplier = 1.0F;
  state.target_movement_multiplier = 1.0F;
  state.effective_move_speed_tiles_per_sec = state.move_speed_tiles_per_sec;
  state.initialized = true;
  return state;
}

void Run3DPlayerFrames(const sar::LevelData& level, const sar::InputState& input,
                       int frame_count,
                       sar::render3d::Level3DPlayerState* state) {
  for (int frame = 0; frame < frame_count; ++frame) {
    sar::render3d::UpdateLevel3DPlayer(level, input, 0.05F, state);
  }
}

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


void TestWindowLayoutScalesPreferredToMonitorLimit() {
  const sar::MonitorInfo monitor{0, 0, 1920, 1080};
  sar::WindowConfig config;
  config.preferred_width = 1920;
  config.preferred_height = 1080;
  config.fallback_width = 1280;
  config.fallback_height = 720;
  config.max_monitor_fraction = 0.90F;

  const sar::WindowState state = sar::CalculateWindowState(monitor, config);

  Expect(state.width == 1728,
         "preferred width should be scaled down to monitor limit");
  Expect(state.height == 972,
         "preferred height should be scaled down to monitor limit");
  Expect(state.x == 96, "scaled window x should keep a visible border");
  Expect(state.y == 54, "scaled window y should keep a visible border");
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
                "  },\n"
                "  \"player3d_health\": {\n"
                "    \"player3d_initial_hp\": 80,\n"
                "    \"player3d_max_hp\": 120,\n"
                "    \"player3d_fall_damage_per_level\": 7\n"
                "  },\n"
                "  \"visual_pipeline\": {\n"
                "    \"visual_pipeline_mode\": \"compare\",\n"
                "    \"prepared_visual_map_path\": \"../visual_map/visual_map.json\",\n"
                "    \"fallback_to_cpp_pipeline\": false,\n"
                "    \"run_cpp_analysis\": true\n"
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
  Expect(result.config.player3d_health.initial_hp == 80,
         "3D player initial HP should be read from project config");
  Expect(result.config.player3d_health.max_hp == 120,
         "3D player max HP should be read from project config");
  Expect(result.config.player3d_health.fall_damage_per_level == 7,
         "3D player fall damage should be read from project config");
  Expect(result.config.visual_pipeline_config.mode ==
             sar::visual_pipeline::VisualPipelineMode::kCompare,
         "visual pipeline mode should be read from project config");
  Expect(result.config.visual_pipeline_config.prepared_visual_map_path ==
             "../visual_map/visual_map.json",
         "prepared visual map path should be read from project config");
  Expect(!result.config.visual_pipeline_config.fallback_to_cpp_pipeline,
         "visual pipeline fallback flag should be read from project config");
  Expect(result.config.visual_pipeline_config.run_cpp_analysis,
         "visual pipeline analysis flag should be read from project config");

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
                "    \"visual_pipeline_summary\": false,\n"
                "    \"visual_pipeline_step_details\": true,\n"
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
  Expect(!result.config.log.visual_pipeline_summary,
         "developer config should load pipeline summary flag");
  Expect(result.config.log.visual_pipeline_step_details,
         "developer config should load pipeline step details flag");
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



void WriteMinimalVisualMap(const std::filesystem::path& visual_map_dir) {
  std::filesystem::create_directories(visual_map_dir);

  WriteTextFile(visual_map_dir / "visual_map.json",
                "{\n"
                "  \"schema_version\": \"visual-map-v1\",\n"
                "  \"visual_generator_version\": \"0.0.test\",\n"
                "  \"visual_profile\": { \"id\": \"dark_forest\", \"name\": \"Dark Forest\" },\n"
                "  \"dimensions\": {\n"
                "    \"width_tiles\": 2,\n"
                "    \"height_tiles\": 2,\n"
                "    \"tile_size_px\": 16\n"
                "  },\n"
                "  \"files\": {\n"
                "    \"visual_layers\": \"visual_layers.json\",\n"
                "    \"visual_objects\": \"visual_objects.json\",\n"
                "    \"visual_chunks\": \"visual_chunks.json\"\n"
                "  },\n"
                "  \"contract\": {\n"
                "    \"changes_gameplay\": false,\n"
                "    \"moves_markers\": false,\n"
                "    \"changes_collision\": false\n"
                "  }\n"
                "}\n");

  WriteTextFile(visual_map_dir / "visual_layers.json",
                "{\n"
                "  \"schema_version\": \"visual-layers-v1\",\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"tile_size_px\": 16,\n"
                "  \"layers\": [\n"
                "    {\n"
                "      \"id\": \"terrain_base\",\n"
                "      \"rows\": [[\"grass.base\", \"forest.edge_n\"], [\"road.end_e\", \"swamp.fill\"]],\n"
                "      \"summary\": { \"unique_tile_ids\": 4 }\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(visual_map_dir / "visual_objects.json",
                "{\n"
                "  \"schema_version\": \"visual-objects-v2\",\n"
                "  \"items\": [\n"
                "    { \"id\": \"visual_decor_000\", \"sprite_id\": \"decor.stone\" },\n"
                "    { \"id\": \"visual_decor_001\", \"sprite_id\": \"decor.reeds\" }\n"
                "  ],\n"
                "  \"summary\": { \"total\": 2 }\n"
                "}\n");

  WriteTextFile(visual_map_dir / "visual_chunks.json",
                "{\n"
                "  \"schema_version\": \"visual-chunks-v1\",\n"
                "  \"chunk_size_tiles\": 32,\n"
                "  \"items\": [\n"
                "    { \"id\": \"chunk_000_000\", \"object_count\": 2 }\n"
                "  ],\n"
                "  \"summary\": { \"total\": 1 }\n"
                "}\n");
}

void TestVisualMapLoader() {
  const std::filesystem::path visual_map_dir =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_visual_map_loader";
  WriteMinimalVisualMap(visual_map_dir);

  sar::visual_pipeline::VisualMapLoader loader;
  const sar::LevelSize raw_size{2, 2, 16};
  const sar::visual_pipeline::VisualMapLoadResult result =
      loader.Load(visual_map_dir / "visual_map.json", raw_size);

  Expect(result.ok, "prepared visual_map should load successfully");
  Expect(result.found, "prepared visual_map should be reported as found");
  Expect(result.data.loaded, "visual map data should be marked loaded");
  Expect(result.data.size.width == 2 && result.data.size.height == 2,
         "visual map dimensions should be loaded");
  Expect(result.data.visual_layer_count == 1,
         "visual map layer count should be loaded");
  Expect(result.data.unique_tile_id_count == 4,
         "visual map unique tile count should be loaded");
  Expect(result.data.visual_object_count == 2,
         "visual map object count should be loaded");
  Expect(result.data.visual_chunk_count == 1,
         "visual map chunk count should be loaded");
  Expect(!result.data.changes_gameplay,
         "visual map contract should not change gameplay");

  std::filesystem::remove_all(visual_map_dir);
}

void TestVisualPreparationPipelineUsesPreparedVisualMap() {
  const std::filesystem::path temp_root =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_prepared_visual_pipeline";
  const std::filesystem::path map_package = temp_root / "map_package";
  const std::filesystem::path visual_map_dir = temp_root / "visual_map";
  std::filesystem::create_directories(map_package);
  WriteMinimalVisualMap(visual_map_dir);

  sar::LevelData level;
  level.size.width = 2;
  level.size.height = 2;
  level.size.tile_size = 16;
  level.cells.resize(4);

  sar::visual_pipeline::VisualPreparationOptions options;
  options.map_package_path = map_package;
  options.visual_pipeline_config.mode =
      sar::visual_pipeline::VisualPipelineMode::kUsePreparedVisualMap;
  options.visual_pipeline_config.prepared_visual_map_path =
      "../visual_map/visual_map.json";
  options.visual_pipeline_config.fallback_to_cpp_pipeline = false;
  options.visual_pipeline_config.run_cpp_analysis = true;

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level, options);
  Expect(pipeline.progress().total_steps == 13,
         "prepared visual-map pipeline should add one loading step");

  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::PreparedLevel& prepared =
      pipeline.prepared_level();
  Expect(pipeline.finished(),
         "prepared visual-map pipeline should finish successfully");
  Expect(prepared.prepared_visual_map.loaded,
         "prepared level should keep loaded visual_map data");
  Expect(prepared.source ==
             sar::visual_pipeline::PreparedLevelSource::kPreparedVisualMap,
         "prepared level source should be prepared_visual_map");
  Expect(prepared.visual_layer_count == 1,
         "prepared level should expose visual map layer count");
  Expect(prepared.decoration_count == 2,
         "prepared level should expose visual object count");
  Expect(prepared.render_cache_entry_count == 1,
         "prepared level should expose visual chunk count");
  Expect(prepared.semantic_masks.IsValid(),
         "prepared visual-map mode should still keep C++ analysis when enabled");

  std::filesystem::remove_all(temp_root);
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
  Expect(pipeline.progress().total_steps == 12,
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
  Expect(pipeline.prepared_level().region_borders.IsValid(),
         "prepared level should contain valid region borders");
  Expect(pipeline.prepared_level().region_border_count == 1,
         "prepared level should expose one unknown region border");
  Expect(pipeline.prepared_level().ruin_visual_plan.IsValid(),
         "prepared level should contain valid ruin visual plan");
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

void TestVisualPreparationRegionBorders() {
  sar::LevelData level;
  level.size.width = 3;
  level.size.height = 3;
  level.size.tile_size = 16;
  level.cells.resize(9);

  level.cells[0].terrain = sar::TerrainType::kForest;
  level.cells[1].terrain = sar::TerrainType::kForest;
  level.cells[2].terrain = sar::TerrainType::kOpenGround;
  level.cells[3].terrain = sar::TerrainType::kForest;
  level.cells[4].terrain = sar::TerrainType::kForest;
  level.cells[5].terrain = sar::TerrainType::kRoad;
  level.cells[6].terrain = sar::TerrainType::kWater;
  level.cells[7].terrain = sar::TerrainType::kRoad;
  level.cells[8].terrain = sar::TerrainType::kRoad;

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::RegionBorders& borders =
      pipeline.prepared_level().region_borders;
  Expect(borders.IsValid(), "region borders should be valid");
  Expect(borders.summary.region_count ==
             pipeline.prepared_level().terrain_region_count,
         "region border count should match terrain region count");
  Expect(borders.summary.border_tile_count > 0,
         "region borders should count border tiles");
  Expect(borders.summary.corner_tile_count > 0,
         "region borders should classify corner tiles");
  Expect(borders.summary.neighbor_outside_map > 0,
         "region borders should count outside-map neighbors");
  Expect(!borders.regions.empty() && !borders.regions[0].tiles.empty(),
         "region border info should retain tile classification data");
}

void TestLevelLoaderManifestPackage() {
  const std::filesystem::path package_path =
      std::filesystem::temp_directory_path() /
      "shoot_and_run_test_manifest_level_package";
  std::filesystem::create_directories(package_path / "layers");
  std::filesystem::create_directories(package_path / "catalogs");

  WriteTextFile(package_path / "map.json",
                "{\n"
                "  \"dimensions\": {\n"
                "    \"width_tiles\": 2,\n"
                "    \"height_tiles\": 2,\n"
                "    \"tile_size_px\": 16\n"
                "  },\n"
                "  \"runtime_grids\": \"runtime_grids.json\",\n"
                "  \"markers\": \"markers.json\",\n"
                "  \"routes\": \"routes.json\",\n"
                "  \"world_graph\": \"world_graph.json\",\n"
                "  \"gameplay_zones\": \"gameplay_zones.json\",\n"
                "  \"elevation_transitions\": \"elevation_transitions.json\",\n"
                "  \"objects\": {\n"
                "    \"runtime_objects\": \"objects/runtime_objects.json\",\n"
                "    \"places\": \"objects/places.json\"\n"
                "  },\n"
                "  \"layers\": {\n"
                "    \"terrain\": \"layers/terrain.json\"\n"
                "  },\n"
                "  \"catalogs\": {\n"
                "    \"tile_types\": \"catalogs/tile_types.json\"\n"
                "  }\n"
                "}\n");

  WriteTextFile(package_path / "layers" / "terrain.json",
                "{\n"
                "  \"width\": 2,\n"
                "  \"height\": 2,\n"
                "  \"rows\": [\n"
                "    [\"tree_blocker\", \"old_overgrown_road\"],\n"
                "    [\"ruin_wall_blocker\", \"water_slow\"]\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "catalogs" / "tile_types.json",
                "{\n"
                "  \"types\": {\n"
                "    \"tree_blocker\": { \"collision\": \"blocked\", \"tags\": [\"blocker\", \"vegetation\"] },\n"
                "    \"old_overgrown_road\": { \"collision\": \"passable\", \"tags\": [\"road\"] },\n"
                "    \"ruin_wall_blocker\": { \"collision\": \"blocked\", \"tags\": [\"blocker\", \"ruin\"] },\n"
                "    \"water_slow\": { \"collision\": \"passable\", \"tags\": [\"slow\", \"water\"] }\n"
                "  }\n"
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


  std::filesystem::create_directories(package_path / "objects");
  WriteTextFile(package_path / "objects" / "runtime_objects.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"fallen_log_001\",\n"
                "      \"type\": \"fallen_log\",\n"
                "      \"family\": \"forest_debris\",\n"
                "      \"x\": 1,\n"
                "      \"y\": 1,\n"
                "      \"visual_bounds\": {\"x\": 1, \"y\": 1, \"width\": 1, \"height\": 1},\n"
                "      \"blocks_movement\": false,\n"
                "      \"blocks_projectiles\": false,\n"
                "      \"blocks_vision\": false,\n"
                "      \"tags\": [\"decor\", \"forest\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "objects" / "places.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"camp_001\",\n"
                "      \"type\": \"camp\",\n"
                "      \"center\": {\"x\": 1, \"y\": 1},\n"
                "      \"radius\": 2,\n"
                "      \"tags\": [\"camp\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "routes.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"main_road_000\",\n"
                "      \"type\": \"main_road\",\n"
                "      \"waypoints\": [{\"x\": 0, \"y\": 0}, {\"x\": 1, \"y\": 1}],\n"
                "      \"tags\": [\"primary\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "world_graph.json",
                "{\n"
                "  \"nodes\": [\n"
                "    {\"id\": \"start\", \"type\": \"start\", \"position\": {\"x\": 0, \"y\": 0}},\n"
                "    {\"id\": \"camp_001\", \"type\": \"camp\", \"position\": {\"x\": 1, \"y\": 1}}\n"
                "  ],\n"
                "  \"edges\": [\n"
                "    {\"source\": \"start\", \"target\": \"camp_001\", \"type\": \"main_path\", \"cost_tiles\": 2}\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "gameplay_zones.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"zone_safe_001\",\n"
                "      \"type\": \"safe_area\",\n"
                "      \"bounds\": {\"min_x\": 0, \"min_y\": 0, \"max_x\": 1, \"max_y\": 1},\n"
                "      \"tags\": [\"safe\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "elevation_transitions.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"generated_step_down\",\n"
                "      \"type\": \"step_down\",\n"
                "      \"from\": { \"x\": 1, \"y\": 0, \"level\": 0 },\n"
                "      \"to\": { \"x\": 1, \"y\": 1, \"level\": -1 }\n"
                "    },\n"
                "    {\n"
                "      \"id\": \"generated_connector_ramp\",\n"
                "      \"type\": \"connector_edge\",\n"
                "      \"suggested_connector\": \"ramp\",\n"
                "      \"from\": { \"x\": 0, \"y\": 0, \"level\": 0 },\n"
                "      \"to\": { \"x\": 1, \"y\": 0, \"level\": 0 }\n"
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
  Expect(result.summary.object_count == 1,
         "manifest runtime objects should be loaded");
  Expect(result.summary.place_count == 1,
         "manifest places should be loaded");
  Expect(result.summary.route_count == 1,
         "manifest routes should be loaded");
  Expect(result.summary.graph_node_count == 2,
         "manifest graph nodes should be loaded");
  Expect(result.summary.graph_edge_count == 1,
         "manifest graph edges should be loaded");
  Expect(result.summary.gameplay_zone_count == 1,
         "manifest gameplay zones should be loaded");
  Expect(result.summary.elevation_transition_count == 2,
         "manifest elevation transitions should be loaded");
  Expect(result.summary.validation_report.transition_endpoint_mismatch_count ==
             0,
         "nested endpoint elevation levels should match height grid");
  Expect(result.summary.validation_report.transition_histogram.at("step") == 1,
         "step_down transition should map to step");
  Expect(result.summary.validation_report.transition_histogram.at("ramp") == 1,
         "connector_edge ramp suggestion should map to ramp");
  Expect(result.level.markers.size() == 1,
         "manifest marker list should be available");
  Expect(result.level.markers[0].type == "player_spawn",
         "player spawn marker should be parsed");
  Expect(result.level.markers[0].x == 1 && result.level.markers[0].y == 0,
         "player spawn marker coordinates should be parsed");
  Expect(result.level.markers[0].has_elevation,
         "explicit marker elevation should be tracked");
  Expect(result.level.objects.size() == 1 &&
             result.level.objects[0].type == "fallen_log",
         "runtime object should be parsed");
  Expect(result.level.places.size() == 1 && result.level.places[0].x == 1 &&
             result.level.places[0].y == 1,
         "place center should be parsed");
  Expect(result.level.routes.size() == 1 &&
             result.level.routes[0].waypoints.size() == 2,
         "route waypoints should be parsed");
  Expect(result.level.world_graph.nodes.size() == 2 &&
             result.level.world_graph.edges.size() == 1,
         "world graph should be parsed");
  Expect(result.level.zones.size() == 1 && result.level.zones[0].width == 2,
         "gameplay zone bounds should be parsed");

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
  Expect(items_result.level.markers[0].x == 1 &&
             items_result.level.markers[0].y == 1,
         "nested marker position should be parsed");
  Expect(!items_result.level.markers[0].has_elevation,
         "missing marker elevation should not be treated as declared zero");

  Expect(result.level.cells.size() == 4,
         "manifest terrain cells should be loaded");
  Expect(result.level.cells[0].terrain == sar::TerrainType::kForest,
         "tree_blocker should render as forest terrain");
  Expect(result.level.cells[1].terrain == sar::TerrainType::kRoad,
         "catalog road tag should render as road terrain");
  Expect(result.level.cells[2].terrain == sar::TerrainType::kWall,
         "catalog blocked ruin tag should render as wall terrain");
  Expect(result.level.cells[3].terrain == sar::TerrainType::kWater,
         "catalog water tag should render as water terrain");
  Expect(result.level.used_tile_catalog,
         "manifest package should use tile type catalog");
  Expect(result.level.tile_catalog_type_count == 4,
         "tile catalog type count should be stored");
  Expect(result.level.terrain_type_counts.at("old_overgrown_road") == 1,
         "raw terrain type counts should be stored");
  Expect(result.level.unknown_terrain_type_counts.empty(),
         "catalog-aware terrain mapping should avoid unknown terrain types");
  Expect(!result.level.cells[0].walkable && result.level.cells[0].collision,
         "runtime grids should populate blocked tree cell data");
  Expect(result.level.cells[1].walkable && !result.level.cells[1].collision,
         "runtime grids should populate walkable road cell data");
  Expect(result.level.cells[3].height == -1,
         "runtime height grid should populate negative height");

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

void TestRuinVisualPlanBuildsSceneComposition() {
  sar::LevelData level;
  level.size.width = 6;
  level.size.height = 5;
  level.size.tile_size = 16;
  level.cells.resize(30);
  for (sar::RuntimeCell& cell : level.cells) {
    cell.terrain = sar::TerrainType::kOpenGround;
    cell.walkable = true;
  }

  const auto set_terrain = [&level](int x, int y, sar::TerrainType terrain) {
    level.cells[static_cast<std::size_t>(y * level.size.width + x)].terrain =
        terrain;
  };
  set_terrain(1, 1, sar::TerrainType::kWall);
  set_terrain(2, 1, sar::TerrainType::kWall);
  set_terrain(3, 1, sar::TerrainType::kWall);
  set_terrain(1, 2, sar::TerrainType::kWall);
  set_terrain(2, 2, sar::TerrainType::kRuins);
  set_terrain(3, 2, sar::TerrainType::kRoad);

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::RuinVisualPlan& ruin_plan =
      pipeline.prepared_level().ruin_visual_plan;
  Expect(ruin_plan.IsValid(), "ruin visual plan should be valid");
  Expect(ruin_plan.summary.site_count == 1,
         "ruin visual plan should group adjacent ruin cells into one site");
  Expect(ruin_plan.summary.source_wall_tiles == 4,
         "ruin visual plan should count source wall tiles");
  Expect(ruin_plan.summary.source_ruin_tiles == 1,
         "ruin visual plan should count source ruin floor tiles");
  Expect(ruin_plan.summary.wall_corner_tiles > 0 ||
             ruin_plan.summary.wall_endcap_tiles > 0,
         "ruin visual plan should classify wall details");
  Expect(ruin_plan.summary.rubble_tiles > 0 ||
             ruin_plan.summary.entrance_tiles > 0,
         "ruin visual plan should add local scene dressing");
}

void TestRoadVisualPlanBuildsTerrainRoadDressing() {
  sar::LevelData level;
  level.size.width = 12;
  level.size.height = 9;
  level.size.tile_size = 16;
  level.cells.resize(108);
  for (sar::RuntimeCell& cell : level.cells) {
    cell.terrain = sar::TerrainType::kOpenGround;
    cell.walkable = true;
  }
  for (int x = 1; x <= 9; ++x) {
    const int road_index = 4 * level.size.width + x;
    level.cells[static_cast<std::size_t>(road_index)].terrain =
        sar::TerrainType::kRoad;
  }

  const int ruin_index = 4 * level.size.width + 10;
  level.cells[static_cast<std::size_t>(ruin_index)].terrain =
      sar::TerrainType::kRuins;

  sar::Route route;
  route.id = "route_main_test";
  route.type = "main_path";
  route.tags.push_back("critical");
  route.waypoints.push_back(sar::RoutePoint{1, 4});
  route.waypoints.push_back(sar::RoutePoint{10, 4});
  level.routes.push_back(route);

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::RoadVisualPlan& road_plan =
      pipeline.prepared_level().road_visual_plan;
  Expect(road_plan.IsValid(), "road visual plan should be valid");
  Expect(road_plan.summary.route_count == 1,
         "road visual plan should count routes");
  Expect(road_plan.summary.main_route_count == 1,
         "road visual plan should classify main route");
  Expect(!road_plan.summary.routes_used_for_visual_roads,
         "road visual plan should not paint routes as roads");
  Expect(road_plan.summary.terrain_road_tiles == 9,
         "road visual plan should count terrain road cells");
  Expect(road_plan.summary.road_core_tiles > 0,
         "road visual plan should build road core band");
  Expect(road_plan.summary.road_side_tiles > 0,
         "road visual plan should build road side band");
  Expect(road_plan.summary.trampled_grass_tiles > 0,
         "road visual plan should build trampled grass band");
  Expect(road_plan.summary.ruin_approach_tiles > 0,
         "road visual plan should mark ruin approach tiles");
}


void TestObjectVisualPlanRemovesGenericObjects() {
  sar::LevelData level;
  level.size.width = 8;
  level.size.height = 6;
  level.size.tile_size = 16;
  level.cells.resize(48);
  for (sar::RuntimeCell& cell : level.cells) {
    cell.terrain = sar::TerrainType::kOpenGround;
    cell.walkable = true;
  }

  sar::RuntimeObject known;
  known.id = "fallen_log_001";
  known.type = "fallen_log";
  known.family = "forest_debris";
  known.x = 1;
  known.y = 1;
  known.width = 2;
  known.height = 1;
  known.tags.push_back("forest");
  level.objects.push_back(known);

  sar::RuntimeObject fallback;
  fallback.id = "unknown_stone_001";
  fallback.type = "unknown_object";
  fallback.family = "stone_cover";
  fallback.x = 4;
  fallback.y = 2;
  fallback.width = 1;
  fallback.height = 1;
  fallback.blocks_movement = true;
  fallback.tags.push_back("cover");
  level.objects.push_back(fallback);

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::ObjectVisualPlan& object_plan =
      pipeline.prepared_level().object_visual_plan;
  Expect(object_plan.IsValid(), "object visual plan should be valid");
  Expect(object_plan.summary.source_object_count == 2,
         "object visual plan should count source objects");
  Expect(object_plan.summary.mapped_object_count == 2,
         "object visual plan should map all valid runtime objects");
  Expect(object_plan.summary.generic_object_count == 0,
         "object visual plan should not emit object.generic");
  Expect(object_plan.summary.missing_sprite_uses == 0,
         "object visual plan should not report missing sprite uses");
  Expect(object_plan.summary.typed_fallback_count == 1,
         "object visual plan should use typed fallback for unknown objects");
  Expect(object_plan.items[0].sprite_family == "object.fallen_log",
         "known object should use explicit sprite family");
  Expect(object_plan.items[1].sprite_family != "object.generic",
         "fallback object should not use generic sprite family");
}


void TestMicroSceneVisualPlanBuildsDressing() {
  sar::LevelData level;
  level.size.width = 8;
  level.size.height = 8;
  level.size.tile_size = 16;
  level.cells.resize(64);
  for (sar::RuntimeCell& cell : level.cells) {
    cell.terrain = sar::TerrainType::kOpenGround;
    cell.walkable = true;
  }

  auto set_terrain = [&level](int x, int y, sar::TerrainType terrain) {
    level.cells[static_cast<std::size_t>(y * level.size.width + x)].terrain =
        terrain;
  };
  for (int x = 1; x <= 5; ++x) {
    set_terrain(x, 2, sar::TerrainType::kRoad);
  }
  set_terrain(4, 4, sar::TerrainType::kWater);
  set_terrain(5, 4, sar::TerrainType::kWater);
  set_terrain(2, 5, sar::TerrainType::kRuins);
  set_terrain(2, 6, sar::TerrainType::kWall);

  sar::RuntimeObject camp;
  camp.id = "campfire_001";
  camp.type = "dead_campfire";
  camp.family = "camp";
  camp.x = 1;
  camp.y = 1;
  camp.width = 1;
  camp.height = 1;
  level.objects.push_back(camp);

  sar::RuntimeObject log;
  log.id = "fallen_log_001";
  log.type = "fallen_log";
  log.family = "forest_debris";
  log.x = 5;
  log.y = 5;
  log.width = 2;
  log.height = 1;
  level.objects.push_back(log);

  sar::visual_pipeline::VisualPreparationPipeline pipeline;
  pipeline.Start(level);
  while (!pipeline.finished() && !pipeline.progress().failed) {
    pipeline.AdvanceOneStep(level);
  }

  const sar::visual_pipeline::MicroSceneVisualPlan& plan =
      pipeline.prepared_level().micro_scene_visual_plan;
  Expect(plan.IsValid(), "micro-scene visual plan should be valid");
  Expect(plan.summary.scene_count > 0,
         "micro-scene visual plan should place scenes");
  Expect(plan.summary.visual_tiles > 0,
         "micro-scene visual plan should paint dressing tiles");
  Expect(plan.summary.camp_scene_count > 0,
         "micro-scene visual plan should detect camp scenes");
  Expect(plan.summary.logging_spot_count > 0,
         "micro-scene visual plan should detect logging spots");
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


  std::filesystem::create_directories(package_path / "objects");
  WriteTextFile(package_path / "objects" / "runtime_objects.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"fallen_log_001\",\n"
                "      \"type\": \"fallen_log\",\n"
                "      \"family\": \"forest_debris\",\n"
                "      \"x\": 1,\n"
                "      \"y\": 1,\n"
                "      \"visual_bounds\": {\"x\": 1, \"y\": 1, \"width\": 1, \"height\": 1},\n"
                "      \"blocks_movement\": false,\n"
                "      \"blocks_projectiles\": false,\n"
                "      \"blocks_vision\": false,\n"
                "      \"tags\": [\"decor\", \"forest\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "objects" / "places.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"camp_001\",\n"
                "      \"type\": \"camp\",\n"
                "      \"center\": {\"x\": 1, \"y\": 1},\n"
                "      \"radius\": 2,\n"
                "      \"tags\": [\"camp\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "routes.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"main_road_000\",\n"
                "      \"type\": \"main_road\",\n"
                "      \"waypoints\": [{\"x\": 0, \"y\": 0}, {\"x\": 1, \"y\": 1}],\n"
                "      \"tags\": [\"primary\"]\n"
                "    }\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "world_graph.json",
                "{\n"
                "  \"nodes\": [\n"
                "    {\"id\": \"start\", \"type\": \"start\", \"position\": {\"x\": 0, \"y\": 0}},\n"
                "    {\"id\": \"camp_001\", \"type\": \"camp\", \"position\": {\"x\": 1, \"y\": 1}}\n"
                "  ],\n"
                "  \"edges\": [\n"
                "    {\"source\": \"start\", \"target\": \"camp_001\", \"type\": \"main_path\", \"cost_tiles\": 2}\n"
                "  ]\n"
                "}\n");

  WriteTextFile(package_path / "gameplay_zones.json",
                "{\n"
                "  \"items\": [\n"
                "    {\n"
                "      \"id\": \"zone_safe_001\",\n"
                "      \"type\": \"safe_area\",\n"
                "      \"bounds\": {\"min_x\": 0, \"min_y\": 0, \"max_x\": 1, \"max_y\": 1},\n"
                "      \"tags\": [\"safe\"]\n"
                "    }\n"
                "  ]\n"
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
  Expect(result.level.cells[0].walkable,
         "movement grid should populate walkable cells");
  Expect(result.level.cells[3].concealment == 1,
         "concealment grid should populate cell concealment");
  Expect(result.level.cells[3].height == -1,
         "height grid should populate negative height");
  Expect(result.summary.validation_report.total_tiles == 4,
         "validation report should count all loaded tiles");
  Expect(result.summary.validation_report.elevation_histogram.at(-1) == 1,
         "validation report should count negative elevation tiles");
  Expect(result.summary.validation_report.elevation_histogram.at(0) == 3,
         "validation report should count base elevation tiles");
  Expect(result.summary.validation_report.open_negative_region_count == 1,
         "validation report should detect open negative pit regions");
  Expect(result.summary.validation_report.closed_negative_region_count == 0,
         "validation report should not mark open pits as closed bunkers");

  std::filesystem::remove_all(package_path);
}

void TestLevel3DPlayerFallsIntoNegativePitWithoutDamage() {
  const sar::LevelData level = BuildFlatTestLevel(2, 1, {0, -1});
  sar::render3d::Level3DPlayerState state = MakeTestPlayer(
      0.5F, 0.5F, 0, 1.0F, 0.0F);
  sar::InputState input;
  input.up_down = true;

  Run3DPlayerFrames(level, input, 10, &state);

  Expect(state.tile_x >= 1.0F,
         "3D player should enter an open -1 pit from surface elevation");
  Expect(state.elevation == -1,
         "3D player elevation should become -1 after falling into the pit");
  Expect(state.current_hp == 100,
         "0 to -1 pit fall should not apply HP damage");
  Expect(state.last_fall_damage == 0,
         "one-level pit fall should keep last fall damage at zero");
}

void TestLevel3DPlayerDropIntoNegativePitUsesFallDamageFormula() {
  const sar::LevelData level = BuildFlatTestLevel(2, 1, {1, -1});
  sar::render3d::Level3DPlayerState state = MakeTestPlayer(
      0.5F, 0.5F, 1, 1.0F, 0.0F);
  sar::InputState input;
  input.up_down = true;

  Run3DPlayerFrames(level, input, 10, &state);

  Expect(state.tile_x >= 1.0F,
         "3D player should enter a -1 pit from higher elevation");
  Expect(state.elevation == -1,
         "3D player elevation should become -1 after a higher pit fall");
  Expect(state.last_fall_drop_levels == 2,
         "1 to -1 pit fall should be counted as a two-level drop");
  Expect(state.last_fall_damage == 5,
         "two-level pit fall should use one unsafe fall-damage level");
  Expect(state.current_hp == 95,
         "two-level pit fall should subtract 5 HP by default");
}

void TestLevel3DPlayerCannotWalkOutOfNegativePit() {
  const sar::LevelData level = BuildFlatTestLevel(2, 1, {0, -1});
  sar::render3d::Level3DPlayerState state = MakeTestPlayer(
      1.5F, 0.5F, -1, -1.0F, 0.0F);
  sar::InputState input;
  input.up_down = true;

  Run3DPlayerFrames(level, input, 10, &state);

  Expect(state.tile_x > 1.0F,
         "3D player should not walk out of -1 pit without Space step-up");
  Expect(state.elevation == -1,
         "3D player should remain at -1 when normal movement tries to climb out");
  Expect(state.last_block_reason ==
             sar::render3d::Level3DMoveBlockReason::kStepUpRequired,
         "walking from -1 to 0 should be blocked as step-up-required");
}

void TestLevel3DTargetDiagnosticsReportsPitStepUp() {
  const sar::LevelData level = BuildFlatTestLevel(2, 1, {0, -1});
  const sar::render3d::Level3DPlayerState state = MakeTestPlayer(
      1.5F, 0.5F, -1, -1.0F, 0.0F);

  const sar::render3d::Level3DTargetTileDiagnostics diagnostics =
      sar::render3d::FacingLevel3DTargetTileDiagnostics(level, state);

  Expect(diagnostics.target_tile_x == 0 && diagnostics.target_tile_y == 0,
         "target diagnostics should inspect the tile in facing direction");
  Expect(diagnostics.height_delta == 1,
         "target diagnostics should report -1 to 0 as +1 height delta");
  Expect(!diagnostics.can_enter,
         "normal movement diagnostics should block -1 to 0 step-up");
  Expect(diagnostics.reason ==
             sar::render3d::Level3DMoveBlockReason::kStepUpRequired,
         "normal movement diagnostics should explain Space is required");
  Expect(diagnostics.can_space_step,
         "target diagnostics should allow Space step-up from -1 to 0");
}

void TestLevel3DPlayerStepJumpsOutOfNegativePit() {
  const sar::LevelData level = BuildFlatTestLevel(2, 1, {0, -1});
  sar::render3d::Level3DPlayerState state = MakeTestPlayer(
      1.5F, 0.5F, -1, -1.0F, 0.0F);
  sar::InputState input;
  input.up_down = true;
  input.jump_pressed = true;

  sar::render3d::UpdateLevel3DPlayer(level, input, 0.05F, &state);
  input.jump_pressed = false;
  input.up_down = false;
  Run3DPlayerFrames(level, input, 8, &state);

  Expect(!state.jump_active,
         "Space step-up from -1 to 0 should finish the jump state");
  Expect(state.tile_x < 1.0F,
         "Space step-up should move the 3D player out of the -1 pit");
  Expect(state.elevation == 0,
         "Space step-up from -1 should land on elevation 0");
}

}  // namespace

int main() {
  TestWindowLayout();
  TestSmallWindowLayout();
  TestWindowLayoutScalesPreferredToMonitorLimit();
  TestMenuNavigationSkipsDisabledItems();
  TestTerrainMapping();
  TestProjectConfigLoader();
  TestDeveloperConfigLoader();
  TestProjectConfigLoaderUsesFontDefaults();
  TestVisualMapLoader();
  TestVisualPreparationPipelineUsesPreparedVisualMap();
  TestVisualPreparationPipelineSkeleton();
  TestVisualPreparationSemanticMasks();
  TestVisualPreparationTerrainRegions();
  TestVisualPreparationRegionBorders();
  TestRuinVisualPlanBuildsSceneComposition();
  TestRoadVisualPlanBuildsTerrainRoadDressing();
  TestObjectVisualPlanRemovesGenericObjects();
  TestMicroSceneVisualPlanBuildsDressing();
  TestLevelLoaderBasicPackage();
  TestLevelLoaderManifestPackage();
  TestLevel3DPlayerFallsIntoNegativePitWithoutDamage();
  TestLevel3DPlayerDropIntoNegativePitUsesFallDamageFormula();
  TestLevel3DPlayerCannotWalkOutOfNegativePit();
  TestLevel3DTargetDiagnosticsReportsPitStepUp();
  TestLevel3DPlayerStepJumpsOutOfNegativePit();
  std::cout << "All tests passed.\n";
  return 0;
}
