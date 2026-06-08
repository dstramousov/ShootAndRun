#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_

#include <cstddef>
#include <string>
#include <vector>

#include <raylib.h>

#include "level/level_data.h"
#include "render3d/level_3d_camera.h"
#include "render3d/level_3d_player_controller.h"
#include "window/window_state.h"

namespace sar::render3d {

/**
 * @brief Fog-of-war algorithm used by the 3D visibility buffer.
 */
enum class Level3DFogMode {
  kCircle,
  kRaycast,
};

/**
 * @brief Runtime visualization mode for the 3D level renderer.
 */
enum class Level3DRenderMode {
  kTerrain,
  kElevation,
  kCollision,
};

/**
 * @brief Aggregated 3D renderer, camera, culling and visibility state.
 */
struct Level3DViewState {
  Level3DPlayerState player;
  Level3DCameraState camera;
  Level3DRenderMode mode = Level3DRenderMode::kTerrain;
  float tile_world_size = 1.0F;
  float elevation_step = 0.35F;
  float elevation_wall_thickness = 0.075F;
  int visible_radius_tiles = 48;
  int culling_deadzone_tiles = 4;
  int chunk_size_tiles = 16;
  int active_chunk_radius = 3;
  int culling_center_tile_x = 0;
  int culling_center_tile_y = 0;
  bool culling_center_initialized = false;
  bool visibility_enabled = true;
  int visibility_radius_tiles = 22;
  bool visibility_memory_enabled = true;
  Level3DFogMode fog_mode = Level3DFogMode::kCircle;
  float seen_tile_dim_factor = 0.32F;
  int visibility_width = 0;
  int visibility_height = 0;
  std::vector<unsigned char> visibility_tiles;
  std::vector<std::size_t> visibility_current_indices;
  bool visibility_state_valid = false;
  int visibility_last_center_x = -1;
  int visibility_last_center_y = -1;
  int visibility_last_radius_tiles = -1;
  bool visibility_last_enabled = false;
  bool visibility_last_memory_enabled = false;
  Level3DFogMode visibility_last_fog_mode = Level3DFogMode::kCircle;
  bool initialized = false;
};

/**
 * @brief Returns the stable display name of a 3D fog-of-war mode.
 *
 * @param mode 3D fog-of-war mode.
 * @return Stable lowercase mode name.
 */
const char* Level3DFogModeName(Level3DFogMode mode);

/**
 * @brief Returns the stable display name of a 3D render mode.
 *
 * @param mode 3D render mode.
 * @return Stable lowercase mode name.
 */
const char* Level3DRenderModeName(Level3DRenderMode mode);

/**
 * @brief Initializes the full 3D view state for a loaded level.
 *
 * @param level Loaded level data.
 * @param state 3D view state to initialize.
 */
void InitializeLevel3DView(const LevelData& level, Level3DViewState* state);

/**
 * @brief Updates player and camera state for the 3D level view.
 *
 * @param level Loaded level data.
 * @param input Current input state.
 * @param dt Frame delta time in seconds.
 * @param state 3D view state to update.
 */
void UpdateLevel3DView(const LevelData& level, const InputState& input,
                       float dt, Level3DViewState* state);

/**
 * @brief Returns a readable dump of a 3D view state.
 *
 * @param state Current 3D view state.
 * @return String representation for logs.
 */
std::string Level3DViewStateToString(const Level3DViewState& state);

/**
 * @brief Draws the loaded level through the standalone 3D renderer path.
 */
class Level3DRenderer {
 public:
  /**
   * @brief Draws a loaded level as a simple 3D tile-world.
   *
   * The renderer uses runtime terrain, collision and height data directly. It
   * does not depend on the 2D visual pipeline or 2D renderer state.
   *
   * @param level Loaded level data.
   * @param state Current 3D view state.
   * @param window Current window state.
   */
  void Draw(const LevelData& level, const Level3DViewState& state,
            const WindowState& window) const;
};

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_
