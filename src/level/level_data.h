#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_

#include <map>
#include <string>
#include <vector>

#include "level/elevation_transition.h"
#include "level/gameplay_zone.h"
#include "level/marker.h"
#include "level/place.h"
#include "level/runtime_grid.h"
#include "level/runtime_object.h"
#include "level/route.h"
#include "level/world_graph.h"

namespace sar {

/**
 * @brief Dimensions of a loaded tile-map level.
 */
struct LevelSize {
  int width = 0;
  int height = 0;
  int tile_size = 16;
};

/**
 * @brief Complete runtime representation of a loaded level package.
 *
 * The structure stores terrain/runtime cells as a flat row-major array and
 * keeps higher-level gameplay data, such as routes, markers and elevation
 * transitions, in stable vectors matching the source package order.
 */
struct LevelData {
  LevelSize size;
  std::vector<RuntimeCell> cells;
  std::vector<RuntimeObject> objects;
  std::vector<Place> places;
  std::vector<Marker> markers;
  std::vector<Route> routes;
  std::vector<ElevationTransition> elevation_transitions;
  std::vector<GameplayZone> zones;
  WorldGraph world_graph;
  std::map<std::string, int> terrain_type_counts;
  std::map<std::string, int> unknown_terrain_type_counts;
  int tile_catalog_type_count = 0;
  bool used_tile_catalog = false;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_
