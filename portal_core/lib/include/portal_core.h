#ifndef PORTAL_CORE_H
#define PORTAL_CORE_H

#include "portal_types.h"
#include "portal_interfaces.h"
#include "portal_math.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Portal {

    /**
     * 传送门对象
     * 表示一个传送门的完整状态
     */
    class Portal {
    public:
        Portal(PortalId id);
        ~Portal() = default;

        // 基本属性访问
        PortalId get_id() const { return id_; }
        const PortalPlane& get_plane() const { return plane_; }
        void set_plane(const PortalPlane& plane) { plane_ = plane; }

        // 链接管理
        PortalId get_linked_portal() const { return linked_portal_id_; }
        void set_linked_portal(PortalId portal_id) { linked_portal_id_ = portal_id; }
        bool is_linked() const { return linked_portal_id_ != INVALID_PORTAL_ID; }

        // 状态管理
        bool is_active() const { return is_active_; }
        void set_active(bool active) { is_active_ = active; }

        bool is_recursive() const { return is_recursive_; }
        void set_recursive(bool recursive) { is_recursive_ = recursive; }

        // 移动和物理状态
        const PhysicsState& get_physics_state() const { return physics_state_; }
        void set_physics_state(const PhysicsState& state) { physics_state_ = state; }

        // 渲染相关
        int get_max_recursion_depth() const { return max_recursion_depth_; }
        void set_max_recursion_depth(int depth) { max_recursion_depth_ = depth; }

    private:
        PortalId id_;
        PortalPlane plane_;
        PortalId linked_portal_id_;
        bool is_active_;
        bool is_recursive_;
        PhysicsState physics_state_;  // 传送门自身的物理状态（如果需要移动）
        int max_recursion_depth_;     // 最大递归渲染深度
    };

    /**
     * 传送门核心管理器
     * 负责所有传送门的创建、销毁、更新和传送逻辑
     */
    class PortalManager {
    public:
        explicit PortalManager(const HostInterfaces& interfaces);
        ~PortalManager() = default;

        // 禁用拷贝构造和赋值
        PortalManager(const PortalManager&) = delete;
        PortalManager& operator=(const PortalManager&) = delete;

        /**
         * 初始化传送门系统
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * 关闭传送门系统
         */
        void shutdown();

        /**
         * 每帧更新，处理传送逻辑
         * @param delta_time 时间差
         */
        void update(float delta_time);

        // === 传送门管理 ===
        
        /**
         * 创建新的传送门
         * @param plane 传送门平面定义
         * @return 传送门ID
         */
        PortalId create_portal(const PortalPlane& plane);

        /**
         * 销毁传送门
         * @param portal_id 传送门ID
         */
        void destroy_portal(PortalId portal_id);

        /**
         * 链接两个传送门
         * @param portal1 第一个传送门ID
         * @param portal2 第二个传送门ID
         * @return 是否链接成功
         */
        bool link_portals(PortalId portal1, PortalId portal2);

        /**
         * 取消传送门链接
         * @param portal_id 传送门ID
         */
        void unlink_portal(PortalId portal_id);

        /**
         * 获取传送门对象
         * @param portal_id 传送门ID
         * @return 传送门指针（可能为空）
         */
        Portal* get_portal(PortalId portal_id);
        const Portal* get_portal(PortalId portal_id) const;

        /**
         * 更新传送门平面
         * @param portal_id 传送门ID
         * @param plane 新的平面定义
         */
        void update_portal_plane(PortalId portal_id, const PortalPlane& plane);

        // === 实体传送管理 ===

        /**
         * 注册需要传送检测的实体
         * @param entity_id 实体ID
         */
        void register_entity(EntityId entity_id);

        /**
         * 取消注册实体
         * @param entity_id 实体ID
         */
        void unregister_entity(EntityId entity_id);

        /**
         * 手动触发实体传送（考虑传送门速度）
         * @param entity_id 实体ID
         * @param source_portal 源传送门
         * @param target_portal 目标传送门
         * @param consider_portal_velocity 是否考虑传送门速度
         * @return 传送结果
         */
        TeleportResult teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal);
        TeleportResult teleport_entity_with_velocity(EntityId entity_id, PortalId source_portal, PortalId target_portal);

        /**
         * 更新传送门的物理状态（位置和速度）
         * @param portal_id 传送门ID
         * @param physics_state 新的物理状态
         */
        void update_portal_physics_state(PortalId portal_id, const PhysicsState& physics_state);

        /**
         * 获取实体的传送状态
         * @param entity_id 实体ID
         * @return 传送状态
         */
        const TeleportState* get_entity_teleport_state(EntityId entity_id) const;

        // === 渲染支持 ===

        /**
         * 计算渲染通道描述符列表
         * @param main_camera 主相机参数
         * @param max_recursion_depth 最大递归深度
         * @return 渲染通道描述符列表
         */
        std::vector<RenderPassDescriptor> calculate_render_passes(
            const CameraParams& main_camera,
            int max_recursion_depth = 3
        ) const;

        /**
         * 获取正在穿越传送门的实体的裁剪平面
         * @param entity_id 实体ID
         * @param clipping_plane 输出的裁剪平面
         * @return 是否需要裁剪
         */
        bool get_entity_clipping_plane(EntityId entity_id, ClippingPlane& clipping_plane) const;

        /**
         * 获取传送门的递归渲染相机列表（已弃用，使用calculate_render_passes代替）
         * @param portal_id 传送门ID
         * @param base_camera 基础相机
         * @param max_depth 最大递归深度
         * @return 相机列表
         */
        std::vector<CameraParams> get_portal_render_cameras(
            PortalId portal_id, 
            const CameraParams& base_camera,
            int max_depth = 3
        ) const;

        /**
         * 检查传送门是否在相机视野内
         * @param portal_id 传送门ID
         * @param camera 相机参数
         * @return 是否可见
         */
        bool is_portal_visible(PortalId portal_id, const CameraParams& camera) const;

        // === 调试和统计 ===
        
        /**
         * 获取传送门数量
         */
        size_t get_portal_count() const { return portals_.size(); }

        /**
         * 获取注册实体数量
         */
        size_t get_registered_entity_count() const { return registered_entities_.size(); }

        /**
         * 获取当前传送中的实体数量
         */
        size_t get_teleporting_entity_count() const;

    private:
        // 内部更新方法
        void update_entity_teleportation(float delta_time);
        void check_entity_portal_intersections();
        void update_portal_recursive_states();
        void cleanup_completed_teleports();

        // 传送逻辑
        bool can_entity_teleport(EntityId entity_id, PortalId portal_id) const;
        void start_entity_teleport(EntityId entity_id, PortalId source_portal);
        void complete_entity_teleport(EntityId entity_id);
        void cancel_entity_teleport(EntityId entity_id);

        // 辅助方法
        PortalId generate_portal_id();
        bool is_valid_portal_id(PortalId portal_id) const;
        void notify_event_handler_if_available(const std::function<void(IPortalEventHandler*)>& callback) const;
        
        // === 新增：三状态机辅助方法 ===
        
        // 获取或创建传送状态
        TeleportState* get_or_create_teleport_state(EntityId entity_id, PortalId portal_id);
        
        // 清理实体与传送门的状态
        void cleanup_entity_portal_state(EntityId entity_id, PortalId portal_id);
        
        // 处理穿越状态变化
        void handle_crossing_state_change(
            EntityId entity_id, 
            PortalId portal_id, 
            PortalCrossingState previous_state, 
            PortalCrossingState new_state
        );
        
        // 管理幽灵碰撞体
        void create_ghost_collider_if_needed(EntityId entity_id, PortalId portal_id);
        void update_ghost_collider_position(EntityId entity_id, PortalId portal_id);
        void destroy_ghost_collider_if_exists(EntityId entity_id);
        
        // 渲染支持辅助方法
        void calculate_recursive_render_passes(
            PortalId portal_id,
            const CameraParams& current_camera,
            int current_depth,
            int max_depth,
            std::vector<RenderPassDescriptor>& render_passes
        ) const;

        // 数据成员
        HostInterfaces interfaces_;
        std::unordered_map<PortalId, std::unique_ptr<Portal>> portals_;
        std::unordered_set<EntityId> registered_entities_;
        std::unordered_map<EntityId, TeleportState> active_teleports_;
        
        PortalId next_portal_id_;
        bool is_initialized_;
        
        // 配置参数
        float teleport_transition_duration_;  // 传送过渡时间
        float portal_detection_distance_;     // 传送门检测距离
        int default_max_recursion_depth_;     // 默认最大递归深度
    };

} // namespace Portal

#endif // PORTAL_CORE_H
