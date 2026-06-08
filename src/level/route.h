#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_

/**
 * @file src/level/route.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for route.h.
 */

#include <string>
#include <vector>

namespace sar {

/**
 * @brief One tile-space point in a route polyline.
 */
struct RoutePoint {
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
};

/**
 * @brief Named path or gameplay route loaded from routes.json.
 */
struct Route {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  std::vector<RoutePoint> waypoints;  ///< Waypoints value carried by this data structure.
  std::vector<std::string> tags;  ///< Semantic tags attached to this entity.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_
