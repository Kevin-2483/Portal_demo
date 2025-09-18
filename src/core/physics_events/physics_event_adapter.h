#pragma once

#include "physics_events.h"
#include "../event_manager.h"
#include "../physics_world_manager.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include <functional>

namespace portal_core {

/**
 * 物理事件适配器
 * 
 * 将Jolt Physics的底层回调转换为统一的事件系统
 * 实现懒加载机制和2D/3D相交检测支持
 */
class PhysicsEventAdapter {
public:
    PhysicsEventAdapter(EventManager& event_manager, 
                       PhysicsWorldManager& physics_world, 
                       entt::registry& registry);
    ~PhysicsEventAdapter() = default;

    // 禁止拷贝和移动（因为包含引用成员变量）
    PhysicsEventAdapter(const PhysicsEventAdapter&) = delete;
    PhysicsEventAdapter& operator=(const PhysicsEventAdapter&) = delete;
    PhysicsEventAdapter(PhysicsEventAdapter&&) = delete;
    PhysicsEventAdapter& operator=(PhysicsEventAdapter&&) = delete;

    /**
     * 初始化适配器，设置Jolt回调
     */
    bool initialize();

    /**
     * 扩展的初始化方法（设置组件监听器）
     */
    bool initialize(entt::registry& registry);

    /**
     * 清理适配器
     */
    void cleanup();

    /**
     * 更新适配器（处理懒加载查询等）
     */
    void update(float delta_time);

    /**
     * 启用/禁用适配器
     */
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

    /**
     * 设置调试模式
     */
    void set_debug_mode(bool debug) { debug_mode_ = debug; }

    /**
     * 从BodyID查找对应的实体（公有接口，用于测试）
     */
    entt::entity body_id_to_entity(BodyID body_id);

private:
    // 核心系统引用
    EventManager& event_manager_;
    PhysicsWorldManager& physics_world_;
    entt::registry& registry_;

    // 状态标志
    bool initialized_ = false;
    bool enabled_ = true;
    bool debug_mode_ = false;

    // EnTT 组件监听器连接 - 用于增量更新映射
    entt::connection physics_body_added_connection_;
    entt::connection physics_body_removed_connection_;
    entt::connection physics_body_updated_connection_;

    // === Jolt Physics 回调处理 ===

    /**
     * 处理碰撞添加回调（带接触信息）
     */
    void handle_contact_added_with_info(BodyID body1, BodyID body2, const Vec3& contact_point, const Vec3& contact_normal, float impulse_magnitude);

    /**
     * 处理碰撞移除回调
     */
    void handle_contact_removed(BodyID body1, BodyID body2);

    /**
     * 处理物体激活回调
     */
    void handle_body_activated(BodyID body_id, uint64 user_data);

    /**
     * 处理物体停用回调
     */
    void handle_body_deactivated(BodyID body_id, uint64 user_data);

    // === 实体查找和映射 ===

    /**
     * 内部实体查找方法
     */
    entt::entity find_entity_by_body_id(BodyID body_id);

    /**
     * 双向映射缓存 - 提供O(1)的双向查找性能
     */
    std::unordered_map<uint32_t, entt::entity> body_to_entity_map_;    // BodyID -> Entity
    std::unordered_map<uint32_t, uint32_t> entity_to_body_id_map_;     // Entity -> BodyID

    /**
     * 更新BodyID到实体的映射缓存 (已废弃 - 使用增量更新)
     * @deprecated 使用增量更新机制替代每帧重建
     */
    void update_body_entity_mapping();

    /**
     * 内部映射管理方法 - 确保双向映射的一致性
     */
    
    /**
     * 增量更新映射 - 添加单个实体映射（双向）
     */
    void add_entity_mapping_internal(entt::entity entity, BodyID body_id);

    /**
     * 增量更新映射 - 移除单个实体映射（双向）
     */
    void remove_entity_mapping_internal(entt::entity entity);

