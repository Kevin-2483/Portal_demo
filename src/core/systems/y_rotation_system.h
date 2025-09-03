#pragma once

#include <cstdio>
#include <cmath>
#include "entt/entt.hpp"
#include "../components/y_rotation_component.h"

namespace portal_core
{

  /**
   * Y軸旋轉系統 - 專門處理 Y 軸旋轉邏輯
   * 🌟 現在完全自主管理 Y 軸旋轉狀態，不依賴 TransformComponent
   */
  class YRotationSystem
  {
  public:
    /**
     * 更新所有擁有 YRotationComponent 的實體
     * @param registry ECS 注册表
     * @param delta_time 自上一幀以來的時間（秒）
     */
    static void update(entt::registry &registry, float delta_time)
    {
      // 🎯 新策略：只需要 YRotationComponent，不依賴 TransformComponent
      auto view = registry.view<YRotationComponent>();

      // 調試：計算並打印找到的實體數量
      size_t entity_count = 0;

      // 為每個實體更新其 Y 軸旋轉狀態
      for (auto [entity, y_rotation] : view.each())
      {
        entity_count++;

        // 根據速度和時間差更新當前旋轉值
        y_rotation.current_rotation += y_rotation.speed * delta_time;
        
        // 保持旋轉值在合理範圍內 (-2π 到 2π)
        while (y_rotation.current_rotation > 2.0f * M_PI)
          y_rotation.current_rotation -= 2.0f * M_PI;
        while (y_rotation.current_rotation < -2.0f * M_PI)
          y_rotation.current_rotation += 2.0f * M_PI;

        printf("YRotationSystem: Entity %u Y-rotation: %f radians (speed: %f)\n",
               static_cast<unsigned>(entity), y_rotation.current_rotation, y_rotation.speed);
      }

      if (entity_count > 0)
      {
        printf("YRotationSystem: Updated %zu entities with Y-axis rotation\n", entity_count);
      }
    }
  };

} // namespace portal_core
