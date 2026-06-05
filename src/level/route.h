#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_

#include <string>
#include <vector>

namespace sar {

struct RoutePoint {
  int x = 0;
  int y = 0;
};

struct Route {
  std::string id;
  std::string type;
  std::vector<RoutePoint> waypoints;
  std::vector<std::string> tags;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_ROUTE_H_
