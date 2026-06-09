#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_RENDERER_H_

/**
 * @file src/render3d/level_3d_renderer.h
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains public
 * declarations for level_3d_renderer.h.
 */

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
 * @brief Lightweight frame counters for the immediate-mode 3D renderer.
 */
struct Level3DPerfStats {
  int fps = 0;  ///< Current raylib FPS value sampled when drawing the overlay.
  double frame_ms = 0.0;  ///< Current frame time in milliseconds.
  double visibility_update_ms = 0.0;  ///< Last visibility update duration in milliseconds.
  int active_chunks_x = 0;  ///< Number of active chunks on the X axis.
  int active_chunks_y = 0;  ///< Number of active chunks on the Y axis.
  int active_chunks_total = 0;  ///< Total active chunk count.
  int active_tile_candidates = 0;  ///< Tile candidates inside the active chunk range.
  int renderable_tiles = 0;  ///< Tiles passing the current visibility filter.
  int ground_tiles_drawn = 0;  ///< Batched ground slab primitives submitted this frame.
  int ground_tiles_covered = 0;  ///< Ground tiles covered by batched ground slabs this frame.
  int transition_candidates = 0;  ///< Elevation transitions considered in the active range.
  int transitions_drawn = 0;  ///< Elevation transitions submitted this frame.
  int elevation_wall_faces_drawn = 0;  ///< Elevation wall face cubes submitted this frame.
  int blocking_volumes_drawn = 0;  ///< Blocking volume cubes submitted this frame.
  int forest_boundary_volumes_drawn = 0;  ///< Passable forest boundary volumes submitted this frame.
  int forest_boundary_wireframes_drawn = 0;  ///< Forest boundary wireframes submitted this frame.
  int debug_overlay_slabs_drawn = 0;  ///< Elevation debug overlay slabs submitted this frame.

  /**
   * @brief Returns an approximate immediate-mode primitive count.
   *
   * @return Estimated number of raylib draw submissions for terrain primitives.
   */
  [[nodiscard]] int EstimatedPrimitiveSubmissions() const;
};

/**
 * @brief Aggregated 3D renderer, camera, culling and visibility state.
 */
struct Level3DViewState {
  Level3DPlayerState player;  ///< Player value carried by this data structure.
  Level3DCameraState camera;  ///< Camera value carried by this data structure.
  Level3DRenderMode mode = Level3DRenderMode::kTerrain;  ///< Mode value carried by this data structure.
  float tile_world_size = 1.0F;  ///< Tile world size value carried by this data structure.
  float elevation_step = 0.35F;  ///< Elevation step value carried by this data structure.
  float elevation_wall_thickness = 0.075F;  ///< Elevation wall thickness value carried by this data structure.
  int visible_radius_tiles = 48;  ///< Visible radius tiles value carried by this data structure.
  int culling_deadzone_tiles = 4;  ///< Culling deadzone tiles value carried by this data structure.
  int chunk_size_tiles = 16;  ///< Chunk size tiles value carried by this data structure.
  int active_chunk_radius = 3;  ///< Active chunk radius value carried by this data structure.
  int culling_center_tile_x = 0;  ///< Tile, screen, or world coordinate for culling center tile x.
  int culling_center_tile_y = 0;  ///< Tile, screen, or world coordinate for culling center tile y.
  bool culling_center_initialized = false;  ///< Culling center initialized value carried by this data structure.
  bool visibility_enabled = true;  ///< Visibility enabled value carried by this data structure.
  int visibility_radius_tiles = 22;  ///< Visibility radius tiles value carried by this data structure.
  bool visibility_memory_enabled = true;  ///< Visibility memory enabled value carried by this data structure.
  Level3DFogMode fog_mode = Level3DFogMode::kCircle;  ///< Fog mode value carried by this data structure.
  float seen_tile_dim_factor = 0.32F;  ///< Scaling factor for seen tile dim factor.
  int visibility_width = 0;  ///< Size component for visibility width.
  int visibility_height = 0;  ///< Size component for visibility height.
  std::vector<unsigned char> visibility_tiles;  ///< Visibility tiles value carried by this data structure.
  std::vector<std::size_t> visibility_current_indices;  ///< Visibility current indices value carried by this data structure.
  bool visibility_state_valid = false;  ///< Visibility state valid value carried by this data structure.
  bool debug_elevation_overlay_enabled = false;  ///< Runtime-only elevation debug overlay toggle.
  bool debug_elevation_move_logs_enabled = false;  ///< Runtime-only elevation movement diagnostics logging toggle.
  bool debug_render3d_perf_enabled = false;  ///< Runtime-only render3d performance diagnostics toggle.
  double last_visibility_update_ms = 0.0;  ///< Last visibility update duration in milliseconds.
  int visibility_last_center_x = -1;  ///< Tile, screen, or world coordinate for visibility last center x.
  int visibility_last_center_y = -1;  ///< Tile, screen, or world coordinate for visibility last center y.
  int visibility_last_radius_tiles = -1;  ///< Visibility last radius tiles value carried by this data structure.
  bool visibility_last_enabled = false;  ///< Visibility last enabled value carried by this data structure.
  bool visibility_last_memory_enabled = false;  ///< Visibility last memory enabled value carried by this data structure.
  Level3DFogMode visibility_last_fog_mode = Level3DFogMode::kCircle;  ///< Visibility last fog mode value carried by this data structure.
  bool initialized = false;  ///< Initialized value carried by this data structure.
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
