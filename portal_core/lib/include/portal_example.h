#ifndef PORTAL_EXAMPLE_H
#define PORTAL_EXAMPLE_H

#include "portal_core.h"
#include <iostream>
#include <memory>

namespace Portal
{
  namespace Example
  {

    /**
     * 示例物理查询实现
     * 这是一个简化的实现，展示了如何适配您的物理引擎
     */
    class ExamplePhysicsQuery : public IPhysicsQuery
    {
    private:
      // 模拟的实体数据
      struct EntityData
      {
        Transform transform;
        PhysicsState physics_state;
        Vector3 bounds_min;
        Vector3 bounds_max;
        bool is_valid = true;
      };

      std::unordered_map<EntityId, EntityData> entities_;

    public:
      // 添加测试实体（用于演示）
      void add_test_entity(EntityId entity_id, const Transform &transform,
                           const Vector3 &bounds_min = Vector3(-0.5f, -0.5f, -0.5f),
                           const Vector3 &bounds_max = Vector3(0.5f, 0.5f, 0.5f))
      {
        EntityData &data = entities_[entity_id];
        data.transform = transform;
        data.bounds_min = bounds_min;
        data.bounds_max = bounds_max;
        data.is_valid = true;
      }

      // IPhysicsQuery 接口实现
      Transform get_entity_transform(EntityId entity_id) const override
      {
        auto it = entities_.find(entity_id);
        return (it != entities_.end()) ? it->second.transform : Transform();
      }

      PhysicsState get_entity_physics_state(EntityId entity_id) const override
      {
        auto it = entities_.find(entity_id);
        return (it != entities_.end()) ? it->second.physics_state : PhysicsState();
      }

      bool is_entity_valid(EntityId entity_id) const override
      {
        auto it = entities_.find(entity_id);
        return (it != entities_.end()) && it->second.is_valid;
      }

      void get_entity_bounds(EntityId entity_id, Vector3 &min_bounds, Vector3 &max_bounds) const override
      {
        auto it = entities_.find(entity_id);
        if (it != entities_.end())
        {
          min_bounds = it->second.bounds_min;
          max_bounds = it->second.bounds_max;
        }
        else
        {
          min_bounds = Vector3(-0.5f, -0.5f, -0.5f);
          max_bounds = Vector3(0.5f, 0.5f, 0.5f);
        }
      }

      bool raycast(const Vector3 &start, const Vector3 &end, EntityId ignore_entity) const override
      {
        // 简化的射线检测 - 实际实现需要调用您的物理引擎
        return false;
      }

      // 辅助方法：更新实体状态（用于演示）
      void update_entity_transform(EntityId entity_id, const Transform &transform)
      {
        auto it = entities_.find(entity_id);
        if (it != entities_.end())
        {
          it->second.transform = transform;
        }
      }

      void update_entity_physics_state(EntityId entity_id, const PhysicsState &physics_state)
      {
        auto it = entities_.find(entity_id);
        if (it != entities_.end())
        {
          it->second.physics_state = physics_state;
        }
      }
    };

    /**
     * 示例物理操作实现
     */
    class ExamplePhysicsManipulator : public IPhysicsManipulator
    {
    private:
      ExamplePhysicsQuery *physics_query_;

    public:
      ExamplePhysicsManipulator(ExamplePhysicsQuery *physics_query)
          : physics_query_(physics_query) {}

      void set_entity_transform(EntityId entity_id, const Transform &transform) override
      {
        if (physics_query_)
        {
          physics_query_->update_entity_transform(entity_id, transform);
        }
        std::cout << "Set entity " << entity_id << " transform to ("
                  << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << ")\n";
      }

      void set_entity_physics_state(EntityId entity_id, const PhysicsState &physics_state) override
      {
        if (physics_query_)
        {
          physics_query_->update_entity_physics_state(entity_id, physics_state);
        }
        std::cout << "Set entity " << entity_id << " velocity to ("
                  << physics_state.linear_velocity.x << ", " << physics_state.linear_velocity.y << ", "
                  << physics_state.linear_velocity.z << ")\n";
      }

      void set_entity_collision_enabled(EntityId entity_id, bool enabled) override
      {
        std::cout << "Set entity " << entity_id << " collision " << (enabled ? "enabled" : "disabled") << "\n";
      }

      // === 幽灵碰撞体管理 ===

