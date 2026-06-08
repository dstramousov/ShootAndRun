#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_

/**
 * @file src/visual_pipeline/visual_map_data.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for visual_map_data.h.
 */

#include <filesystem>
#include <string>
#include <vector>

#include "level/level_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Stores visual layer grid data shared between runtime systems.
 */
struct VisualLayerGrid {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string role;  ///< Role value carried by this data structure.
  int width = 0;  ///< Size component for width.
  int height = 0;  ///< Signed elevation level for this tile or object.
  std::vector<std::string> tile_ids;  ///< Tile ids value carried by this data structure.

  /**
   * @brief Returns true when the layer has complete grid data.
   *
   * @return True when tile ids match width and height.
   */
  bool IsRenderable() const;
};

/**
 * @brief Stores visual object data data shared between runtime systems.
 */
struct VisualObjectData {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string sprite_id;  ///< Stable identifier for sprite ID.
  std::string draw_layer;  ///< Draw layer value carried by this data structure.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int width = 1;  ///< Size component for width.
  int height = 1;  ///< Signed elevation level for this tile or object.
};

/**
 * @brief Stores visual map data data shared between runtime systems.
 */
struct VisualMapData {
  bool loaded = false;  ///< Loaded value carried by this data structure.
  std::filesystem::path manifest_path;  ///< Filesystem path used by this configuration or data object.
  std::filesystem::path base_path;  ///< Filesystem path used by this configuration or data object.
  std::filesystem::path final_render_path;  ///< Filesystem path used by this configuration or data object.
  std::string schema_version;  ///< Schema version value carried by this data structure.
  std::string generator_version;  ///< Generator version value carried by this data structure.
  std::string visual_profile_id;  ///< Stable identifier for visual profile ID.
  std::string visual_profile_name;  ///< Visual profile name value carried by this data structure.
  LevelSize size;  ///< Size value carried by this data structure.
  int visual_layer_count = 0;  ///< Count of visual layer count entries or events.
  int unique_tile_id_count = 0;  ///< Count of unique tile ID count entries or events.
  int visual_object_count = 0;  ///< Count of visual object count entries or events.
  int visual_chunk_count = 0;  ///< Count of visual chunk count entries or events.
  int chunk_size_tiles = 0;  ///< Chunk size tiles value carried by this data structure.
  bool changes_gameplay = false;  ///< Changes gameplay value carried by this data structure.
  bool moves_markers = false;  ///< Moves markers value carried by this data structure.
  bool changes_collision = false;  ///< Changes collision value carried by this data structure.
  std::vector<VisualLayerGrid> layers;  ///< Layers value carried by this data structure.
  std::vector<VisualObjectData> objects;  ///< Objects value carried by this data structure.
  std::vector<std::string> warnings;  ///< Recoverable validation warnings collected during loading.

  /**
   * @brief Checks whether the visual map has compatible level dimensions.
   *
   * @param raw_size Raw map package dimensions.
   * @param error Error text populated on mismatch.
   * @return True when dimensions are compatible.
   */
  bool ValidateAgainstRawSize(const LevelSize& raw_size,
                              std::string* error) const;  ///< Const value carried by this data structure.

  /**
   * @brief Returns true when at least one visual layer can be rendered.
   *
   * @return Renderable layer availability flag.
   */
  bool HasRenderableLayer() const;

  /**
   * @brief Returns a readable dump of the prepared visual-map state.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_