    /**
     * 增量更新映射 - 通过BodyID移除映射（双向）
     */
    void remove_body_mapping_internal(BodyID body_id);

    /**
     * 初始化映射 - 仅在初始化时调用一次
     */
    void initialize_body_entity_mapping();

    /**
     * 清理所有映射
     */
    void clear_all_mappings();

    // === 碰撞和触发器检测 ===

    /**
     * 获取接触信息
     */
    struct ContactInfo {
        Vec3 point;
        Vec3 normal;
        float impulse_magnitude = 0.0f;
    };
    
    ContactInfo get_contact_info(BodyID body1, BodyID body2);

    /**
     * 检查是否为传感器（触发器）
     */
    bool is_sensor_body(BodyID body_id);
    
    /**
     * 安全检查是否为传感器（避免死锁）
     */
    bool is_body_sensor_safe(BodyID body_id);

    // === 2D/3D 相交检测支持 ===

    /**
     * 处理平面相交检测（2D相交）
     * 物体与平面的相交检测
     */
    void process_plane_intersections(float delta_time);

    /**
     * 检测单个实体的平面相交
     */
    void check_entity_plane_intersection(entt::entity entity, PlaneIntersectionComponent& plane_comp);

    /**
     * 处理区域监控更新
     */
    void process_area_monitoring(float delta_time);

    /**
     * 处理持续碰撞检测
     */
    void process_persistent_contacts(float delta_time);

    // === 懒加载查询处理 ===

    /**
     * 处理待处理的物理查询
     */
    void process_pending_queries();

    /**
     * 处理单个实体的查询
     */
    void process_entity_queries(entt::entity entity);

    /**
     * 执行射线查询并发送结果事件
     */
    void execute_raycast_queries(entt::entity entity, PhysicsEventQueryComponent& query_comp);

    /**
     * 执行重叠查询并发送结果事件
     */
    void execute_overlap_queries(entt::entity entity, PhysicsEventQueryComponent& query_comp);

    // === 事件分发辅助方法 ===

    /**
     * 发送碰撞开始事件
     */
    void dispatch_collision_start_event(entt::entity entity_a, entt::entity entity_b, 
                                       const ContactInfo& contact);

    /**
     * 发送碰撞结束事件
     */
    void dispatch_collision_end_event(entt::entity entity_a, entt::entity entity_b);

    /**
     * 发送触发器进入事件
     */
    void dispatch_trigger_enter_event(entt::entity sensor_entity, entt::entity other_entity, 
                                     const ContactInfo& contact);

    /**
     * 发送触发器退出事件
     */
    void dispatch_trigger_exit_event(entt::entity sensor_entity, entt::entity other_entity);

    /**
     * 处理区域监控变化
     */
    void handle_area_monitoring_change(entt::entity sensor_entity, entt::entity other_entity, bool entering);

    // === 相交类型检测 ===

    /**
     * 检测相交类型（2D平面相交或3D空间相交）
     */
    PhysicsEventDimension detect_intersection_dimension(const Vec3& contact_point, const Vec3& contact_normal);

    /**
     * 判断是否为平面相交（2D类型）
     */
    bool is_plane_intersection(const Vec3& contact_normal, float tolerance = 0.1f);

    // === 性能监控 ===
    mutable size_t processed_collisions_count_ = 0;
    mutable size_t processed_queries_count_ = 0;
    mutable float last_update_time_ = 0.0f;

    /**
     * 调试日志输出
     */
    void debug_log(const std::string& message);

    // === 增量映射更新事件处理 ===

    /**
     * 处理物理体组件添加事件
     */
    void on_physics_body_component_added(entt::registry& registry, entt::entity entity);

    /**
     * 处理物理体组件移除事件
     */
    void on_physics_body_component_removed(entt::registry& registry, entt::entity entity);

    /**
     * 处理物理体组件更新事件（BodyID可能改变）
     */
    void on_physics_body_component_updated(entt::registry& registry, entt::entity entity);
};

} // namespace portal_core