      bool create_ghost_collider(EntityId entity_id, const Transform &ghost_transform) override
      {
        std::cout << "Creating ghost collider for entity " << entity_id
                  << " at position (" << ghost_transform.position.x
                  << ", " << ghost_transform.position.y
                  << ", " << ghost_transform.position.z << ")\n";
        // 简化实现：假设总是成功
        return true;
      }

      void update_ghost_collider(EntityId entity_id, const Transform &ghost_transform, const PhysicsState &ghost_physics) override
      {
        std::cout << "Updating ghost collider for entity " << entity_id
                  << " to position (" << ghost_transform.position.x
                  << ", " << ghost_transform.position.y
                  << ", " << ghost_transform.position.z << ")\n";
      }

      void destroy_ghost_collider(EntityId entity_id) override
      {
        std::cout << "Destroying ghost collider for entity " << entity_id << "\n";
      }

      bool has_ghost_collider(EntityId entity_id) const override
      {
        std::cout << "Checking ghost collider for entity " << entity_id << "\n";
        return false; // 简化实现
      }
    };

    /**
     * 示例渲染查询实现
     */
    class ExampleRenderQuery : public IRenderQuery
    {
    public:
      CameraParams get_main_camera() const override
      {
        CameraParams camera;
        camera.position = Vector3(0, 0, 5);
        camera.rotation = Quaternion(); // 默认朝向
        camera.fov = 75.0f;
        return camera;
      }

      bool is_point_in_view_frustum(const Vector3 &point, const CameraParams &camera) const override
      {
        // 简化的视锥体检测
        Vector3 to_point = point - camera.position;
        float distance = to_point.length();
        return distance > camera.near_plane && distance < camera.far_plane;
      }

      Frustum calculate_frustum(const CameraParams &camera) const override
      {
        // 简化的视锥体计算
        Frustum frustum;
        // 这里需要实际的视锥体计算逻辑
        return frustum;
      }
    };

    /**
     * 示例渲染操作实现
     */
    class ExampleRenderManipulator : public IRenderManipulator
    {
    public:
      void set_portal_render_texture(PortalId portal_id, const CameraParams &virtual_camera) override
      {
        std::cout << "Set portal " << portal_id << " render texture with virtual camera\n";
      }

      void set_entity_render_enabled(EntityId entity_id, bool enabled) override
      {
        std::cout << "Set entity " << entity_id << " render " << (enabled ? "enabled" : "disabled") << "\n";
      }

      void configure_stencil_buffer(bool enable, int ref_value) override
      {
        std::cout << "Configure stencil buffer: " << (enable ? "enabled" : "disabled")
                  << " (ref value: " << ref_value << ")\n";
      }

      void set_clipping_plane(const ClippingPlane &plane) override
      {
        std::cout << "Set clipping plane: normal("
                  << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z
                  << ") distance(" << plane.distance << ")\n";
      }

      void disable_clipping_plane() override
      {
        std::cout << "Disable clipping plane\n";
      }

      void reset_render_state() override
      {
        std::cout << "Reset render state\n";
      }

      void render_portal_recursive_view(PortalId portal_id, int recursion_depth) override
      {
        std::cout << "Render portal " << portal_id << " recursive view (depth: " << recursion_depth << ")\n";
      }
    };

    /**
     * 示例事件处理器实现
     */
    class ExampleEventHandler : public IPortalEventHandler
    {
    public:
      void on_entity_teleport_start(EntityId entity_id, PortalId source_portal, PortalId target_portal) override
      {
        std::cout << "Entity " << entity_id << " started teleporting from portal "
                  << source_portal << " to portal " << target_portal << "\n";
      }

      void on_entity_teleport_complete(EntityId entity_id, PortalId source_portal, PortalId target_portal) override
      {
        std::cout << "Entity " << entity_id << " completed teleporting from portal "
                  << source_portal << " to portal " << target_portal << "\n";
      }

      void on_portals_linked(PortalId portal1, PortalId portal2) override
      {
        std::cout << "Portal " << portal1 << " linked with portal " << portal2 << "\n";
      }

      void on_portals_unlinked(PortalId portal1, PortalId portal2) override
      {
        std::cout << "Portal " << portal1 << " unlinked from portal " << portal2 << "\n";
      }

      void on_portal_recursive_state(PortalId portal_id, bool is_recursive) override
      {
        std::cout << "Portal " << portal_id << " recursive state: " << (is_recursive ? "ON" : "OFF") << "\n";
      }
    };

