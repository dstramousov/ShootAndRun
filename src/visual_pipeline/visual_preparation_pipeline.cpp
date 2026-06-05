#include "visual_pipeline/visual_preparation_pipeline.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>
#include <utility>
#include <vector>

#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/steps/build_semantic_masks_step.h"
#include "visual_pipeline/steps/classify_region_borders_step.h"
#include "visual_pipeline/steps/build_terrain_regions_step.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/region_borders.h"

namespace sar::visual_pipeline {
namespace {

std::vector<PipelineStepInfo> BuildDefaultSteps() {
  return {
      {"Load raw map package"},
      {"Validate raw map data"},
      {"Build semantic terrain masks"},
      {"Build terrain regions"},
      {"Classify region borders"},
      {"Build road and path shapes"},
      {"Build forest masses"},
      {"Build water and swamp edges"},
      {"Place visual decorations"},
      {"Build render cache"},
  };
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

}  // namespace

void VisualPreparationPipeline::Start(const LevelData& level) {
  steps_ = BuildDefaultSteps();
  progress_ = {};
  last_step_report_ = {};
  prepared_level_ = {};
  prepared_level_.size = level.size;
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

  if (step.name == "Build road and path shapes") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 2);
    report->summaries.push_back("road shape placeholder layers=" +
                                std::to_string(
                                    prepared_level_.visual_layer_count));
    return;
  }

  if (step.name == "Build forest masses") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 3);
    report->summaries.push_back("forest mass placeholder layers=" +
                                std::to_string(
                                    prepared_level_.visual_layer_count));
    return;
  }

  if (step.name == "Build water and swamp edges") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 4);
    report->summaries.push_back("water edge placeholder layers=" +
                                std::to_string(
                                    prepared_level_.visual_layer_count));
    return;
  }

  if (step.name == "Place visual decorations") {
    prepared_level_.decoration_count =
        std::max(1, (level.size.width * level.size.height) / 64);
    report->summaries.push_back(
        "decoration placeholder count=" +
        std::to_string(prepared_level_.decoration_count));
    return;
  }

  if (step.name == "Build render cache") {
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
