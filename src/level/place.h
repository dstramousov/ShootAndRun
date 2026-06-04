#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_

#include <string>
#include <vector>

namespace sar {

struct Place {
  std::string id;
  std::string type;
  std::string name;
  int x = 0;
  int y = 0;
  int radius = 0;
  std::vector<std::string> tags;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_
