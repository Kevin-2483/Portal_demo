#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "../math_types.h"

namespace portal_core
{

  struct TransformComponent
  {
    Vector3 position{0.0f, 0.0f, 0.0f};
    Quaternion rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
    Vector3 scale{1.0f, 1.0f, 1.0f};
  };

  struct RotationComponent
  {
    Vector3 angular_velocity{0.0f, 0.0f, 0.0f}; // 角速度 (弧度/秒)
    bool enabled{true};
  };

} // namespace portal_core

#endif // TRANSFORM_COMPONENT_H
