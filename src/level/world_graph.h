#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_

/**
 * @file src/level/world_graph.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for world_graph.h.
 */

#include <string>
#include <vector>

namespace sar {

/**
 * @brief Semantic node in the level world graph.
 */
struct GraphNode {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
};

/**
 * @brief Directed or logical connection between two world graph nodes.
 */
struct GraphEdge {
  std::string from;  ///< From value carried by this data structure.
  std::string to;  ///< To value carried by this data structure.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  float cost = 1.0F;  ///< Cost value carried by this data structure.
};

/**
 * @brief High-level connectivity graph loaded from world_graph.json.
 */
struct WorldGraph {
  std::vector<GraphNode> nodes;  ///< Nodes value carried by this data structure.
  std::vector<GraphEdge> edges;  ///< Edges value carried by this data structure.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_WORLD_GRAPH_H_
