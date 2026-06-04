#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "app/project_config.h"
#include "level/level_loader.h"
#include "level/terrain_type.h"
#include "ui/main_menu.h"
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
  TestProjectConfigLoaderUsesFontDefaults();
  TestLevelLoaderBasicPackage();
  TestLevelLoaderManifestPackage();
  std::cout << "All tests passed.\n";
  return 0;
}
