#include "render/forest_asset_catalog.h"

#include <raylib.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sar {
namespace {

std::uint32_t HashTile(int x, int y, int salt) {
  std::uint32_t value = static_cast<std::uint32_t>(x) * 0x9E3779B1U;
  value ^= static_cast<std::uint32_t>(y) * 0x85EBCA77U;
  value ^= static_cast<std::uint32_t>(salt) * 0xC2B2AE3DU;
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  value *= 0x846CA68BU;
  value ^= value >> 16U;
  return value;
}

std::string TextureIdFromPath(const std::filesystem::path& path) {
  std::string id = path.generic_string();
  std::replace(id.begin(), id.end(), '/', '.');
  const std::string suffix = ".png";
  if (id.size() >= suffix.size() &&
      id.substr(id.size() - suffix.size()) == suffix) {
    id.resize(id.size() - suffix.size());
  }
  return "forest." + id;
}

bool FileExists(const std::filesystem::path& path, std::string* error) {
  std::error_code code;
  if (std::filesystem::exists(path, code)) {
    return true;
  }
  if (error != nullptr) {
    *error = code ? "failed to inspect " + path.string() + ": " +
                        code.message()
                  : "file does not exist: " + path.string();
  }
  return false;
}

std::vector<std::filesystem::path> NumberedPaths(std::string_view prefix,
                                                 int first, int last,
                                                 std::string_view suffix) {
  std::vector<std::filesystem::path> paths;
  for (int i = first; i <= last; ++i) {
    const std::string index = i < 10 ? "0" + std::to_string(i)
                                     : std::to_string(i);
    paths.push_back(std::string(prefix) + index + std::string(suffix));
  }
  return paths;
}

}  // namespace

ForestAssetCatalog::~ForestAssetCatalog() { Reset(); }

