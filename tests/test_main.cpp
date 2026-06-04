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

  Expect(state.width == 1280, "window width should use base width");
  Expect(state.height == 720, "window height should use base height");
  Expect(state.x == 320, "window x should be centered");
  Expect(state.y == 180, "window y should be centered");
  Expect(state.ui_scale == 1.0F, "ui scale should be one for base size");
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
                "  \"ui_font_size\": 22\n"
                "}\n");

  const sar::ProjectConfigResult result = sar::LoadProjectConfig(config_path);
  Expect(result.ok, "project config should load successfully");
  Expect(result.config.map_package_path == "data/maps/sample_level",
         "map package path should be read from project config");
  Expect(result.config.ui_font_path == "data/fonts/test.ttf",
         "font path should be read from project config");
  Expect(result.config.ui_font_size == 22,
         "font size should be read from project config");

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
  std::cout << "All tests passed.\n";
  return 0;
}
