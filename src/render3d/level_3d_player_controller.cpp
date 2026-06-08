#include "render3d/level_3d_player_controller.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string_view>

namespace sar::render3d {
namespace {

bool TextContains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool IsPreferredSpawnMarker(const Marker& marker) {
  return marker.type == "player_spawn" || marker.id == "player_spawn" ||
         TextContains(marker.type, "player_spawn") ||
         TextContains(marker.id, "player_spawn");
}

bool IsFallbackSpawnMarker(const Marker& marker) {
  return marker.type == "start" || marker.id == "start" ||
         TextContains(marker.type, "spawn") || TextContains(marker.id, "spawn");
}

const Marker* FindSpawnMarker(const LevelData& level) {
  for (const Marker& marker : level.markers) {
    if (IsPreferredSpawnMarker(marker)) {
      return &marker;
    }
  }

  for (const Marker& marker : level.markers) {
    if (IsFallbackSpawnMarker(marker)) {
      return &marker;
    }
  }

  return nullptr;
}

bool IsInsideMap(const LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

const RuntimeCell* CellAt(const LevelData& level, int x, int y) {
  if (!IsInsideMap(level, x, y)) {
    return nullptr;
  }
  const int index = y * level.size.width + x;
  if (index < 0 || index >= static_cast<int>(level.cells.size())) {
    return nullptr;
  }
  return &level.cells[static_cast<std::size_t>(index)];
}

std::int8_t HeightAtOrZero(const LevelData& level, int x, int y) {
  const RuntimeCell* cell = CellAt(level, x, y);
  return cell != nullptr ? cell->height : 0;
}

int TileIndexFromPosition(float value) {
  return static_cast<int>(std::floor(value));
}

bool CanEnterTile(const LevelData& level, const Level3DPlayerState& state,
                  float next_x, float next_y) {
  const int x = TileIndexFromPosition(next_x);
  const int y = TileIndexFromPosition(next_y);
  const RuntimeCell* target = CellAt(level, x, y);
  if (target == nullptr) {
    return false;
  }
  if (target->collision || !target->walkable) {
    return false;
  }
  if (target->height < 0 && state.elevation >= 0) {
    return false;
  }

  const int height_delta = std::abs(static_cast<int>(target->height) -
                                    static_cast<int>(state.elevation));
  return height_delta <= state.allowed_step_height;
}

void ApplyMovementAxis(const LevelData& level, float dx, float dy,
                       Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const float next_x = std::clamp(state->tile_x + dx, 0.0F,
                                  static_cast<float>(level.size.width) -
                                      0.001F);
  const float next_y = std::clamp(state->tile_y + dy, 0.0F,
                                  static_cast<float>(level.size.height) -
                                      0.001F);
  if (!CanEnterTile(level, *state, next_x, next_y)) {
    return;
  }

  state->tile_x = next_x;
  state->tile_y = next_y;
  state->elevation = HeightAtOrZero(level, TileIndexFromPosition(state->tile_x),
                                    TileIndexFromPosition(state->tile_y));
}

}  // namespace

void InitializeLevel3DPlayer(const LevelData& level,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const Marker* marker = FindSpawnMarker(level);
  if (marker != nullptr) {
    state->tile_x = static_cast<float>(marker->x) + 0.5F;
    state->tile_y = static_cast<float>(marker->y) + 0.5F;
    state->elevation = HeightAtOrZero(level, marker->x, marker->y);
  } else {
    state->tile_x = static_cast<float>(level.size.width) * 0.5F;
    state->tile_y = static_cast<float>(level.size.height) * 0.5F;
    state->elevation = HeightAtOrZero(
        level, TileIndexFromPosition(state->tile_x),
        TileIndexFromPosition(state->tile_y));
  }
  state->initialized = true;
}

void UpdateLevel3DPlayer(const LevelData& level, const InputState& input,
                         float dt, Level3DPlayerState* state) {
  if (state == nullptr || !state->initialized || level.size.width <= 0 ||
      level.size.height <= 0) {
    return;
  }

  float direction_x = 0.0F;
  float direction_y = 0.0F;
  if (input.left_down) {
    direction_x -= 1.0F;
  }
  if (input.right_down) {
    direction_x += 1.0F;
  }
  if (input.up_down) {
    direction_y -= 1.0F;
  }
  if (input.down_down) {
    direction_y += 1.0F;
  }

  if (direction_x == 0.0F && direction_y == 0.0F) {
    return;
  }

  const float length = std::sqrt(direction_x * direction_x +
                                 direction_y * direction_y);
  direction_x /= length;
  direction_y /= length;

  const float safe_dt = std::clamp(dt, 0.0F, 0.05F);
  const float step = state->move_speed_tiles_per_sec * safe_dt;
  const float dx = direction_x * step;
  const float dy = direction_y * step;

  if (CanEnterTile(level, *state, state->tile_x + dx, state->tile_y + dy)) {
    ApplyMovementAxis(level, dx, dy, state);
    return;
  }

  ApplyMovementAxis(level, dx, 0.0F, state);
  ApplyMovementAxis(level, 0.0F, dy, state);
}

Vector3 Level3DPlayerWorldPosition(const LevelData& level,
                                   const Level3DPlayerState& state,
                                   float tile_world_size,
                                   float elevation_step) {
  const float origin_x = static_cast<float>(level.size.width) * tile_world_size *
                         0.5F;
  const float origin_z = static_cast<float>(level.size.height) *
                         tile_world_size * 0.5F;
  return Vector3{state.tile_x * tile_world_size - origin_x,
                 static_cast<float>(state.elevation) * elevation_step,
                 state.tile_y * tile_world_size - origin_z};
}

std::string Level3DPlayerStateToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "player3d: tile=" << state.tile_x << ',' << state.tile_y
         << " elevation=" << static_cast<int>(state.elevation);
  return stream.str();
}

}  // namespace sar::render3d