bool ForestAssetCatalog::Load(const std::filesystem::path& root,
                              std::string* error) {
  Reset();

  const std::filesystem::path manifest_path =
      root / "forest_assets_manifest.json";
  if (!FileExists(manifest_path, error)) {
    return false;
  }

  root_ = root;
  bool ok = true;
  ok = LoadTextureList(root, {"tiles/grass_01.png", "tiles/grass_02.png",
                              "tiles/grass_03.png", "tiles/grass_moss.png"},
                       &grass_tiles_, error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/forest_floor_01.png",
                              "tiles/forest_floor_02.png",
                              "tiles/forest_floor_03.png",
                              "tiles/forest_floor_04.png"},
                       &forest_floor_tiles_, error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/mid_01.png", "tiles/mid_02.png"},
                       &mid_tiles_, error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/deep_01.png", "tiles/deep_02.png",
                              "tiles/deep_03.png"},
                       &deep_tiles_, error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/edge_n.png"}, &edge_n_tiles_, error) && ok;
  ok = LoadTextureList(root, {"tiles/edge_s.png"}, &edge_s_tiles_, error) && ok;
  ok = LoadTextureList(root, {"tiles/edge_e.png"}, &edge_e_tiles_, error) && ok;
  ok = LoadTextureList(root, {"tiles/edge_w.png"}, &edge_w_tiles_, error) && ok;
  ok = LoadTextureList(root, {"tiles/corner_ne.png"}, &corner_ne_tiles_,
                       error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/corner_nw.png"}, &corner_nw_tiles_,
                       error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/corner_se.png"}, &corner_se_tiles_,
                       error) &&
       ok;
  ok = LoadTextureList(root, {"tiles/corner_sw.png"}, &corner_sw_tiles_,
                       error) &&
       ok;

  std::vector<std::filesystem::path> tree_paths;
  for (int i = 1; i <= 20; ++i) {
    const std::string index = i < 10 ? "0" + std::to_string(i)
                                     : std::to_string(i);
    tree_paths.push_back("sprites/trees/single_tree_" + index + ".png");
  }
  ok = LoadTextureList(root, tree_paths, &trees_, error) && ok;

  ok = LoadTextureList(root, {"sprites/bushes/bush_01.png",
                              "sprites/bushes/bush_02.png",
                              "sprites/bushes/bush_03.png",
                              "sprites/bushes/bush_04.png",
                              "sprites/bushes/bush_05.png",
                              "sprites/bushes/bush_06.png"},
                       &bushes_, error) &&
       ok;
  ok = LoadTextureList(root, {"sprites/clusters/cluster_2x2_01.png",
                              "sprites/clusters/cluster_2x2_02.png",
                              "sprites/clusters/cluster_3x2_01.png",
                              "sprites/clusters/cluster_3x3_01.png",
                              "sprites/clusters/cluster_3x3_02.png"},
                       &clusters_, error) &&
       ok;
  ok = LoadTextureList(root, {"overlays/canopy_01.png",
                              "overlays/canopy_02.png",
                              "overlays/canopy_03.png",
                              "overlays/canopy_04.png"},
                       &canopies_, error) &&
       ok;
  ok = LoadTextureList(root, {"overlays/shadow_01.png",
                              "overlays/shadow_02.png",
                              "overlays/shadow_03.png"},
                       &shadows_, error) &&
       ok;

  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/ground/dark_grass_", 1, 8,
                                        ".png"),
                          &fringe_ground_);
  LoadOptionalTextureList(root, NumberedPaths("fringe/ground/moss_", 1, 6,
                                             ".png"),
                          &fringe_ground_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/ground/leaf_litter_", 1, 6,
                                        ".png"),
                          &fringe_ground_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/ground/needle_litter_", 1, 6,
                                        ".png"),
                          &fringe_ground_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/bushes/small_bush_", 1, 10,
                                        ".png"),
                          &fringe_bushes_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/ferns/fern_fringe_", 1, 8,
                                        ".png"),
                          &fringe_ferns_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/saplings/sapling_", 1, 8,
                                        ".png"),
                          &fringe_saplings_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/wood/branch_fringe_", 1, 6,
                                        ".png"),
                          &fringe_wood_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/wood/stump_fringe_", 1, 4,
                                        ".png"),
                          &fringe_wood_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/wood/fallen_log_small_", 1,
                                        4, ".png"),
                          &fringe_wood_);
  LoadOptionalTextureList(root, NumberedPaths("fringe/wood/roots_", 1, 5,
                                             ".png"),
                          &fringe_wood_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/rocks/rock_moss_", 1, 4,
                                        ".png"),
                          &fringe_rocks_);
  LoadOptionalTextureList(root,
                          NumberedPaths("fringe/shadows/edge_shadow_blob_", 1,
                                        8, ".png"),
                          &fringe_shadows_);
  LoadOptionalTextureList(root,
                          NumberedPaths(
                              "large_canopy/clusters/large_cluster_3x3_",
                              1, 10, ".png"),
                          &large_clusters_3x3_);
  LoadOptionalTextureList(root,
                          NumberedPaths(
                              "large_canopy/clusters/large_cluster_4x3_",
                              1, 6, ".png"),
                          &large_clusters_4x3_);
  LoadOptionalTextureList(root,
                          NumberedPaths(
                              "large_canopy/canopy/deep_canopy_mass_3x3_",
                              1, 8, ".png"),
                          &large_canopy_masses_3x3_);
  LoadOptionalTextureList(root,
                          NumberedPaths(
                              "large_canopy/shadows/deep_shadow_3x3_", 1,
                              6, ".png"),
                          &large_deep_shadows_3x3_);
  LoadOptionalTextureList(root,
                          NumberedPaths(
                              "large_canopy/edge_breaks/edge_canopy_break_2x2_",
                              1, 6, ".png"),
                          &large_edge_breaks_2x2_);

  loaded_ = ok && !forest_floor_tiles_.empty() && !mid_tiles_.empty() &&
            !deep_tiles_.empty() && !trees_.empty();
  if (!loaded_ && error != nullptr && error->empty()) {
    *error = "forest asset pack is incomplete: " + root.string();
  }
  return loaded_;
}

