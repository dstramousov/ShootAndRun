#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_

#include <string>
#include <vector>

namespace sar {

struct GameplayZone {
  std::string id;
  std::string type;
  std::string shape;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  int radius = 0;
  std::vector<std::string> tags;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_
