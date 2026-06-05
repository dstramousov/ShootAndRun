#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_DEBUG_ARTIFACT_WRITER_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_DEBUG_ARTIFACT_WRITER_H_

#include <filesystem>
#include <string>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

/**
 * @brief Writes visual pipeline debug artifacts to disk.
 *
 * The writer produces machine-readable JSON reports and PNG images for the
 * first visual-normalization checkpoint. It does not modify gameplay data.
 */
class DebugArtifactWriter {
 public:
  /**
   * @brief Creates a writer rooted at the debug output directory.
   *
   * @param output_root Directory where debug subdirectories are created.
   */
  explicit DebugArtifactWriter(std::filesystem::path output_root);

  /**
   * @brief Writes the raw input validation report.
   *
   * @param level Loaded level data.
   * @param error Error text populated on failure.
   * @return True when the report was written successfully.
   */
  bool WriteInputValidationReport(const LevelData& level,
                                  std::string* error) const;

  /**
   * @brief Writes semantic mask PNGs and a JSON report.
   *
   * @param masks Semantic masks built from the raw level.
   * @param error Error text populated on failure.
   * @return True when all artifacts were written successfully.
   */
  bool WriteSemanticMaskArtifacts(const SemanticMasks& masks,
                                  std::string* error) const;

  /**
   * @brief Writes semantic link PNGs and a JSON report.
   *
   * @param level Loaded level data with optional semantic layers.
   * @param error Error text populated on failure.
   * @return True when all artifacts were written successfully.
   */
  bool WriteSemanticLinkArtifacts(const LevelData& level,
                                  std::string* error) const;

  /**
   * @brief Writes terrain region PNGs and a JSON report.
   *
   * @param regions Connected terrain regions built from semantic masks.
   * @param error Error text populated on failure.
   * @return True when all artifacts were written successfully.
   */
  bool WriteTerrainRegionArtifacts(const TerrainRegions& regions,
                                   std::string* error) const;

 private:
  std::filesystem::path output_root_;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_DEBUG_ARTIFACT_WRITER_H_
