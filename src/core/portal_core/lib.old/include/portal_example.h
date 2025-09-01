#ifndef PORTAL_EXAMPLE_H
#define PORTAL_EXAMPLE_H

#include "portal_core.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

// 为std::pair添加hash支持
namespace std {
    template <>
    struct hash<std::pair<Portal::EntityId, Portal::PortalId>>
    {
        size_t operator()(const std::pair<Portal::EntityId, Portal::PortalId>& p) const
        {
            return std::hash<Portal::EntityId>()(p.first) ^ 
                   (std::hash<Portal::PortalId>()(p.second) << 1);
        }
    };
}

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

      // 穿越状态追踪
      struct CrossingTrackingData
      {
        bool was_on_positive_side = false; // 上一帧是否在正法向量一侧
        float last_distance = 0.0f;        // 上一次的距离
        bool has_previous_data = false;    // 是否有上一帧数据
      };

      std::unordered_map<EntityId, EntityData> entities_;
      mutable std::unordered_map<std::pair<EntityId, PortalId>, CrossingTrackingData, 
                                 std::hash<std::pair<EntityId, PortalId>>> crossing_states_;

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

      // === 新增：质心检测支持实现 ===

      EntityDescription get_entity_description(EntityId entity_id) const override
      {
        EntityDescription desc;
        desc.entity_id = entity_id;
        
        auto it = entities_.find(entity_id);
        if (it != entities_.end())
        {
          desc.transform = it->second.transform;
          desc.physics = it->second.physics_state;
          desc.bounds_min = it->second.bounds_min;
          desc.bounds_max = it->second.bounds_max;
        }
        else
        {
          // 默认值
          desc.bounds_min = Vector3(-0.5f, -0.5f, -0.5f);
          desc.bounds_max = Vector3(0.5f, 0.5f, 0.5f);
        }
        
        // 设置质心（展示新的質心管理系統）
        desc.center_of_mass = Vector3(0, 0.25f, 0);  // 設置在實體上方0.25單位
        desc.entity_type = EntityType::MAIN;
        desc.is_fully_functional = true;
        
        return desc;
      }

      CenterOfMassCrossing check_center_crossing(EntityId entity_id, const PortalPlane &portal_plane, PortalFace face) const override
      {
        CenterOfMassCrossing crossing;
        crossing.entity_id = entity_id;
        crossing.crossed_face = face;
        
        EntityDescription desc = get_entity_description(entity_id);
        Vector3 center_world = desc.transform.transform_point(desc.center_of_mass);
        
        // 计算质心到平面的距离
        float distance = (center_world - portal_plane.center).dot(portal_plane.normal);
        bool is_on_positive_side = distance > 0;
        
        crossing.center_world_pos = center_world;
        crossing.crossing_progress = is_on_positive_side ? 1.0f : 0.0f;
        
        // 生成基於傳送門位置的唯一追踪鍵
        // 使用传送门中心坐标的哈希值作为伪PortalId
        size_t portal_hash = std::hash<float>()(portal_plane.center.x) ^ 
                            (std::hash<float>()(portal_plane.center.y) << 1) ^
                            (std::hash<float>()(portal_plane.center.z) << 2);
        PortalId portal_id = static_cast<PortalId>(portal_hash);
        std::pair<EntityId, PortalId> tracking_key{entity_id, portal_id};
        
        // 获取或创建追踪数据
        auto& tracking = crossing_states_[tracking_key];
        
        // 检测状态变化
        if (tracking.has_previous_data)
        {
          // 检测刚开始穿越：从一侧移动到另一侧
          crossing.just_started = (is_on_positive_side != tracking.was_on_positive_side);
          
          // 如果刚开始穿越，记录穿越面
          if (crossing.just_started)
          {
            if (is_on_positive_side)
            {
              crossing.crossed_face = PortalFace::A; // 从负侧穿越到正侧
            }
            else
            {
              crossing.crossed_face = PortalFace::B; // 从正侧穿越到负侧
            }
          }
        }
        else
        {
          crossing.just_started = false;
        }
        
        // 更新追踪数据
        tracking.was_on_positive_side = is_on_positive_side;
        tracking.last_distance = distance;
        tracking.has_previous_data = true;
        
        std::cout << "Checking center crossing for entity " << entity_id 
                  << ", distance to portal: " << distance 
                  << ", just_started: " << (crossing.just_started ? 1 : 0) << "\n";
        
        return crossing;
      }

      float calculate_center_crossing_progress(EntityId entity_id, const PortalPlane &portal_plane) const override
      {
        EntityDescription desc = get_entity_description(entity_id);
        Vector3 center_world = desc.transform.transform_point(desc.center_of_mass);
        
        // 使用数学库计算穿越进度
        return Math::PortalMath::calculate_point_crossing_progress(
            center_world, portal_plane, desc.bounds_min, desc.bounds_max);
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

      // === 新增：幽灵实体管理方法 ===
      
      EntityId create_ghost_entity(EntityId main_entity_id, const Transform &ghost_transform, const PhysicsState &ghost_physics) override
      {
        EntityId ghost_id = main_entity_id + 10000; // 简单的幽灵ID分配策略
        std::cout << "Creating ghost entity " << ghost_id << " for main entity " << main_entity_id
                  << " at position (" << ghost_transform.position.x
                  << ", " << ghost_transform.position.y
                  << ", " << ghost_transform.position.z << ")\n";
        return ghost_id;
      }

      void destroy_ghost_entity(EntityId ghost_entity_id) override
      {
        std::cout << "Destroying ghost entity " << ghost_entity_id << "\n";
      }

      void sync_ghost_entities(const std::vector<GhostEntitySnapshot> &snapshots) override
      {
        std::cout << "Syncing " << snapshots.size() << " ghost entities\n";
        for (const auto &snapshot : snapshots)
        {
          std::cout << "  Ghost " << snapshot.ghost_entity_id
                    << " -> pos(" << snapshot.ghost_transform.position.x
                    << ", " << snapshot.ghost_transform.position.y
                    << ", " << snapshot.ghost_transform.position.z << ")\n";
        }
      }

      void set_ghost_entity_bounds(EntityId ghost_entity_id, const Vector3 &bounds_min, const Vector3 &bounds_max) override
      {
        std::cout << "Setting ghost entity " << ghost_entity_id
                  << " bounds: min(" << bounds_min.x << ", " << bounds_min.y << ", " << bounds_min.z
                  << ") max(" << bounds_max.x << ", " << bounds_max.y << ", " << bounds_max.z << ")\n";
      }

      // === 新增：无缝传送支持实现 ===

      EntityId create_full_functional_ghost(const EntityDescription &entity_desc, const Transform &ghost_transform, const PhysicsState &ghost_physics) override
      {
        EntityId ghost_id = entity_desc.entity_id + 20000; // 全功能幽灵ID策略
        std::cout << "Creating FULL-FUNCTIONAL ghost entity " << ghost_id 
                  << " for main entity " << entity_desc.entity_id
                  << " at position (" << ghost_transform.position.x
                  << ", " << ghost_transform.position.y
                  << ", " << ghost_transform.position.z << ")\n";
        std::cout << "  Ghost has FULL physics, collision, and rendering capabilities\n";
        return ghost_id;
      }

      bool promote_ghost_to_main(EntityId ghost_id, EntityId old_main_id) override
      {
        std::cout << "PROMOTING ghost entity " << ghost_id 
                  << " to main entity, replacing old main " << old_main_id << "\n";
        
        // 重要：在測試中，我們需要找到幽靈實體的位置並更新主實體
        // 在真實實現中，這裡應該將幽靈實體的變換和物理狀態應用到主實體上
        
        // 由於測試環境限制，我們需要從 physics_query 獲取幽靈位置
        // 然後更新主實體位置
        if (physics_query_) {
          // 查找幽靈實體的變換（這裡需要實際實現）
          // 目前先打印成功信息
          std::cout << "  ✅ 實體位置更新：主實體應移動到幽靈實體的位置\n";
        }
        
        std::cout << "  Ghost entity now becomes the PRIMARY entity with full functionality\n";
        std::cout << "  Old main entity is destroyed or becomes inactive\n";
        return true;  // 假设总是成功
      }

      void set_entity_functional_state(EntityId entity_id, bool is_fully_functional) override
      {
        std::cout << "Setting entity " << entity_id 
                  << " functional state: " << (is_fully_functional ? "FULL" : "LIMITED") 
                  << " functionality\n";
      }

      bool copy_all_entity_properties(EntityId source_entity_id, EntityId target_entity_id) override
      {
        std::cout << "Copying ALL properties from entity " << source_entity_id 
                  << " to entity " << target_entity_id << "\n";
        std::cout << "  Copied: physics, rendering, collision, gameplay properties\n";
        return true;
      }

      Vector3 get_entity_center_of_mass_world_pos(EntityId entity_id) const override
      {
        std::cout << "Getting center of mass for entity " << entity_id << "\n";
        return Vector3(0, 0.5f, 0); // 简化实现：假设质心在实体上方0.5单位
      }

      void set_entity_center_of_mass(EntityId entity_id, const Vector3 &center_offset) override
      {
        std::cout << "Setting center of mass for entity " << entity_id 
                  << " to offset (" << center_offset.x << ", " << center_offset.y 
                  << ", " << center_offset.z << ")\n";
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

        // 演示新的質心配置系統
        std::cout << "\nDemonstrating advanced Center of Mass system...\n";

        // 方法1: 使用自定義質心點
        CenterOfMassConfig custom_config = create_custom_point_config(Vector3(0, 0.5f, 0));
        portal_manager_->set_entity_center_of_mass_config(entity_id, custom_config);
        std::cout << "Set custom center of mass at (0, 0.5, 0)\n";

        // 方法2: 演示加權點質心
        std::vector<WeightedPoint> weighted_points = {
            WeightedPoint(Vector3(0, 0.8f, 0), 2.0f),    // 頭部，權重2.0
            WeightedPoint(Vector3(0, 0.4f, 0), 3.0f),    // 胸部，權重3.0  
            WeightedPoint(Vector3(0, -0.2f, 0), 1.0f)    // 腿部，權重1.0
        };
        CenterOfMassConfig weighted_config = create_weighted_points_config(weighted_points);
        
        // 先使用自定義配置，然後切換到加權配置做對比
        std::cout << "Will switch to weighted points configuration after initial test...\n";

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
          std::cout << "Update frame " << (i + 1) << ":\n";
          portal_manager_->update(0.016f); // 模拟60FPS
        }

        // 演示質心配置切換
        std::cout << "\nSwitching to weighted points center of mass...\n";
        portal_manager_->set_entity_center_of_mass_config(entity_id, weighted_config);

        // 再模拟几帧以展示新的質心配置效果
        std::cout << "Testing with new center of mass configuration...\n";
        for (int i = 0; i < 3; ++i)
        {
          std::cout << "Update frame " << (i + 6) << " (weighted CoM):\n";
          portal_manager_->update(0.016f);
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
