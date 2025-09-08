#pragma once

#include "physics_events.h"
#include "physics_event_adapter.h"
#include "lazy_physics_query_manager.h"
#include "../event_manager.h"
#include "../physics_world_manager.h"
#include <entt/entt.hpp>
#include <memory>

namespace portal_core {

/**
 * 统一物理事件系统管理器
 * 
 * 整合所有物理事件相关组件，提供统一的接口
 * 支持2D/3D相交检测的懒加载物理事件系统
 */
class PhysicsEventSystem {
public:
    PhysicsEventSystem(EventManager& event_manager, 
                      PhysicsWorldManager& physics_world, 
                      entt::registry& registry);
    ~PhysicsEventSystem() = default;

    // 禁止拷贝，允许移动
    PhysicsEventSystem(const PhysicsEventSystem&) = delete;
    PhysicsEventSystem& operator=(const PhysicsEventSystem&) = delete;
    PhysicsEventSystem(PhysicsEventSystem&&) = default;
    PhysicsEventSystem& operator=(PhysicsEventSystem&&) = default;

    /**
     * 初始化物理事件系统
     */
    bool initialize();

    /**
     * 清理物理事件系统
     */
    void cleanup();

    /**
     * 更新物理事件系统
     */
    void update(float delta_time);

    /**
     * 启用/禁用系统
     */
    void set_enabled(bool enabled);
    bool is_enabled() const;

    /**
     * 设置调试模式
     */
    void set_debug_mode(bool debug);

    // === 便捷的事件订阅接口 ===

    /**
     * 获取碰撞开始事件的sink，用于连接处理器
     * 使用方式：get_collision_start_sink().connect<&YourClass::handler>(*this);
     */
    auto get_collision_start_sink() {
        return event_manager_.subscribe<CollisionStartEvent>();
    }

    /**
     * 获取碰撞结束事件的sink，用于连接处理器
     * 使用方式：get_collision_end_sink().connect<&YourClass::handler>(*this);
     */
    auto get_collision_end_sink() {
        return event_manager_.subscribe<CollisionEndEvent>();
    }

    /**
     * 获取触发器进入事件的sink，用于连接处理器
     * 使用方式：get_trigger_enter_sink().connect<&YourClass::handler>(*this);
     */
    auto get_trigger_enter_sink() {
        return event_manager_.subscribe<TriggerEnterEvent>();
    }

    /**
     * 获取触发器退出事件的sink，用于连接处理器
     * 使用方式：get_trigger_exit_sink().connect<&YourClass::handler>(*this);
     */
    auto get_trigger_exit_sink() {
        return event_manager_.subscribe<TriggerExitEvent>();
    }

    /**
     * 获取射线检测结果事件的sink，用于连接处理器
     * 使用方式：get_raycast_result_sink().connect<&YourClass::handler>(*this);
     */
    auto get_raycast_result_sink() {
        return event_manager_.subscribe<RaycastResultEvent>();
    }

    /**
     * 获取重叠查询结果事件的sink，用于连接处理器
     * 使用方式：get_overlap_result_sink().connect<&YourClass::handler>(*this);
     */
    auto get_overlap_result_sink() {
        return event_manager_.subscribe<OverlapQueryResultEvent>();
    }

    // === 便捷的查询请求接口 ===

    /**
     * 请求射线检测
     */
    void request_raycast(entt::entity requester, const Vec3& origin, const Vec3& direction, 
                        float max_distance = 1000.0f) {
        query_manager_->request_raycast(requester, origin, direction, max_distance);
    }

    /**
     * 请求区域监控
     */
    void request_area_monitoring(entt::entity requester, const Vec3& center, float radius) {
        query_manager_->request_area_monitoring(requester, center, radius);
    }

    /**
     * 请求水面检测（2D相交）
     */
    void request_water_surface_detection(entt::entity requester, entt::entity target, float water_level = 0.0f) {
        query_manager_->request_water_surface_detection(requester, target, water_level);
    }

    /**
     * 请求地面检测（2D相交）
     */
    void request_ground_detection(entt::entity requester, entt::entity target) {
        query_manager_->request_ground_detection(requester, target);
    }

    /**
     * 请求平面相交监控（2D相交）
     */
    void request_plane_intersection(entt::entity requester, entt::entity target, 
                                   const Vec3& plane_normal, float plane_distance) {
        query_manager_->request_plane_intersection_monitoring(requester, target, plane_normal, plane_distance);
    }

    // === 系统访问器 ===

    /**
     * 获取事件适配器
     */
    PhysicsEventAdapter& get_adapter() { return *adapter_; }
    const PhysicsEventAdapter& get_adapter() const { return *adapter_; }

    /**
     * 获取查询管理器
     */
    LazyPhysicsQueryManager& get_query_manager() { return *query_manager_; }
    const LazyPhysicsQueryManager& get_query_manager() const { return *query_manager_; }

    /**
     * 获取事件管理器
     */
    EventManager& get_event_manager() { return event_manager_; }
    const EventManager& get_event_manager() const { return event_manager_; }

    // === 统计和调试接口 ===

    /**
     * 获取系统统计信息
     */
    struct SystemStatistics {
        size_t processed_collisions = 0;
        size_t processed_queries = 0;
        size_t active_area_monitors = 0;
        size_t active_plane_intersections = 0;
        bool system_initialized = false;
        bool system_enabled = false;
        float last_update_time = 0.0f;
    };

    SystemStatistics get_statistics() const;

    /**
     * 重置统计信息
     */
    void reset_statistics();

    /**
     * 导出调试信息
     */
    void export_debug_info() const;

private:
    // 核心系统引用
    EventManager& event_manager_;
    PhysicsWorldManager& physics_world_;
    entt::registry& registry_;

    // 子系统
    std::unique_ptr<PhysicsEventAdapter> adapter_;
    std::unique_ptr<LazyPhysicsQueryManager> query_manager_;

    // 状态
    bool initialized_ = false;
    bool enabled_ = true;
    bool debug_mode_ = false;

    // 统计信息
    mutable SystemStatistics statistics_;
    float last_update_time_ = 0.0f;

    /**
     * 更新统计信息
     */
    void update_statistics() const;
};

} // namespace portal_core
