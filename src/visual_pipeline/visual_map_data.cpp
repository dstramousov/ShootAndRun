/**
 * @file src/visual_pipeline/visual_map_data.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for visual_map_data.cpp.
 */

#include "visual_pipeline/visual_map_data.h"

#include <string>

namespace sar::visual_pipeline {

/**
 * @brief Checks whether renderable is true.
 */
bool VisualLayerGrid::IsRenderable() const {
  if (width <= 0 || height <= 0) {
    return false;
  }
  return static_cast<int>(tile_ids.size()) == width * height;
}

/**
 * @brief Validates against raw size and reports failures.
 */
bool VisualMapData::ValidateAgainstRawSize(const LevelSize& raw_size,
                                           std::string* error) const {
  if (size.width != raw_size.width || size.height != raw_size.height) {
    if (error != nullptr) {
      *error = "visual_map dimensions mismatch: visual=" +
               std::to_string(size.width) + "x" +
               std::to_string(size.height) + " raw=" +
               std::to_string(raw_size.width) + "x" +
               std::to_string(raw_size.height);
    }
    return false;
  }

  if (size.tile_size != raw_size.tile_size) {
    if (error != nullptr) {
      *error = "visual_map tile size mismatch: visual=" +
               std::to_string(size.tile_size) + " raw=" +
               std::to_string(raw_size.tile_size);
    }
    return false;
  }

  return true;
}

/**
 * @brief Checks whether renderable layer is present.
 */
bool VisualMapData::HasRenderableLayer() const {
  for (const VisualLayerGrid& layer : layers) {
    if (layer.IsRenderable()) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string VisualMapData::Dump() const {
  return "VisualMapData { loaded: " + std::string(loaded ? "true" : "false") +
         ", schema: " + schema_version + ", generator: " +
         generator_version + ", profile: " + visual_profile_id +
         ", size: " + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", tile_size: " +
         std::to_string(size.tile_size) + ", layers: " +
         std::to_string(visual_layer_count) + ", unique_tiles: " +
         std::to_string(unique_tile_id_count) + ", objects: " +
         std::to_string(visual_object_count) + ", chunks: " +
         std::to_string(visual_chunk_count) + ", renderable_layers: " +
         std::string(HasRenderableLayer() ? "true" : "false") +
         ", changes_gameplay: " +
         std::string(changes_gameplay ? "true" : "false") +
         ", moves_markers: " + std::string(moves_markers ? "true" : "false") +
         ", changes_collision: " +
         std::string(changes_collision ? "true" : "false") + " }";
}

}  // namespace sar::visual_pipeline
