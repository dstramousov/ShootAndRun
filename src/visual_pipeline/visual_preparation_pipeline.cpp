#include "visual_pipeline/visual_preparation_pipeline.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <map>
#include <utility>
#include <vector>

#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/debug_artifact_writer.h"
#include "visual_pipeline/steps/build_semantic_masks_step.h"
#include "visual_pipeline/steps/classify_region_borders_step.h"
#include "visual_pipeline/steps/build_forest_visual_plan_step.h"
#include "visual_pipeline/steps/build_road_visual_plan_step.h"
#include "visual_pipeline/steps/build_ruin_visual_plan_step.h"
#include "visual_pipeline/steps/build_water_visual_plan_step.h"
#include "visual_pipeline/steps/build_object_visual_plan_step.h"
#include "visual_pipeline/steps/build_terrain_regions_step.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/water_visual_plan.h"
#include "visual_pipeline/region_borders.h"
#include "visual_pipeline/visual_map_loader.h"
#include "visual_pipeline/visual_pipeline_config.h"

namespace sar::visual_pipeline {
namespace {

bool UsesPreparedVisualMap(VisualPipelineMode mode) {
  return mode == VisualPipelineMode::kUsePreparedVisualMap ||
         mode == VisualPipelineMode::kHybrid ||
         mode == VisualPipelineMode::kCompare;
}

bool ShouldRunCppAnalysis(const VisualPreparationOptions& options) {
  if (options.visual_pipeline_config.mode == VisualPipelineMode::kBuildCpp ||
      options.visual_pipeline_config.mode == VisualPipelineMode::kCompare) {
    return true;
  }
  return options.visual_pipeline_config.run_cpp_analysis;
}

std::vector<PipelineStepInfo> BuildDefaultSteps(
    const VisualPreparationOptions& options) {
  std::vector<PipelineStepInfo> steps = {
      {"Load raw map package"},
      {"Validate raw map data"},
  };

  if (UsesPreparedVisualMap(options.visual_pipeline_config.mode)) {
    steps.push_back({"Load prepared visual map"});
  }

  if (ShouldRunCppAnalysis(options)) {
    steps.push_back({"Build semantic terrain masks"});
    steps.push_back({"Build terrain regions"});
    steps.push_back({"Classify region borders"});
    steps.push_back({"Build semantic map links"});
  }

  steps.push_back({"Build road and path shapes"});
  steps.push_back({"Build forest masses"});
  steps.push_back({"Build ruin scene compositions"});
  steps.push_back({"Build water and swamp edges"});
  steps.push_back({"Place visual decorations"});
  steps.push_back({"Build render cache"});
  return steps;
}

std::filesystem::path ResolvePreparedVisualMapPath(
    const VisualPreparationOptions& options) {
  const std::filesystem::path configured =
      options.visual_pipeline_config.prepared_visual_map_path;
  if (configured.is_absolute()) {
    return configured;
  }
  if (options.map_package_path.empty()) {
    return configured;
  }
  return options.map_package_path / configured;
}


std::filesystem::path ResolveDebugOutputPath(
    const VisualPreparationOptions& options) {
  const std::filesystem::path configured =
      options.visual_pipeline_config.debug_output_path;
  if (configured.is_absolute()) {
    return configured;
  }
  if (options.map_package_path.empty()) {
    return configured;
  }
  return options.map_package_path / configured;
}

void AddDebugArtifactResult(std::string_view artifact_name,
                            bool written,
                            const std::string& error,
                            PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }
  if (written) {
    report->summaries.push_back(std::string("debug artifact written: ") +
                                std::string(artifact_name));
    return;
  }
  report->warnings.push_back(std::string("debug artifact write failed: ") +
                             std::string(artifact_name) + ": " + error);
}

bool HasPreparedVisualMap(const PreparedLevel& prepared_level) {
  return prepared_level.prepared_visual_map.loaded;
}

PreparedLevelSource SourceForMode(VisualPipelineMode mode) {
  switch (mode) {
    case VisualPipelineMode::kUsePreparedVisualMap:
      return PreparedLevelSource::kPreparedVisualMap;
    case VisualPipelineMode::kHybrid:
    case VisualPipelineMode::kCompare:
      return PreparedLevelSource::kHybrid;
    case VisualPipelineMode::kBuildCpp:
      return PreparedLevelSource::kCppPipeline;
  }

  return PreparedLevelSource::kCppPipeline;
}


std::string TerrainSummaryLine(const SemanticMaskSummary& summary) {
  return "terrain tiles=" + std::to_string(summary.total_tiles) +
         " open=" + std::to_string(summary.open_ground_tiles) +
         " forest=" + std::to_string(summary.forest_tiles) +
         " road=" + std::to_string(summary.road_tiles) +
         " swamp=" + std::to_string(summary.swamp_tiles) +
         " water=" + std::to_string(summary.water_tiles) +
         " ruins=" + std::to_string(summary.ruins_tiles) +
         " wall=" + std::to_string(summary.wall_tiles) +
         " unknown=" + std::to_string(summary.unknown_tiles);
}

std::string RuntimeSummaryLine(const SemanticMaskSummary& summary) {
  return "runtime walkable=" + std::to_string(summary.walkable_tiles) +
         " blocked=" + std::to_string(summary.blocked_tiles) +
         " vision_blocked=" +
         std::to_string(summary.vision_blocked_tiles) +
         " projectile_blocked=" +
         std::to_string(summary.projectile_blocked_tiles) +
         " cover=" + std::to_string(summary.cover_tiles) +
         " concealment=" + std::to_string(summary.concealment_tiles) +
         " low_ground=" + std::to_string(summary.low_ground_tiles) +
         " elevated=" + std::to_string(summary.elevated_tiles);
}

std::string CountMapLine(const std::string& prefix,
                         const std::map<std::string, int>& counts) {
  std::string line = prefix;
  if (counts.empty()) {
    return line + " none";
  }

  bool first = true;
  for (const auto& [name, count] : counts) {
    line += first ? " " : " ";
    line += name + "=" + std::to_string(count);
    first = false;
  }
  return line;
}

std::string CatalogSummaryLine(const LevelData& level) {
  return "terrain_catalog used=" +
         std::string(level.used_tile_catalog ? "true" : "false") +
         " catalog_types=" + std::to_string(level.tile_catalog_type_count) +
         " raw_types=" + std::to_string(level.terrain_type_counts.size()) +
         " unknown_types=" +
         std::to_string(level.unknown_terrain_type_counts.size());
}

void AddSemanticMaskDiagnostics(const LevelData& level,
                                const SemanticMaskSummary& summary,
                                PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(CatalogSummaryLine(level));
  report->summaries.push_back(CountMapLine("terrain_type_counts:",
                                           level.terrain_type_counts));
  if (!level.unknown_terrain_type_counts.empty()) {
    report->summaries.push_back(CountMapLine("unknown_terrain_types:",
                                             level.unknown_terrain_type_counts));
  }
  report->summaries.push_back(TerrainSummaryLine(summary));
  report->summaries.push_back(RuntimeSummaryLine(summary));
  if (summary.unknown_tiles > 0) {
    report->warnings.push_back("unknown terrain tiles=" +
                               std::to_string(summary.unknown_tiles));
  }
}

std::string RegionSummaryLine(const TerrainRegionSummary& summary) {
  return "regions total=" + std::to_string(summary.total_regions) +
         " open=" + std::to_string(summary.open_ground_regions) +
         " forest=" + std::to_string(summary.forest_regions) +
         " road=" + std::to_string(summary.road_regions) +
         " swamp=" + std::to_string(summary.swamp_regions) +
         " water=" + std::to_string(summary.water_regions) +
         " ruins=" + std::to_string(summary.ruins_regions) +
         " wall=" + std::to_string(summary.wall_regions) +
         " unknown=" + std::to_string(summary.unknown_regions) +
         " tiny=" + std::to_string(summary.tiny_regions);
}

std::string RegionLargestLine(const TerrainRegionSummary& summary) {
  return "region_stats largest=" + std::to_string(summary.largest_region_area) +
         " largest_forest=" + std::to_string(summary.largest_forest_area) +
         " largest_open=" +
         std::to_string(summary.largest_open_ground_area);
}

void AddTerrainRegionDiagnostics(const TerrainRegionSummary& summary,
                                 PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(RegionSummaryLine(summary));
  report->summaries.push_back(RegionLargestLine(summary));
  if (summary.tiny_regions > 0) {
    report->warnings.push_back("tiny terrain regions=" +
                               std::to_string(summary.tiny_regions));
  }
  if (summary.unknown_regions > 0) {
    report->warnings.push_back("unknown terrain regions=" +
                               std::to_string(summary.unknown_regions));
  }
}

std::string BorderSummaryLine(const RegionBorderSummary& summary) {
  return "region_borders tiles=" + std::to_string(summary.border_tile_count) +
         " edge=" + std::to_string(summary.edge_tile_count) +
         " corner=" + std::to_string(summary.corner_tile_count) +
         " thin=" + std::to_string(summary.thin_tile_count) +
         " complex=" + std::to_string(summary.complex_tile_count) +
         " map_edge=" + std::to_string(summary.map_edge_tile_count);
}

std::string BorderNeighborSummaryLine(const RegionBorderSummary& summary) {
  return "border_neighbors open=" +
         std::to_string(summary.neighbor_open_ground) +
         " forest=" + std::to_string(summary.neighbor_forest) +
         " road=" + std::to_string(summary.neighbor_road) +
         " swamp=" + std::to_string(summary.neighbor_swamp) +
         " water=" + std::to_string(summary.neighbor_water) +
         " ruins=" + std::to_string(summary.neighbor_ruins) +
         " wall=" + std::to_string(summary.neighbor_wall) +
         " unknown=" + std::to_string(summary.neighbor_unknown) +
         " outside_map=" + std::to_string(summary.neighbor_outside_map);
}

void AddRegionBorderDiagnostics(const RegionBorderSummary& summary,
                                PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(BorderSummaryLine(summary));
  report->summaries.push_back(BorderNeighborSummaryLine(summary));
  if (summary.complex_tile_count > 0) {
    report->warnings.push_back("complex border tiles=" +
                               std::to_string(summary.complex_tile_count));
  }
}

std::string ForestVisualSummaryLine(const ForestVisualSummary& summary) {
  return "forest_visual edge=" + std::to_string(summary.forest_edge_tiles) +
         " mid=" + std::to_string(summary.forest_mid_tiles) +
         " deep=" + std::to_string(summary.forest_deep_tiles) +
         " suppressed_tiny=" +
         std::to_string(summary.suppressed_tiny_forest_tiles) +
         " canopy=" + std::to_string(summary.canopy_candidate_tiles) +
         " mass_groups=" +
         std::to_string(summary.forest_mass_group_count) +
         " route_influence=" +
         std::to_string(summary.route_influenced_tiles);
}

std::string ClearingRoleSummaryLine(const ForestVisualSummary& summary) {
  return "clearing_roles main=" +
         std::to_string(summary.main_clearing_tiles) +
         " side=" + std::to_string(summary.side_clearing_tiles) +
         " connector=" +
         std::to_string(summary.connector_corridor_tiles) +
         " micro=" + std::to_string(summary.micro_clearing_tiles) +
         " scene=" + std::to_string(summary.scene_space_tiles) +
         " ruins_scene=" + std::to_string(summary.ruins_scene_tiles) +
         " road_approach=" +
         std::to_string(summary.road_approach_scene_tiles) +
         " object_scene=" + std::to_string(summary.object_scene_tiles) +
         " generic_scene=" + std::to_string(summary.generic_scene_tiles);
}

void AddForestVisualDiagnostics(const ForestVisualSummary& summary,
                                PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(ForestVisualSummaryLine(summary));
  report->summaries.push_back(ClearingRoleSummaryLine(summary));
  if (summary.forest_tiles > 0 && summary.forest_deep_tiles == 0) {
    report->warnings.push_back("deep forest band is empty");
  }
  if (summary.route_influenced_tiles == 0) {
    report->warnings.push_back("route influence is empty");
  }
}

std::string RoadVisualSummaryLine(const RoadVisualSummary& summary) {
  return "road_visual source=terrain_road routes_used_for_visual_roads=" +
         std::string(summary.routes_used_for_visual_roads ? "true" : "false") +
         " routes=" + std::to_string(summary.route_count) +
         " main=" + std::to_string(summary.main_route_count) +
         " side=" + std::to_string(summary.side_route_count) +
         " hidden=" + std::to_string(summary.hidden_route_count) +
         " terrain_road=" + std::to_string(summary.terrain_road_tiles) +
         " core=" + std::to_string(summary.road_core_tiles) +
         " side_band=" + std::to_string(summary.road_side_tiles) +
         " trampled=" + std::to_string(summary.trampled_grass_tiles) +
         " mud=" + std::to_string(summary.mud_patch_tiles) +
         " ruin_approach=" + std::to_string(summary.ruin_approach_tiles) +
         " dressing=" + std::to_string(summary.road_dressing_tiles);
}

void AddRoadVisualDiagnostics(const RoadVisualSummary& summary,
                              PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(RoadVisualSummaryLine(summary));
  if (summary.routes_used_for_visual_roads) {
    report->warnings.push_back("routes are used for visual roads");
  }
  if (summary.terrain_road_tiles > 0 && summary.road_core_tiles == 0) {
    report->warnings.push_back("road core band is empty");
  }
}

std::string RuinVisualSummaryLine(const RuinVisualSummary& summary) {
  return "ruin_visual sites=" + std::to_string(summary.site_count) +
         " source_ruins=" + std::to_string(summary.source_ruin_tiles) +
         " source_walls=" + std::to_string(summary.source_wall_tiles) +
         " cracked=" + std::to_string(summary.cracked_floor_tiles) +
         " overgrown=" + std::to_string(summary.overgrown_floor_tiles) +
         " intact=" + std::to_string(summary.wall_intact_tiles) +
         " broken=" + std::to_string(summary.wall_broken_tiles) +
         " corners=" + std::to_string(summary.wall_corner_tiles) +
         " endcaps=" + std::to_string(summary.wall_endcap_tiles) +
         " rubble=" + std::to_string(summary.rubble_tiles) +
         " entrances=" + std::to_string(summary.entrance_tiles) +
         " visual=" + std::to_string(summary.visual_tiles);
}

void AddRuinVisualDiagnostics(const RuinVisualSummary& summary,
                              PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(RuinVisualSummaryLine(summary));
  if (summary.source_wall_tiles > 0 &&
      summary.wall_broken_tiles + summary.wall_corner_tiles +
              summary.wall_endcap_tiles ==
          0) {
    report->warnings.push_back("ruin wall detail bands are empty");
  }
}

std::string WaterVisualSummaryLine(const WaterVisualSummary& summary) {
  return "water_visual regions=" +
         std::to_string(summary.water_region_count) +
         " source_water=" + std::to_string(summary.source_water_tiles) +
         " source_swamp=" + std::to_string(summary.source_swamp_tiles) +
         " core=" + std::to_string(summary.water_core_tiles) +
         " edge=" + std::to_string(summary.water_edge_tiles) +
         " mud=" + std::to_string(summary.mud_ring_tiles) +
         " wet_grass=" + std::to_string(summary.wet_grass_tiles) +
         " reeds=" + std::to_string(summary.reed_zone_tiles) +
         " crossings=" + std::to_string(summary.crossing_tiles) +
         " visual=" + std::to_string(summary.visual_tiles);
}

void AddWaterVisualDiagnostics(const WaterVisualSummary& summary,
                               PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(WaterVisualSummaryLine(summary));
  if (summary.water_like_tiles > 0 && summary.water_edge_tiles == 0) {
    report->warnings.push_back("water edge band is empty");
  }
  if (summary.water_like_tiles > 0 && summary.mud_ring_tiles == 0) {
    report->warnings.push_back("water mud ring is empty");
  }
}

std::string ObjectVisualSummaryLine(const ObjectVisualSummary& summary) {
  return "object_visual source=" +
         std::to_string(summary.source_object_count) +
         " mapped=" + std::to_string(summary.mapped_object_count) +
         " typed_fallback=" +
         std::to_string(summary.typed_fallback_count) +
         " object_generic=" +
         std::to_string(summary.generic_object_count) +
         " missing_sprite_uses=" +
         std::to_string(summary.missing_sprite_uses) +
         " skipped_sprites=" + std::to_string(summary.skipped_sprites);
}

void AddObjectVisualDiagnostics(const ObjectVisualSummary& summary,
                                PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(ObjectVisualSummaryLine(summary));
  if (summary.generic_object_count > 0) {
    report->warnings.push_back("generic visual objects=" +
                               std::to_string(summary.generic_object_count));
  }
  if (summary.missing_sprite_uses > 0) {
    report->warnings.push_back("missing sprite uses=" +
                               std::to_string(summary.missing_sprite_uses));
  }
}

void AddVisualMapDiagnostics(const VisualMapData& data,
                             PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(data.Dump());
  report->summaries.push_back(
      "visual_map summary layers=" +
      std::to_string(data.visual_layer_count) +
      " unique_tiles=" + std::to_string(data.unique_tile_id_count) +
      " objects=" + std::to_string(data.visual_object_count) +
      " chunks=" + std::to_string(data.visual_chunk_count) +
      " final_render=" +
      (data.final_render_path.empty() ? std::string("none")
                                      : data.final_render_path.string()));
  for (const std::string& warning : data.warnings) {
    report->warnings.push_back(warning);
  }
}

}  // namespace

