#include "visual_pipeline/visual_map_data.h"

#include <string>

namespace sar::visual_pipeline {

bool VisualMapData::ValidateAgainstRawSize(const LevelSize& raw_size,
                                           std::string* error) const {
  if (size.width != raw_size.width || size.height != raw_size.height ||
      size.tile_size != raw_size.tile_size) {
    if (error != nullptr) {
      *error = "visual_map dimensions do not match raw map: visual=" +
               std::to_string(size.width) + "x" +
               std::to_string(size.height) + " tile_size=" +
               std::to_string(size.tile_size) + " raw=" +
               std::to_string(raw_size.width) + "x" +
               std::to_string(raw_size.height) + " tile_size=" +
               std::to_string(raw_size.tile_size);
    }
    return false;
  }
  return true;
}

std::string VisualMapData::Dump() const {
  return "VisualMapData { loaded: " +
         std::string(loaded ? "true" : "false") + ", schema: " +
         (schema_version.empty() ? "unknown" : schema_version) +
         ", generator: " +
         (generator_version.empty() ? "unknown" : generator_version) +
         ", profile: " +
         (visual_profile_id.empty() ? "unknown" : visual_profile_id) +
         ", size: " + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", tile_size: " +
         std::to_string(size.tile_size) + ", layers: " +
         std::to_string(visual_layer_count) + ", unique_tiles: " +
         std::to_string(unique_tile_id_count) + ", objects: " +
         std::to_string(visual_object_count) + ", chunks: " +
         std::to_string(visual_chunk_count) + ", chunk_size: " +
         std::to_string(chunk_size_tiles) + ", changes_gameplay: " +
         std::string(changes_gameplay ? "true" : "false") +
         ", moves_markers: " + std::string(moves_markers ? "true" : "false") +
         ", changes_collision: " +
         std::string(changes_collision ? "true" : "false") + " }";
}

}  // namespace sar::visual_pipeline
