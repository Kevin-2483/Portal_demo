#pragma once

#include <cstdio>
#include "entt/entt.hpp"
#include "../components/x_rotation_component.h"
#include "../system_base.h"
#include "../math_constants.h"
#include "../debug/portal_debug_logging.h"

namespace portal_core
{

  /**
   * X軸旋轉系統 - 專門處理 X 軸旋轉邏輯
   * 🌟 現在完全自主管理 X 軸旋轉狀態，不依賴 TransformComponent
   */
  class XRotationSystem : public ISystem
  {
  public:
    /**
     * 更新所有擁有 XRotationComponent 的實體
     * @param registry ECS 注册表
     * @param delta_time 自上一幀以來的時間（秒）
     */
    void update(entt::registry &registry, float delta_time) override
    {
      // 🎯 新策略：只需要 XRotationComponent，不依賴 TransformComponent
      auto view = registry.view<XRotationComponent>();

      // 調試：計算並打印找到的實體數量
      size_t entity_count = 0;

      // 為每個實體更新其 X 軸旋轉狀態
      for (auto [entity, x_rotation] : view.each())
      {
        entity_count++;

        // 根據速度和時間差更新當前旋轉值
        x_rotation.current_rotation += x_rotation.speed * delta_time;
        
        // 保持旋轉值在合理範圍內 (-2π 到 2π)
        while (x_rotation.current_rotation > 2.0f * M_PI)
          x_rotation.current_rotation -= 2.0f * M_PI;
        while (x_rotation.current_rotation < -2.0f * M_PI)
          x_rotation.current_rotation += 2.0f * M_PI;

      }
    }

    const char* get_name() const override
    {
      return "XRotationSystem";
    }

    std::vector<std::string> get_dependencies() const override
    {
      return {}; // X軸旋轉系統沒有依賴
    }
  };

  // 自動註冊系統
  REGISTER_SYSTEM_SIMPLE(XRotationSystem, 100);

} // namespace portal_core
