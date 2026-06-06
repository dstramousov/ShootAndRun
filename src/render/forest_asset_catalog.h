#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_FOREST_ASSET_CATALOG_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_FOREST_ASSET_CATALOG_H_

#include <filesystem>
#include <string>
#include <vector>

#include <raylib.h>

namespace sar {

enum class ForestAssetBand {
  kEdge,
  kMid,
  kDeep,
};

enum class ForestSpriteBand {
  kEdge,
  kMid,
  kDeep,
  kCanopy,
};

struct ForestAssetTexture {
  std::string id;
  std::filesystem::path relative_path;
  Texture2D texture{};
};

/**
 * @brief Owns the runtime forest texture set used by the level renderer.
 *
 * The catalog loads the style-locked PNG pack from assets/visual/forest and
 * provides deterministic asset selection helpers. It does not change gameplay
 * data, collision, routes or prepared map semantics.
 */
class ForestAssetCatalog {
 public:
  ForestAssetCatalog() = default;
  ForestAssetCatalog(const ForestAssetCatalog&) = delete;
  ForestAssetCatalog& operator=(const ForestAssetCatalog&) = delete;

  /**
   * @brief Releases all loaded raylib textures.
   */
  ~ForestAssetCatalog();

  /**
   * @brief Loads the forest asset pack from disk.
   *
   * @param root Directory that contains forest_assets_manifest.json.
   * @param error Error text populated when the pack cannot be loaded.
   * @return True when at least the required tile and tree textures are loaded.
   */
  bool Load(const std::filesystem::path& root, std::string* error);

  /**
   * @brief Releases all loaded textures and resets catalog state.
   */
  void Reset();

  /**
   * @brief Returns true when required assets are loaded.
   *
   * @return Load state flag.
   */
  bool loaded() const { return loaded_; }

  /**
   * @brief Returns the number of loaded textures.
   *
   * @return Loaded texture count.
   */
  int loaded_texture_count() const { return loaded_texture_count_; }

  /**
   * @brief Selects a deterministic ground tile texture.
   *
   * @param band Forest depth band.
   * @param x Tile X coordinate.
   * @param y Tile Y coordinate.
   * @return Selected texture entry or nullptr when unavailable.
   */
  const ForestAssetTexture* PickGround(ForestAssetBand band, int x,
                                       int y) const;

  /**
   * @brief Selects a deterministic edge tile texture.
   *
   * @param north_open True when the north neighbor is not forest.
   * @param south_open True when the south neighbor is not forest.
   * @param east_open True when the east neighbor is not forest.
   * @param west_open True when the west neighbor is not forest.
   * @return Selected texture entry or nullptr when unavailable.
   */
  const ForestAssetTexture* PickEdge(bool north_open, bool south_open,
                                     bool east_open, bool west_open) const;

  /**
   * @brief Selects a deterministic tall forest sprite.
   *
   * @param band Placement band for the sprite.
   * @param x Tile X coordinate.
   * @param y Tile Y coordinate.
   * @return Selected texture entry or nullptr when unavailable.
   */
  const ForestAssetTexture* PickSprite(ForestSpriteBand band, int x,
                                       int y) const;

  /**
   * @brief Returns a shadow texture for tall sprites.
   *
   * @param x Tile X coordinate.
   * @param y Tile Y coordinate.
   * @return Selected shadow texture or nullptr when unavailable.
   */
  const ForestAssetTexture* PickShadow(int x, int y) const;

 private:
  bool LoadTextureList(const std::filesystem::path& root,
                       const std::vector<std::filesystem::path>& paths,
                       std::vector<ForestAssetTexture>* textures,
                       std::string* error);
  const ForestAssetTexture* PickFrom(const std::vector<ForestAssetTexture>& list,
                                     int x, int y, int salt) const;

  bool loaded_ = false;
  int loaded_texture_count_ = 0;
  std::filesystem::path root_;
  std::vector<ForestAssetTexture> grass_tiles_;
  std::vector<ForestAssetTexture> forest_floor_tiles_;
  std::vector<ForestAssetTexture> mid_tiles_;
  std::vector<ForestAssetTexture> deep_tiles_;
  std::vector<ForestAssetTexture> edge_n_tiles_;
  std::vector<ForestAssetTexture> edge_s_tiles_;
  std::vector<ForestAssetTexture> edge_e_tiles_;
  std::vector<ForestAssetTexture> edge_w_tiles_;
  std::vector<ForestAssetTexture> corner_ne_tiles_;
  std::vector<ForestAssetTexture> corner_nw_tiles_;
  std::vector<ForestAssetTexture> corner_se_tiles_;
  std::vector<ForestAssetTexture> corner_sw_tiles_;
  std::vector<ForestAssetTexture> trees_;
  std::vector<ForestAssetTexture> bushes_;
  std::vector<ForestAssetTexture> clusters_;
  std::vector<ForestAssetTexture> canopies_;
  std::vector<ForestAssetTexture> shadows_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_FOREST_ASSET_CATALOG_H_
