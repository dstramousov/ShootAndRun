#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_MODEL_REGISTRY_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_MODEL_REGISTRY_H_

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace sar::render3d {

/**
 * @brief Model selection algorithm used by a 3D tileset binding.
 */
enum class ModelSelectorMode3D {
  kFixed,
  kRandom,
  kWeightedRandom,
  kNamed,
  kByTag,
};

/**
 * @brief Returns the stable configuration name of a model selector mode.
 *
 * @param mode Selector mode.
 * @return Lowercase configuration name.
 */
const char* ModelSelectorMode3DName(ModelSelectorMode3D mode);

/**
 * @brief Placement strategy used when a semantic tile generates model instances.
 */
enum class ModelPlacementMode3D {
  kSingle,
  kCluster,
};

/**
 * @brief Returns the stable configuration name of a model placement mode.
 *
 * @param mode Placement mode.
 * @return Lowercase configuration name.
 */
const char* ModelPlacementMode3DName(ModelPlacementMode3D mode);

/**
 * @brief Physical 3D model asset registered in the asset library.
 *
 * This structure describes a model file and its default transform metadata only.
 * It does not contain map-specific placement rules.
 */
struct ModelAsset3D {
  std::string id;
  std::filesystem::path path;
  std::vector<std::string> tags;
  float default_scale = 1.0F;
  float vertical_offset = 0.0F;
  std::string fallback = "cube_debug";
};

/**
 * @brief Weighted model reference used by random and weighted-random bindings.
 */
struct ModelVariant3D {
  std::string model_id;
  int weight = 1;
};

/**
 * @brief Tileset rule that binds a semantic map key to one or more models.
 */
struct ModelBinding3D {
  std::string semantic_key;
  ModelPlacementMode3D placement = ModelPlacementMode3D::kSingle;
  ModelSelectorMode3D selector = ModelSelectorMode3D::kWeightedRandom;
  std::string fixed_model_id;
  std::vector<ModelVariant3D> variants;
  std::vector<std::string> required_tags;
  int count_min = 1;
  int count_max = 1;
  bool random_rotation = false;
  float scale_min = 1.0F;
  float scale_max = 1.0F;
  float offset_min = 0.0F;
  float offset_max = 0.0F;
  float vertical_offset = 0.0F;
  std::string fallback = "cube_debug";
};

/**
 * @brief Named 3D tileset with semantic bindings for one visual theme.
 */
struct Tileset3D {
  std::string id = "dark_forest_3d";
  std::map<std::string, ModelBinding3D> bindings;
};

/**
 * @brief Compact summary of a loaded 3D model registry.
 */
struct ModelRegistry3DSummary {
  int model_count = 0;
  int binding_count = 0;
  int variant_reference_count = 0;
  int missing_reference_count = 0;
};

/**
 * @brief Runtime registry for 3D model assets and active tileset bindings.
 *
 * The registry is only a metadata layer in this patch. It prepares deterministic
 * model selection and fallback decisions, while the current debug primitives
 * remain the renderer fallback until real model drawing is connected.
 */
class ModelRegistry3D {
 public:
  /**
   * @brief Looks up a model asset by id.
   *
   * @param model_id Asset id.
   * @return Pointer to the asset, or nullptr when it is not registered.
   */
  const ModelAsset3D* FindAsset(std::string_view model_id) const;

  /**
   * @brief Looks up a tileset binding by semantic key.
   *
   * @param semantic_key Semantic terrain or object key.
   * @return Pointer to the binding, or nullptr when it is not registered.
   */
  const ModelBinding3D* FindBinding(std::string_view semantic_key) const;

  /**
   * @brief Selects a deterministic model id for a semantic key and tile.
   *
   * The same input seed, semantic key and tile coordinates always produce the
   * same model id. This keeps generated maps stable between runs.
   *
   * @param semantic_key Semantic terrain or object key.
   * @param base_seed Stable map or session seed.
   * @param tile_x Tile x coordinate.
   * @param tile_y Tile y coordinate.
   * @return Selected model id, or an empty string when no concrete model exists.
   */
  std::string SelectModelId(std::string_view semantic_key,
                            std::uint64_t base_seed,
                            int tile_x,
                            int tile_y) const;

  /**
   * @brief Returns aggregate registry counters.
   *
   * @return Registry summary.
   */
  ModelRegistry3DSummary Summary() const;

  /**
   * @brief Returns a readable registry dump for startup logs.
   *
   * @return String summary of loaded assets and bindings.
   */
  std::string Dump() const;

  std::map<std::string, ModelAsset3D> assets;
  Tileset3D tileset;
};

/**
 * @brief Result of loading the 3D model registry configuration files.
 */
struct LoadModelRegistry3DResult {
  bool ok = false;
  bool found = false;
  ModelRegistry3D registry;
  std::vector<std::string> warnings;
  std::string error;
};

/**
 * @brief Loads and validates the 3D asset library and active tileset metadata.
 *
 * @param asset_library_path Path to `asset_library.json`.
 * @param tileset_path Path to the active 3D tileset JSON file.
 * @return Load result with registry metadata, warnings and errors.
 */
LoadModelRegistry3DResult LoadModelRegistry3D(
    const std::filesystem::path& asset_library_path,
    const std::filesystem::path& tileset_path);

/**
 * @brief Builds a deterministic 64-bit seed for tile-local asset selection.
 *
 * @param base_seed Stable map or session seed.
 * @param semantic_key Semantic terrain or object key.
 * @param tile_x Tile x coordinate.
 * @param tile_y Tile y coordinate.
 * @return Mixed deterministic seed.
 */
std::uint64_t DeterministicAssetSeed(std::uint64_t base_seed,
                                     std::string_view semantic_key,
                                     int tile_x,
                                     int tile_y);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_MODEL_REGISTRY_H_