void VisualPreparationPipeline::Start(const LevelData& level) {
  VisualPreparationOptions options;
  options.visual_pipeline_config.mode = VisualPipelineMode::kBuildCpp;
  Start(level, std::move(options));
}

void VisualPreparationPipeline::Start(const LevelData& level,
                                      VisualPreparationOptions options) {
  options_ = std::move(options);
  steps_ = BuildDefaultSteps(options_);
  progress_ = {};
  last_step_report_ = {};
  prepared_level_ = {};
  prepared_level_.size = level.size;
  prepared_level_.source =
      SourceForMode(options_.visual_pipeline_config.mode);
  progress_.total_steps = static_cast<int>(steps_.size());
  progress_.running = true;
  progress_.finished = steps_.empty();
  progress_.current_step_name = steps_.empty() ? "Done" : steps_.front().name;
}

void VisualPreparationPipeline::AdvanceOneStep(const LevelData& level) {
  if (!progress_.running || progress_.failed || progress_.finished) {
    return;
  }

  if (progress_.completed_steps < 0 ||
      progress_.completed_steps >= static_cast<int>(steps_.size())) {
    Fail("visual preparation step index is out of range");
    return;
  }

  const PipelineStepInfo& step =
      steps_[static_cast<std::size_t>(progress_.completed_steps)];
  progress_.current_step_name = step.name;

  PipelineStepReport report;
  report.step_index = progress_.completed_steps + 1;
  report.total_steps = progress_.total_steps;
  report.step_name = step.name;

  const auto started_at = std::chrono::steady_clock::now();
  RunCurrentStep(level, step, &report);
  const auto finished_at = std::chrono::steady_clock::now();
  report.duration_ms =
      std::chrono::duration<double, std::milli>(finished_at - started_at)
          .count();
  report.success = !progress_.failed;
  last_step_report_ = std::move(report);

  if (progress_.failed) {
    return;
  }

  ++progress_.completed_steps;
  if (progress_.completed_steps >= progress_.total_steps) {
    progress_.running = false;
    progress_.finished = true;
    progress_.current_step_name = "Done";
    prepared_level_.ready = true;
    return;
  }

  progress_.current_step_name =
      steps_[static_cast<std::size_t>(progress_.completed_steps)].name;
}

