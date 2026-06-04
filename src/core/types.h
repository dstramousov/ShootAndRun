#ifndef SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
#define SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_

namespace sar {

struct Vec2 {
  float x = 0.0F;
  float y = 0.0F;
};

struct Rect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
