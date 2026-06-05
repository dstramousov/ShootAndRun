#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_

#include <filesystem>
#include <string>
#include <vector>

#include "level/level_data.h"

namespace sar::visual_pipeline {

struct VisualMapData {
  bool loaded = false;
  std::filesystem::path manifest_path;
  std::filesystem::path base_path;
  std::string schema_version;
  std::string generator_version;
  std::string visual_profile_id;
  std::string visual_profile_name;
  LevelSize size;
  int visual_layer_count = 0;
  int unique_tile_id_count = 0;
  int visual_object_count = 0;
  int visual_chunk_count = 0;
  int chunk_size_tiles = 0;
  bool changes_gameplay = false;
  bool moves_markers = false;
  bool changes_collision = false;
  std::vector<std::string> warnings;

  /**
   * @brief Checks whether the visual map has compatible level dimensions.
   *
   * @param raw_size Raw map package dimensions.
   * @param error Error text populated on mismatch.
   * @return True when dimensions are compatible.
   */
  bool ValidateAgainstRawSize(const LevelSize& raw_size,
                              std::string* error) const;

  /**
   * @brief Returns a readable dump of the prepared visual-map state.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_DATA_H_
