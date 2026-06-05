#include "visual_pipeline/prepared_level.h"

#include <string>

namespace sar::visual_pipeline {

const char* PreparedLevelSourceName(PreparedLevelSource source) {
  switch (source) {
    case PreparedLevelSource::kCppPipeline:
      return "cpp_pipeline";
    case PreparedLevelSource::kPreparedVisualMap:
      return "prepared_visual_map";
    case PreparedLevelSource::kHybrid:
      return "hybrid";
  }

  return "cpp_pipeline";
}

std::string PreparedLevel::Dump() const {
  std::string dump =
      "PreparedLevel { ready: " + std::string(ready ? "true" : "false") +
      ", size: " + std::to_string(size.width) + "x" +
      std::to_string(size.height) + ", tile_size: " +
      std::to_string(size.tile_size) + ", source: " +
      PreparedLevelSourceName(source) + ", semantic_masks: " +
      std::to_string(semantic_mask_count) + ", regions: " +
      std::to_string(terrain_region_count) + ", region_borders: " +
      std::to_string(region_border_count) + ", visual_layers: " +
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

  if (region_borders.IsValid()) {
    const RegionBorderSummary& summary = region_borders.summary;
    dump += ", border_summary: { border_tiles: " +
            std::to_string(summary.border_tile_count) +
            ", edge: " + std::to_string(summary.edge_tile_count) +
            ", corner: " + std::to_string(summary.corner_tile_count) +
            ", map_edge: " + std::to_string(summary.map_edge_tile_count) +
            " }";
  }

  if (prepared_visual_map.loaded) {
    dump += ", prepared_visual_map: { layers: " +
            std::to_string(prepared_visual_map.visual_layer_count) +
            ", unique_tiles: " +
            std::to_string(prepared_visual_map.unique_tile_id_count) +
            ", objects: " +
            std::to_string(prepared_visual_map.visual_object_count) +
            ", chunks: " +
            std::to_string(prepared_visual_map.visual_chunk_count) + " }";
  }

  dump += " }";
  return dump;
}

}  // namespace sar::visual_pipeline
