#include "visual_pipeline/visual_preparation_pipeline.h"

#include <algorithm>
#include <string>
#include <utility>

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

}  // namespace

void VisualPreparationPipeline::Start(const LevelData& level) {
  steps_ = BuildDefaultSteps();
  progress_ = {};
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
  RunCurrentStep(level, step);

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
                                               const PipelineStepInfo& step) {
  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0) {
    Fail("raw level size is invalid");
    return;
  }

  if (step.name == "Validate raw map data") {
    const int expected_cells = level.size.width * level.size.height;
    if (expected_cells <= 0 ||
        level.cells.size() != static_cast<std::size_t>(expected_cells)) {
      Fail("raw level cell count does not match level dimensions");
    }
    return;
  }

  if (step.name == "Build semantic terrain masks") {
    prepared_level_.semantic_mask_count = 6;
    return;
  }

  if (step.name == "Build terrain regions") {
    prepared_level_.terrain_region_count =
        std::max(1, (level.size.width * level.size.height) / 256);
    return;
  }

  if (step.name == "Smooth region borders") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 1);
    return;
  }

  if (step.name == "Build road and path shapes") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 2);
    return;
  }

  if (step.name == "Build forest masses") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 3);
    return;
  }

  if (step.name == "Build water and swamp edges") {
    prepared_level_.visual_layer_count =
        std::max(prepared_level_.visual_layer_count, 4);
    return;
  }

  if (step.name == "Place visual decorations") {
    prepared_level_.decoration_count =
        std::max(1, (level.size.width * level.size.height) / 64);
    return;
  }

  if (step.name == "Build render cache") {
    prepared_level_.render_cache_entry_count =
        level.size.width * level.size.height;
  }
}

void VisualPreparationPipeline::Fail(std::string error) {
  progress_.running = false;
  progress_.failed = true;
  progress_.finished = false;
  progress_.error = std::move(error);
}

}  // namespace sar::visual_pipeline
