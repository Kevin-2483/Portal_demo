#ifndef ROTATION_SYSTEM_H
#define ROTATION_SYSTEM_H

#include <entt/entt.hpp>
#include "../components/transform_component.h"
#include "../system_base.h"

namespace portal_core
{

  /**
   * 通用旋轉系統
   * 處理具有 TransformComponent 和 RotationComponent 的實體
   */
  class RotationSystem : public ISystem
  {
  public:
    RotationSystem() = default;
    virtual ~RotationSystem() = default;

    // ISystem 接口實現
    bool initialize() override { return true; }
    void cleanup() override {}

    void update(entt::registry& registry, float delta_time) override
    {
      // 更新所有具有 TransformComponent 和 RotationComponent 的實體
      auto view = registry.view<TransformComponent, RotationComponent>();

      for (auto [entity, transform, rotation_comp] : view.each())
      {
        if (!rotation_comp.enabled)
        {
          continue;
        }

        // 計算旋轉增量
        Vector3 angular_displacement = rotation_comp.angular_velocity * delta_time;

        // 將角位移轉換為四元數並應用旋轉
        if (angular_displacement.length() > 0.0f)
        {
          float angle = angular_displacement.length();
          Vector3 axis = angular_displacement.normalized();
          Quaternion delta_rotation = Quaternion::from_axis_angle(axis, angle);

          // 應用旋轉
          transform.rotation = delta_rotation * transform.rotation;
          transform.rotation = transform.rotation.normalized();
        }
      }
    }

    // 設置實體的角速度
    static void set_angular_velocity(entt::registry &registry, entt::entity entity, const Vector3 &velocity)
    {
      if (registry.all_of<RotationComponent>(entity))
      {
        auto &rotation_comp = registry.get<RotationComponent>(entity);
        rotation_comp.angular_velocity = velocity;
      }
    }

    // 啟用/禁用旋轉
    static void set_rotation_enabled(entt::registry &registry, entt::entity entity, bool enabled)
    {
      if (registry.all_of<RotationComponent>(entity))
      {
        auto &rotation_comp = registry.get<RotationComponent>(entity);
        rotation_comp.enabled = enabled;
      }
    }
  };

  // 自動註冊系統
  REGISTER_SYSTEM_SIMPLE(RotationSystem, 50);

} // namespace portal_core

#endif // ROTATION_SYSTEM_H
