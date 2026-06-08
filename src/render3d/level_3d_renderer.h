#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_

#include <string>

#include <raylib.h>

#include "level/level_data.h"
#include "render3d/level_3d_camera.h"
#include "render3d/level_3d_player_controller.h"
#include "window/window_state.h"

namespace sar::render3d {

enum class Level3DRenderMode {
  kTerrain,
  kElevation,
  kCollision,
};

struct Level3DViewState {
  Level3DPlayerState player;
  Level3DCameraState camera;
  Level3DRenderMode mode = Level3DRenderMode::kTerrain;
  float tile_world_size = 1.0F;
  float elevation_step = 0.35F;
  int visible_radius_tiles = 48;
  int culling_deadzone_tiles = 4;
  int culling_center_tile_x = 0;
  int culling_center_tile_y = 0;
  bool culling_center_initialized = false;
  bool initialized = false;
};

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
 * @return String representation for debug overlays and logs.
 */
std::string Level3DViewStateToString(const Level3DViewState& state);

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
