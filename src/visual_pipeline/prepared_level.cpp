#include "visual_pipeline/prepared_level.h"

#include <string>

namespace sar::visual_pipeline {

std::string PreparedLevel::Dump() const {
  return "PreparedLevel { ready: " + std::string(ready ? "true" : "false") +
         ", size: " + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", tile_size: " +
         std::to_string(size.tile_size) + ", semantic_masks: " +
         std::to_string(semantic_mask_count) + ", regions: " +
         std::to_string(terrain_region_count) + ", visual_layers: " +
         std::to_string(visual_layer_count) + ", decorations: " +
         std::to_string(decoration_count) + ", render_cache: " +
         std::to_string(render_cache_entry_count) + " }";
}

}  // namespace sar::visual_pipeline
