#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_LEVEL_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_LEVEL_RENDERER_H_

#include <string>

#include "level/level_data.h"
#include "window/window_state.h"

namespace sar {

struct LevelViewState {
  float target_x = 0.0F;
  float target_y = 0.0F;
  float zoom = 1.0F;
  float min_zoom = 0.5F;
  float max_zoom = 4.0F;
  float pan_speed_px_per_sec = 720.0F;
};

/**
 * @brief Centers a level view on a preferred spawn marker or map center.
 *
 * @param level Loaded level data.
 * @param view View state to initialize.
 */
void InitializeLevelView(const LevelData& level, LevelViewState* view);

/**
 * @brief Clamps a level view so the camera does not show space outside map.
 *
 * @param level Loaded level data.
 * @param window Current window state.
 * @param view View state to clamp.
 */
void ClampLevelViewToMap(const LevelData& level, const WindowState& window,
                         LevelViewState* view);

/**
 * @brief Returns a readable dump of a level view state.
 *
 * @param view Current level view state.
 * @return String representation for debug overlay and logs.
 */
std::string LevelViewStateToString(const LevelViewState& view);

class LevelRenderer {
 public:
  /**
   * @brief Draws the loaded level terrain and debug markers.
   *
   * @param level Loaded level data.
   * @param view Current level view state.
   * @param window Current window state.
   */
  void DrawTerrain(const LevelData& level, const LevelViewState& view,
                   const WindowState& window) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_LEVEL_RENDERER_H_
