#include "visual_pipeline/visual_preparation_pipeline.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/steps/build_semantic_masks_step.h"

namespace sar::visual_pipeline {
namespace {

std::vector<PipelineStepInfo> BuildDefaultSteps() {
  return {
      {"Load raw map package"},
      {"Validate raw map data"},
      {"Build semantic terrain masks"},
      {"Build terrain regions"},
      {"Smooth region borders"},
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

void AddSemanticMaskDiagnostics(const SemanticMaskSummary& summary,
                                PipelineStepReport* report) {
  if (report == nullptr) {
    return;
  }

  report->summaries.push_back(TerrainSummaryLine(summary));
  report->summaries.push_back(RuntimeSummaryLine(summary));
  if (summary.unknown_tiles > 0) {
    report->warnings.push_back("unknown terrain tiles=" +
                               std::to_string(summary.unknown_tiles));
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

    AddSemanticMaskDiagnostics(prepared_level_.semantic_masks.summary,
                               report);
    return;
  }

  if (step.name == "Build terrain regions") {
    prepared_level_.terrain_region_count =
        std::max(1, (level.size.width * level.size.height) / 256);
    report->summaries.push_back(
        "region builder placeholder regions=" +
        std::to_string(prepared_level_.terrain_region_count));
    return;
  }

  if (step.name == "Smooth region borders") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 1);
    report->summaries.push_back("border smoothing placeholder layers=" +
                                std::to_string(
                                    prepared_level_.visual_layer_count));
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
