#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FINAL_VISUAL_PACKAGE_WRITER_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FINAL_VISUAL_PACKAGE_WRITER_H_

#include <filesystem>
#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

struct FinalVisualPackageResult {
  std::filesystem::path visual_map_path;
  std::filesystem::path visual_layers_path;
  std::filesystem::path visual_objects_path;
  std::filesystem::path visual_chunks_path;
  std::filesystem::path final_render_path;
  int visual_layer_count = 0;
  int unique_tile_id_count = 0;
  int visual_object_count = 0;
  int visual_chunk_count = 0;
};

/**
 * @brief Writes the final visual package produced by the C++ pipeline.
 *
 * The writer serializes visual layer, object, chunk and quality-report data
 * without modifying gameplay grids, markers, routes or collision data.
 */
class FinalVisualPackageWriter {
 public:
  /**
   * @brief Creates a writer rooted at a prepared-map output directory.
   *
   * @param output_root Directory where visual package files are written.
   */
  explicit FinalVisualPackageWriter(std::filesystem::path output_root);

  /**
   * @brief Writes the final package files for a prepared level.
   *
   * @param level Raw level data used as the gameplay source of truth.
   * @param prepared_level Visual pipeline output to serialize.
   * @param error Error text populated on failure.
   * @return Written file paths and counters.
   */
  FinalVisualPackageResult Write(const LevelData& level,
                                 const PreparedLevel& prepared_level,
                                 std::string* error) const;

 private:
  std::filesystem::path output_root_;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FINAL_VISUAL_PACKAGE_WRITER_H_