void ForestAssetCatalog::Reset() {
  const auto unload = [](std::vector<ForestAssetTexture>* textures) {
    for (ForestAssetTexture& item : *textures) {
      if (item.texture.id != 0U) {
        UnloadTexture(item.texture);
        item.texture = Texture2D{};
      }
    }
    textures->clear();
  };

  unload(&grass_tiles_);
  unload(&forest_floor_tiles_);
  unload(&mid_tiles_);
  unload(&deep_tiles_);
  unload(&edge_n_tiles_);
  unload(&edge_s_tiles_);
  unload(&edge_e_tiles_);
  unload(&edge_w_tiles_);
  unload(&corner_ne_tiles_);
  unload(&corner_nw_tiles_);
  unload(&corner_se_tiles_);
  unload(&corner_sw_tiles_);
  unload(&trees_);
  unload(&bushes_);
  unload(&clusters_);
  unload(&canopies_);
  unload(&shadows_);
  unload(&fringe_ground_);
  unload(&fringe_bushes_);
  unload(&fringe_ferns_);
  unload(&fringe_saplings_);
  unload(&fringe_wood_);
  unload(&fringe_rocks_);
  unload(&fringe_shadows_);
  unload(&large_clusters_3x3_);
  unload(&large_clusters_4x3_);
  unload(&large_canopy_masses_3x3_);
  unload(&large_deep_shadows_3x3_);
  unload(&large_edge_breaks_2x2_);

  loaded_texture_count_ = 0;
  loaded_ = false;
  root_.clear();
}

const ForestAssetTexture* ForestAssetCatalog::PickGround(
    ForestAssetBand band, int x, int y) const {
  switch (band) {
    case ForestAssetBand::kEdge:
      return PickFrom(forest_floor_tiles_.empty() ? grass_tiles_
                                                  : forest_floor_tiles_,
                      x, y, 11);
    case ForestAssetBand::kMid:
      return PickFrom(mid_tiles_.empty() ? forest_floor_tiles_ : mid_tiles_,
                      x, y, 23);
    case ForestAssetBand::kDeep:
      return PickFrom(deep_tiles_.empty() ? mid_tiles_ : deep_tiles_, x, y,
                      37);
  }
  return nullptr;
}

const ForestAssetTexture* ForestAssetCatalog::PickEdge(
    bool north_open, bool south_open, bool east_open, bool west_open) const {
  if (north_open && east_open) {
    return PickFrom(corner_ne_tiles_, 0, 0, 41);
  }
  if (north_open && west_open) {
    return PickFrom(corner_nw_tiles_, 0, 0, 43);
  }
  if (south_open && east_open) {
    return PickFrom(corner_se_tiles_, 0, 0, 47);
  }
  if (south_open && west_open) {
    return PickFrom(corner_sw_tiles_, 0, 0, 53);
  }
  if (north_open) {
    return PickFrom(edge_n_tiles_, 0, 0, 59);
  }
  if (south_open) {
    return PickFrom(edge_s_tiles_, 0, 0, 61);
  }
  if (east_open) {
    return PickFrom(edge_e_tiles_, 0, 0, 67);
  }
  if (west_open) {
    return PickFrom(edge_w_tiles_, 0, 0, 71);
  }
  return nullptr;
}

const ForestAssetTexture* ForestAssetCatalog::PickSprite(
    ForestSpriteBand band, int x, int y) const {
  switch (band) {
    case ForestSpriteBand::kEdge:
      if (!bushes_.empty() && HashTile(x, y, 79) % 3U == 0U) {
        return PickFrom(bushes_, x, y, 83);
      }
      return PickFrom(trees_, x, y, 89);
    case ForestSpriteBand::kMid:
      return PickFrom(trees_, x, y, 97);
    case ForestSpriteBand::kDeep:
      return PickFrom(trees_, x, y, 101);
    case ForestSpriteBand::kCanopy:
      if (!clusters_.empty() && HashTile(x, y, 103) % 2U == 0U) {
        return PickFrom(clusters_, x, y, 107);
      }
      return PickFrom(canopies_, x, y, 109);
  }
  return nullptr;
}

