#include "visual_pipeline/prepared_level.h"

#include <string>

namespace sar::visual_pipeline {

std::string PreparedLevel::Dump() const {
  std::string dump =
      "PreparedLevel { ready: " + std::string(ready ? "true" : "false") +
      ", size: " + std::to_string(size.width) + "x" +
      std::to_string(size.height) + ", tile_size: " +
      std::to_string(size.tile_size) + ", semantic_masks: " +
      std::to_string(semantic_mask_count) + ", regions: " +
      std::to_string(terrain_region_count) + ", visual_layers: " +
      std::to_string(visual_layer_count) + ", decorations: " +
      std::to_string(decoration_count) + ", render_cache: " +
      std::to_string(render_cache_entry_count);

  if (semantic_masks.IsValid()) {
    const SemanticMaskSummary& summary = semantic_masks.summary;
    dump += ", semantic_summary: { forest: " +
            std::to_string(summary.forest_tiles) +
            ", open: " + std::to_string(summary.open_ground_tiles) +
            ", road: " + std::to_string(summary.road_tiles) +
            ", water: " + std::to_string(summary.water_tiles) +
            ", swamp: " + std::to_string(summary.swamp_tiles) +
            ", ruins: " + std::to_string(summary.ruins_tiles) +
            ", blocked: " + std::to_string(summary.blocked_tiles) + " }";
  }

  if (terrain_regions.IsValid()) {
    const TerrainRegionSummary& summary = terrain_regions.summary;
    dump += ", region_summary: { total: " +
            std::to_string(summary.total_regions) +
            ", forest: " + std::to_string(summary.forest_regions) +
            ", open: " + std::to_string(summary.open_ground_regions) +
            ", road: " + std::to_string(summary.road_regions) +
            ", water: " + std::to_string(summary.water_regions) +
            ", swamp: " + std::to_string(summary.swamp_regions) +
            ", ruins: " + std::to_string(summary.ruins_regions) +
            ", tiny: " + std::to_string(summary.tiny_regions) + " }";
  }

  dump += " }";
  return dump;
}

}  // namespace sar::visual_pipeline