void VisualPreparationPipeline::RunCurrentStep(const LevelData& level,
                                               const PipelineStepInfo& step,
                                               PipelineStepReport* report) {
  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0) {
    Fail("raw level size is invalid");
    return;
  }

  if (step.name == "Load raw map package") {
    report->summaries.push_back(
        "raw map already loaded size=" + std::to_string(level.size.width) +
        "x" + std::to_string(level.size.height) +
        " tile_size=" + std::to_string(level.size.tile_size) +
        " cells=" + std::to_string(level.cells.size()) +
        " markers=" + std::to_string(level.markers.size()));
    return;
  }

  if (step.name == "Validate raw map data") {
    const int expected_cells = level.size.width * level.size.height;
    if (expected_cells <= 0 ||
        level.cells.size() != static_cast<std::size_t>(expected_cells)) {
      Fail("raw level cell count does not match level dimensions");
      return;
    }

    report->summaries.push_back(
        "validated map cells=" + std::to_string(level.cells.size()) +
        " expected=" + std::to_string(expected_cells));
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteInputValidationReport(
          level, &artifact_error);
      AddDebugArtifactResult("reports/00_input_validation.json", written,
                             artifact_error, report);
    }
    return;
  }

  if (step.name == "Load prepared visual map") {
    const std::filesystem::path visual_map_path =
        ResolvePreparedVisualMapPath(options_);
    VisualMapLoader loader;
    const VisualMapLoadResult result = loader.Load(visual_map_path, level.size);
    if (!result.ok) {
      if (options_.visual_pipeline_config.fallback_to_cpp_pipeline) {
        prepared_level_.source = PreparedLevelSource::kCppPipeline;
        report->warnings.push_back(
            "prepared visual_map load failed; falling back to cpp pipeline: " +
            result.error);
        return;
      }
      Fail(result.error.empty() ? "prepared visual_map load failed" :
                             result.error);
      return;
    }

    if (!result.found) {
      if (options_.visual_pipeline_config.fallback_to_cpp_pipeline) {
        prepared_level_.source = PreparedLevelSource::kCppPipeline;
        report->warnings.push_back(
            "prepared visual_map not found path=" +
            visual_map_path.string() + "; falling back to cpp pipeline");
        return;
      }
      Fail("prepared visual_map not found path=" + visual_map_path.string());
      return;
    }

    prepared_level_.prepared_visual_map = result.data;
    prepared_level_.source =
        SourceForMode(options_.visual_pipeline_config.mode);
    prepared_level_.visual_layer_count =
        result.data.visual_layer_count;
    prepared_level_.decoration_count =
        result.data.visual_object_count;
    prepared_level_.render_cache_entry_count =
        result.data.visual_chunk_count;
    AddVisualMapDiagnostics(result.data, report);
    return;
  }

  if (step.name == "Build semantic terrain masks") {
    std::string error;
    if (!RunBuildSemanticMasksStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "semantic mask step failed" : error);
      return;
    }

    AddSemanticMaskDiagnostics(level, prepared_level_.semantic_masks.summary,
                               report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteSemanticMaskArtifacts(
          prepared_level_.semantic_masks, &artifact_error);
      AddDebugArtifactResult("semantic masks", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build terrain regions") {
    std::string error;
    if (!RunBuildTerrainRegionsStep(&prepared_level_, &error)) {
      Fail(error.empty() ? "terrain region step failed" : error);
      return;
    }

    AddTerrainRegionDiagnostics(prepared_level_.terrain_regions.summary,
                                report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteTerrainRegionArtifacts(
          prepared_level_.terrain_regions, &artifact_error);
      AddDebugArtifactResult("terrain regions", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Classify region borders") {
    std::string error;
    if (!RunClassifyRegionBordersStep(&prepared_level_, &error)) {
      Fail(error.empty() ? "region border step failed" : error);
      return;
    }

    AddRegionBorderDiagnostics(prepared_level_.region_borders.summary,
                               report);
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 1);
    return;
  }

  if (step.name == "Build semantic map links") {
    report->summaries.push_back(
        "semantic links objects=" + std::to_string(level.objects.size()) +
        " places=" + std::to_string(level.places.size()) +
        " routes=" + std::to_string(level.routes.size()) +
        " markers=" + std::to_string(level.markers.size()) +
        " zones=" + std::to_string(level.zones.size()) +
        " graph_nodes=" +
        std::to_string(level.world_graph.nodes.size()) +
        " graph_edges=" +
        std::to_string(level.world_graph.edges.size()));
    if (level.routes.empty()) {
      report->warnings.push_back("semantic routes are empty");
    }
    if (level.places.empty()) {
      report->warnings.push_back("semantic places are empty");
    }
    if (level.objects.empty()) {
      report->warnings.push_back("runtime objects are empty");
    }
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteSemanticLinkArtifacts(
          level, &artifact_error);
      AddDebugArtifactResult("semantic links", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build road and path shapes") {
    if (HasPreparedVisualMap(prepared_level_)) {
      report->summaries.push_back(
          "road shapes using prepared visual_map layers=" +
          std::to_string(prepared_level_.visual_layer_count));
      return;
    }

    std::string error;
    if (!RunBuildRoadVisualPlanStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "road visual plan step failed" : error);
      return;
    }

    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 2);
    AddRoadVisualDiagnostics(prepared_level_.road_visual_plan.summary,
                             report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteRoadVisualArtifacts(
          prepared_level_.road_visual_plan, &artifact_error);
      AddDebugArtifactResult("road visual plan", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build forest masses") {
    std::string error;
    if (!RunBuildForestVisualPlanStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "forest visual plan step failed" : error);
      return;
    }

    if (!HasPreparedVisualMap(prepared_level_)) {
      prepared_level_.visual_layer_count =
          std::max(prepared_level_.visual_layer_count, 3);
    }
    AddForestVisualDiagnostics(prepared_level_.forest_visual_plan.summary,
                               report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteForestVisualArtifacts(
          prepared_level_.forest_visual_plan, &artifact_error);
      AddDebugArtifactResult("forest visual plan", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build ruin scene compositions") {
    std::string error;
    if (!RunBuildRuinVisualPlanStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "ruin visual plan step failed" : error);
      return;
    }

    if (!HasPreparedVisualMap(prepared_level_)) {
      prepared_level_.visual_layer_count =
          std::max(prepared_level_.visual_layer_count, 4);
    }
    AddRuinVisualDiagnostics(prepared_level_.ruin_visual_plan.summary,
                             report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteRuinVisualArtifacts(
          prepared_level_.ruin_visual_plan, &artifact_error);
      AddDebugArtifactResult("ruin visual plan", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build water and swamp edges") {
    std::string error;
    if (!RunBuildWaterVisualPlanStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "water visual plan step failed" : error);
      return;
    }

    if (!HasPreparedVisualMap(prepared_level_)) {
      prepared_level_.visual_layer_count =
          std::max(prepared_level_.visual_layer_count, 5);
    }
    AddWaterVisualDiagnostics(prepared_level_.water_visual_plan.summary,
                              report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteWaterVisualArtifacts(
          prepared_level_.water_visual_plan, &artifact_error);
      AddDebugArtifactResult("water visual plan", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Place visual decorations") {
    if (HasPreparedVisualMap(prepared_level_)) {
      report->summaries.push_back(
          "decorations using prepared visual_map objects=" +
          std::to_string(prepared_level_.decoration_count));
      return;
    }

    std::string error;
    if (!RunBuildObjectVisualPlanStep(level, &prepared_level_, &error)) {
      Fail(error.empty() ? "object visual plan step failed" : error);
      return;
    }

    prepared_level_.decoration_count =
        prepared_level_.object_visual_plan.summary.mapped_object_count;
    AddObjectVisualDiagnostics(prepared_level_.object_visual_plan.summary,
                               report);
    if (options_.visual_pipeline_config.write_debug_artifacts) {
      std::string artifact_error;
      const DebugArtifactWriter writer(ResolveDebugOutputPath(options_));
      const bool written = writer.WriteObjectVisualArtifacts(
          prepared_level_.object_visual_plan, &artifact_error);
      AddDebugArtifactResult("object visual plan", written, artifact_error,
                             report);
    }
    return;
  }

  if (step.name == "Build render cache") {
    if (HasPreparedVisualMap(prepared_level_)) {
      report->summaries.push_back(
          "render cache using prepared visual_map chunks=" +
          std::to_string(prepared_level_.render_cache_entry_count));
      return;
    }
    prepared_level_.render_cache_entry_count =
        level.size.width * level.size.height;
    report->summaries.push_back(
        "render cache placeholder entries=" +
        std::to_string(prepared_level_.render_cache_entry_count));
  }
}

void VisualPreparationPipeline::Fail(std::string error) {
  progress_.running = false;
  progress_.failed = true;
  progress_.finished = false;
  progress_.error = std::move(error);
}

}  // namespace sar::visual_pipeline
