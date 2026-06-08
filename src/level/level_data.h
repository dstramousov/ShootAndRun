#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_

/**
 * @file src/level/level_data.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for level_data.h.
 */

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
  int width = 0;  ///< Size component for width.
  int height = 0;  ///< Signed elevation level for this tile or object.
  int tile_size = 16;  ///< Tile size value carried by this data structure.
};

/**
 * @brief Complete runtime representation of a loaded level package.
 *
 * The structure stores terrain/runtime cells as a flat row-major array and
 * keeps higher-level gameplay data, such as routes, markers and elevation
 * transitions, in stable vectors matching the source package order.
 */
struct LevelData {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<RuntimeCell> cells;  ///< Cells value carried by this data structure.
  std::vector<RuntimeObject> objects;  ///< Objects value carried by this data structure.
  std::vector<Place> places;  ///< Places value carried by this data structure.
  std::vector<Marker> markers;  ///< Markers value carried by this data structure.
  std::vector<Route> routes;  ///< Routes value carried by this data structure.
  std::vector<ElevationTransition> elevation_transitions;  ///< Elevation transitions value carried by this data structure.
  std::vector<GameplayZone> zones;  ///< Zones value carried by this data structure.
  WorldGraph world_graph;  ///< World graph value carried by this data structure.
  std::map<std::string, int> terrain_type_counts;  ///< Count of terrain type counts entries or events.
  std::map<std::string, int> unknown_terrain_type_counts;  ///< Count of unknown terrain type counts entries or events.
  int tile_catalog_type_count = 0;  ///< Count of tile catalog type count entries or events.
  bool used_tile_catalog = false;  ///< Used tile catalog value carried by this data structure.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_DATA_H_