    /**
     * 完整的传送门系统使用示例
     */
    class PortalSystemExample
    {
    private:
      std::unique_ptr<ExamplePhysicsQuery> physics_query_;
      std::unique_ptr<ExamplePhysicsManipulator> physics_manipulator_;
      std::unique_ptr<ExampleRenderQuery> render_query_;
      std::unique_ptr<ExampleRenderManipulator> render_manipulator_;
      std::unique_ptr<ExampleEventHandler> event_handler_;
      std::unique_ptr<PortalManager> portal_manager_;

    public:
      PortalSystemExample()
      {
        // 创建所有接口实现
        physics_query_ = std::make_unique<ExamplePhysicsQuery>();
        physics_manipulator_ = std::make_unique<ExamplePhysicsManipulator>(physics_query_.get());
        render_query_ = std::make_unique<ExampleRenderQuery>();
        render_manipulator_ = std::make_unique<ExampleRenderManipulator>();
        event_handler_ = std::make_unique<ExampleEventHandler>();

        // 设置接口容器
        HostInterfaces interfaces;
        interfaces.physics_query = physics_query_.get();
        interfaces.physics_manipulator = physics_manipulator_.get();
        interfaces.render_query = render_query_.get();
        interfaces.render_manipulator = render_manipulator_.get();
        interfaces.event_handler = event_handler_.get();

        // 创建传送门管理器
        portal_manager_ = std::make_unique<PortalManager>(interfaces);
      }

      void run_example()
      {
        std::cout << "=== Portal System Example ===\n\n";

        // 初始化传送门系统
        if (!portal_manager_->initialize())
        {
          std::cout << "Failed to initialize portal system!\n";
          return;
        }

        // 创建两个传送门
        PortalPlane plane1;
        plane1.center = Vector3(-5, 0, 0);
        plane1.normal = Vector3(1, 0, 0);
        plane1.up = Vector3(0, 1, 0);
        plane1.right = Vector3(0, 0, 1);
        plane1.width = 2.0f;
        plane1.height = 3.0f;

        PortalPlane plane2;
        plane2.center = Vector3(5, 0, 0);
        plane2.normal = Vector3(-1, 0, 0);
        plane2.up = Vector3(0, 1, 0);
        plane2.right = Vector3(0, 0, -1);
        plane2.width = 2.0f;
        plane2.height = 3.0f;

        PortalId portal1 = portal_manager_->create_portal(plane1);
        PortalId portal2 = portal_manager_->create_portal(plane2);

        std::cout << "Created portal " << portal1 << " and portal " << portal2 << "\n";

        // 链接传送门
        if (portal_manager_->link_portals(portal1, portal2))
        {
          std::cout << "Successfully linked portals\n";
        }

        // 创建测试实体
        EntityId entity_id = 100;
        Transform entity_transform;
        entity_transform.position = Vector3(-3, 0, 0); // 靠近第一个传送门

        physics_query_->add_test_entity(entity_id, entity_transform);
        portal_manager_->register_entity(entity_id);

        std::cout << "Created test entity at position ("
                  << entity_transform.position.x << ", " << entity_transform.position.y << ", "
                  << entity_transform.position.z << ")\n";

        // 手动触发传送
        std::cout << "\nTriggeringmanual teleport...\n";
        TeleportResult result = portal_manager_->teleport_entity(entity_id, portal1, portal2);

        switch (result)
        {
        case TeleportResult::SUCCESS:
          std::cout << "Teleport successful!\n";
          break;
        case TeleportResult::FAILED_INVALID_PORTAL:
          std::cout << "Teleport failed: Invalid portal\n";
          break;
        default:
          std::cout << "Teleport failed: Unknown reason\n";
          break;
        }

        // 模拟几帧更新
        std::cout << "\nSimulating system updates...\n";
        for (int i = 0; i < 5; ++i)
        {
          portal_manager_->update(0.016f); // 模拟60FPS
        }

        // 获取系统统计信息
        std::cout << "\nSystem statistics:\n";
        std::cout << "Portal count: " << portal_manager_->get_portal_count() << "\n";
        std::cout << "Registered entities: " << portal_manager_->get_registered_entity_count() << "\n";
        std::cout << "Teleporting entities: " << portal_manager_->get_teleporting_entity_count() << "\n";

        // 清理
        portal_manager_->shutdown();
        std::cout << "\n=== Example Complete ===\n";
      }
    };

  } // namespace Example
} // namespace Portal

#endif // PORTAL_EXAMPLE_H
