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

  if (forest_visual_plan.IsValid()) {
    const ForestVisualSummary& summary = forest_visual_plan.summary;
    dump += ", forest_visual: { edge: " +
            std::to_string(summary.forest_edge_tiles) +
            ", mid: " + std::to_string(summary.forest_mid_tiles) +
            ", deep: " + std::to_string(summary.forest_deep_tiles) +
            ", main_clearing: " +
            std::to_string(summary.main_clearing_tiles) +
            ", connector: " +
            std::to_string(summary.connector_corridor_tiles) +
            ", scene: " + std::to_string(summary.scene_space_tiles) +
            " }";
  }

  if (road_visual_plan.IsValid()) {
    const RoadVisualSummary& summary = road_visual_plan.summary;
    dump += ", road_visual: { core: " +
            std::to_string(summary.road_core_tiles) +
            ", side: " + std::to_string(summary.road_side_tiles) +
            ", trampled: " +
            std::to_string(summary.trampled_grass_tiles) +
            ", mud: " + std::to_string(summary.mud_patch_tiles) +
            ", ruin_approach: " +
            std::to_string(summary.ruin_approach_tiles) + " }";
  }

  if (ruin_visual_plan.IsValid()) {
    const RuinVisualSummary& summary = ruin_visual_plan.summary;
    dump += ", ruin_visual: { sites: " +
            std::to_string(summary.site_count) +
            ", walls: " + std::to_string(summary.source_wall_tiles) +
            ", floors: " + std::to_string(summary.source_ruin_tiles) +
            ", rubble: " + std::to_string(summary.rubble_tiles) +
            ", entrances: " + std::to_string(summary.entrance_tiles) +
            " }";
  }


  if (water_visual_plan.IsValid()) {
    const WaterVisualSummary& summary = water_visual_plan.summary;
    dump += ", water_visual: { regions: " +
            std::to_string(summary.water_region_count) +
            ", core: " + std::to_string(summary.water_core_tiles) +
            ", edge: " + std::to_string(summary.water_edge_tiles) +
            ", mud: " + std::to_string(summary.mud_ring_tiles) +
            ", wet_grass: " + std::to_string(summary.wet_grass_tiles) +
            ", reeds: " + std::to_string(summary.reed_zone_tiles) +
            " }";
  }

  if (object_visual_plan.IsValid()) {
    const ObjectVisualSummary& summary = object_visual_plan.summary;
    dump += ", object_visual: { mapped: " +
            std::to_string(summary.mapped_object_count) +
            ", typed_fallback: " +
            std::to_string(summary.typed_fallback_count) +
            ", object_generic: " +
            std::to_string(summary.generic_object_count) + " }";
  }



  if (micro_scene_visual_plan.IsValid()) {
    const MicroSceneSummary& summary = micro_scene_visual_plan.summary;
    dump += ", micro_scenes: { scenes: " +
            std::to_string(summary.scene_count) +
            ", visual_tiles: " + std::to_string(summary.visual_tiles) +
            ", camp: " + std::to_string(summary.camp_scene_count) +
            ", roadside: " +
            std::to_string(summary.roadside_debris_count) +
            ", ruins: " +
            std::to_string(summary.ruin_debris_cluster_count) +
            ", swamp: " +
            std::to_string(summary.swamp_crossing_detail_count) + " }";
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
