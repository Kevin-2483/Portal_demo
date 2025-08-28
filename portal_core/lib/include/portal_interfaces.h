#ifndef PORTAL_INTERFACES_H
#define PORTAL_INTERFACES_H

#include "portal_types.h"
#include <vector>

namespace Portal {

    // 抽象接口：宿主应用程序需要实现这些接口来与传送门系统交互

    /**
     * 物理查询接口 - 宿主应用程序实现，用于查询物理世界状态
     */
    class IPhysicsQuery {
    public:
        virtual ~IPhysicsQuery() = default;

        // 获取实体的变换信息
        virtual Transform get_entity_transform(EntityId entity_id) const = 0;
        
        // 获取实体的物理状态
        virtual PhysicsState get_entity_physics_state(EntityId entity_id) const = 0;
        
        // 检查实体是否存在
        virtual bool is_entity_valid(EntityId entity_id) const = 0;
        
        // 获取实体的包围盒（相对于实体本地空间）
        virtual void get_entity_bounds(EntityId entity_id, Vector3& min_bounds, Vector3& max_bounds) const = 0;
        
        // 射线检测 - 检查从起点到终点是否被阻挡
        virtual bool raycast(const Vector3& start, const Vector3& end, EntityId ignore_entity = INVALID_ENTITY_ID) const = 0;
    };

    /**
     * 物理操作接口 - 宿主应用程序实现，用于修改物理世界状态
     */
    class IPhysicsManipulator {
    public:
        virtual ~IPhysicsManipulator() = default;

        // 设置实体的变换
        virtual void set_entity_transform(EntityId entity_id, const Transform& transform) = 0;
        
        // 设置实体的物理状态
        virtual void set_entity_physics_state(EntityId entity_id, const PhysicsState& physics_state) = 0;
        
        // 临时禁用实体的碰撞检测（用于传送过程中）
        virtual void set_entity_collision_enabled(EntityId entity_id, bool enabled) = 0;
        
        // === 新增：幽灵碰撞体管理 ===
        
        // 在目标位置创建幽灵碰撞体（用于穿越期间的物理交互）
        virtual bool create_ghost_collider(EntityId entity_id, const Transform& ghost_transform) = 0;
        
        // 更新幽灵碰撞体的位置和状态
        virtual void update_ghost_collider(EntityId entity_id, const Transform& ghost_transform, const PhysicsState& ghost_physics) = 0;
        
        // 销毁幽灵碰撞体
        virtual void destroy_ghost_collider(EntityId entity_id) = 0;
        
        // 检查实体是否有活跃的幽灵碰撞体
        virtual bool has_ghost_collider(EntityId entity_id) const = 0;
    };

    /**
     * 渲染查询接口 - 用于获取渲染相关信息
     */
    class IRenderQuery {
    public:
        virtual ~IRenderQuery() = default;

        // 获取主相机参数
        virtual CameraParams get_main_camera() const = 0;
        
        // 检查点是否在视锥体内
        virtual bool is_point_in_view_frustum(const Vector3& point, const CameraParams& camera) const = 0;
        
        // 计算视锥体
        virtual Frustum calculate_frustum(const CameraParams& camera) const = 0;
    };

    /**
     * 渲染操作接口 - 用于控制渲染
     */
    class IRenderManipulator {
    public:
        virtual ~IRenderManipulator() = default;

        // 设置传送门渲染纹理
        virtual void set_portal_render_texture(PortalId portal_id, const CameraParams& virtual_camera) = 0;
        
        // 启用/禁用实体渲染
        virtual void set_entity_render_enabled(EntityId entity_id, bool enabled) = 0;
        
        // 配置模板缓冲区
        virtual void configure_stencil_buffer(bool enable, int ref_value = 1) = 0;
        
        // 设置裁剪平面
        virtual void set_clipping_plane(const ClippingPlane& plane) = 0;
        
        // 禁用裁剪平面
        virtual void disable_clipping_plane() = 0;
        
        // 重置渲染状态（清除所有特殊配置）
        virtual void reset_render_state() = 0;
        
        // 渲染传送门递归视图
        virtual void render_portal_recursive_view(PortalId portal_id, int recursion_depth) = 0;
    };

    /**
     * 事件通知接口 - 传送门系统通过此接口通知宿主应用程序重要事件
     */
    class IPortalEventHandler {
    public:
        virtual ~IPortalEventHandler() = default;

        // 实体即将传送
        virtual void on_entity_teleport_start(EntityId entity_id, PortalId source_portal, PortalId target_portal) {}
        
        // 实体传送完成
        virtual void on_entity_teleport_complete(EntityId entity_id, PortalId source_portal, PortalId target_portal) {}
        
        // 传送门链接建立
        virtual void on_portals_linked(PortalId portal1, PortalId portal2) {}
        
        // 传送门链接断开
        virtual void on_portals_unlinked(PortalId portal1, PortalId portal2) {}
        
        // 传送门进入递归状态（传送门穿过自己）
        virtual void on_portal_recursive_state(PortalId portal_id, bool is_recursive) {}
    };

    /**
     * 宿主接口容器 - 将所有需要的接口集合在一起
     */
    struct HostInterfaces {
        IPhysicsQuery* physics_query = nullptr;
        IPhysicsManipulator* physics_manipulator = nullptr;
        IRenderQuery* render_query = nullptr;
        IRenderManipulator* render_manipulator = nullptr;
        IPortalEventHandler* event_handler = nullptr;
        
        // 验证接口完整性
        bool is_valid() const {
            return physics_query && physics_manipulator && 
                   render_query && render_manipulator;
            // event_handler 是可选的
        }
    };

} // namespace Portal

#endif // PORTAL_INTERFACES_H