const ForestAssetTexture* ForestAssetCatalog::PickShadow(int x, int y) const {
  return PickFrom(shadows_, x, y, 113);
}

const ForestAssetTexture* ForestAssetCatalog::PickFringe(
    ForestFringeAssetKind kind, int x, int y) const {
  switch (kind) {
    case ForestFringeAssetKind::kGround:
      return PickFrom(fringe_ground_, x, y, 229);
    case ForestFringeAssetKind::kBush:
      return PickFrom(fringe_bushes_, x, y, 233);
    case ForestFringeAssetKind::kFern:
      return PickFrom(fringe_ferns_, x, y, 239);
    case ForestFringeAssetKind::kSapling:
      return PickFrom(fringe_saplings_, x, y, 241);
    case ForestFringeAssetKind::kWood:
      return PickFrom(fringe_wood_, x, y, 251);
    case ForestFringeAssetKind::kRock:
      return PickFrom(fringe_rocks_, x, y, 257);
    case ForestFringeAssetKind::kShadow:
      return PickFrom(fringe_shadows_, x, y, 263);
  }
  return nullptr;
}

const ForestAssetTexture* ForestAssetCatalog::PickLargeCanopy(
    ForestLargeCanopyAssetKind kind, int x, int y) const {
  switch (kind) {
    case ForestLargeCanopyAssetKind::kCluster3x3:
      return PickFrom(large_clusters_3x3_, x, y, 269);
    case ForestLargeCanopyAssetKind::kCluster4x3:
      return PickFrom(large_clusters_4x3_, x, y, 271);
    case ForestLargeCanopyAssetKind::kCanopyMass3x3:
      return PickFrom(large_canopy_masses_3x3_, x, y, 277);
    case ForestLargeCanopyAssetKind::kDeepShadow3x3:
      return PickFrom(large_deep_shadows_3x3_, x, y, 281);
    case ForestLargeCanopyAssetKind::kEdgeBreak2x2:
      return PickFrom(large_edge_breaks_2x2_, x, y, 283);
  }
  return nullptr;
}

bool ForestAssetCatalog::LoadTextureList(
    const std::filesystem::path& root,
    const std::vector<std::filesystem::path>& paths,
    std::vector<ForestAssetTexture>* textures,
    std::string* error) {
  if (textures == nullptr) {
    return false;
  }

  bool ok = true;
  for (const std::filesystem::path& relative_path : paths) {
    const std::filesystem::path full_path = root / relative_path;
    if (!FileExists(full_path, error)) {
      ok = false;
      continue;
    }

    Texture2D texture = LoadTexture(full_path.string().c_str());
    if (texture.id == 0U) {
      if (error != nullptr) {
        *error = "failed to load forest texture: " + full_path.string();
      }
      ok = false;
      continue;
    }

    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    textures->push_back(ForestAssetTexture{TextureIdFromPath(relative_path),
                                           relative_path, texture});
    ++loaded_texture_count_;
  }
  return ok;
}

void ForestAssetCatalog::LoadOptionalTextureList(
    const std::filesystem::path& root,
    const std::vector<std::filesystem::path>& paths,
    std::vector<ForestAssetTexture>* textures) {
  std::string ignored_error;
  static_cast<void>(LoadTextureList(root, paths, textures, &ignored_error));
}

const ForestAssetTexture* ForestAssetCatalog::PickFrom(
    const std::vector<ForestAssetTexture>& list, int x, int y, int salt) const {
  if (list.empty()) {
    return nullptr;
  }
  const std::uint32_t hash = HashTile(x, y, salt);
  const auto index = static_cast<std::size_t>(hash % list.size());
  return &list[index];
}

}  // namespace sar
