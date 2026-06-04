#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "app/project_config.h"
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

  {
    std::ofstream output(config_path);
    output << "{\n"
           << "  \"map_package_path\": \"data/maps/sample_level\"\n"
           << "}\n";
  }

  const sar::ProjectConfigResult result = sar::LoadProjectConfig(config_path);
  Expect(result.ok, "project config should load successfully");
  Expect(result.config.map_package_path == "data/maps/sample_level",
         "map package path should be read from project config");

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

}  // namespace

int main() {
  TestWindowLayout();
  TestSmallWindowLayout();
  TestMenuNavigationSkipsDisabledItems();
  TestTerrainMapping();
  TestProjectConfigLoader();
  std::cout << "All tests passed.\n";
  return 0;
}
