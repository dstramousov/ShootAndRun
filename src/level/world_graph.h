#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_

#include <string>
#include <vector>

namespace sar {

struct GraphNode {
  std::string id;
  std::string type;
  int x = 0;
  int y = 0;
};

struct GraphEdge {
  std::string from;
  std::string to;
  std::string type;
  float cost = 1.0F;
};

struct WorldGraph {
  std::vector<GraphNode> nodes;
  std::vector<GraphEdge> edges;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
