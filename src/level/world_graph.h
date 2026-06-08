#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_

#include <string>
#include <vector>

namespace sar {

/**
 * @brief Semantic node in the level world graph.
 */
struct GraphNode {
  std::string id;
  std::string type;
  int x = 0;
  int y = 0;
};

/**
 * @brief Directed or logical connection between two world graph nodes.
 */
struct GraphEdge {
  std::string from;
  std::string to;
  std::string type;
  float cost = 1.0F;
};

/**
 * @brief High-level connectivity graph loaded from world_graph.json.
 */
struct WorldGraph {
  std::vector<GraphNode> nodes;
  std::vector<GraphEdge> edges;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
