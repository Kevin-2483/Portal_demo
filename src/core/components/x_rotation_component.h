#pragma once

namespace portal_core
{

  /**
   * X軸旋轉組件 - 純數據容器
   * 🌟 現在由 ECS 系統完全管理旋轉狀態
   */
  struct XRotationComponent
  {
    float speed = 1.0f;           // 每秒旋轉的弧度數
    float current_rotation = 0.0f; // 當前的 X 軸旋轉值（弧度）

    XRotationComponent() = default;
    explicit XRotationComponent(float rotation_speed) : speed(rotation_speed), current_rotation(0.0f) {}
    XRotationComponent(float rotation_speed, float initial_rotation) : speed(rotation_speed), current_rotation(initial_rotation) {}
  };

} // namespace portal_core
