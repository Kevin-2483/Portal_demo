#pragma once

#include "physics_events.h"
#include "../event_manager.h"
#include "../physics_world_manager.h"
#include <entt/entt.hpp>

namespace portal_core {

// 前向声明
struct PhysicsBodyComponent;

/**
 * 懒加载物理查询管理器
 * 
 * 提供便捷的懒加载物理查询接口，按需创建组件和查询
 * 支持2D/3D相交检测和各种物理查询类型
 */
class LazyPhysicsQueryManager {
public:
    LazyPhysicsQueryManager(EventManager& event_manager, 
                           PhysicsWorldManager& physics_world, 
                           entt::registry& registry);
    ~LazyPhysicsQueryManager() = default;

    // 禁止拷贝，允许移动
    LazyPhysicsQueryManager(const LazyPhysicsQueryManager&) = delete;
    LazyPhysicsQueryManager& operator=(const LazyPhysicsQueryManager&) = delete;
    LazyPhysicsQueryManager(LazyPhysicsQueryManager&&) = default;
    LazyPhysicsQueryManager& operator=(LazyPhysicsQueryManager&&) = default;

    // === 懒加载射线检测 ===

    /**
     * 请求射线检测（懒加载）
     * 如果实体没有查询组件，会自动创建
     */
    void request_raycast(entt::entity requester, const Vec3& origin, const Vec3& direction, 
                        float max_distance = 1000.0f, uint32_t layer_mask = 0xFFFFFFFF);

    /**
     * 批量射线检测请求
     */
    void request_multiple_raycasts(entt::entity requester, 
                                 const std::vector<std::tuple<Vec3, Vec3, float>>& raycast_params);

    // === 懒加载区域监控 ===

    /**
     * 请求区域监控（懒加载）
     * 自动创建区域监控组件和查询组件
     */
    void request_area_monitoring(entt::entity requester, const Vec3& center, float radius, 
                               uint32_t layer_mask = 0xFFFFFFFF, float update_interval = 0.1f);

    /**
     * 请求矩形区域监控
     */
    void request_box_area_monitoring(entt::entity requester, const Vec3& center, const Vec3& half_extents,
                                   const Quat& rotation = Quat(0, 0, 0, 1), uint32_t layer_mask = 0xFFFFFFFF);

    // === 懒加载平面相交检测（2D相交） ===

    /**
     * 请求平面相交监控（2D相交检测）
     * 监控指定实体与平面的相交情况
     */
    void request_plane_intersection_monitoring(entt::entity requester, entt::entity target_entity,
                                             const Vec3& plane_normal, float plane_distance,
                                             float check_interval = 0.016f);

    /**
     * 请求水面检测（特殊的平面相交）
     */
    void request_water_surface_detection(entt::entity requester, entt::entity target_entity, 
                                        float water_level = 0.0f);

    /**
     * 请求地面检测（特殊的平面相交）
     */
    void request_ground_detection(entt::entity requester, entt::entity target_entity);

    // === 懒加载距离查询 ===

    /**
     * 请求最近实体查询
     */
    void request_nearest_entity_query(entt::entity requester, const Vec3& center, 
                                     float max_distance = 100.0f, uint32_t layer_mask = 0xFFFFFFFF);

    /**
     * 请求距离范围查询
     */
    void request_distance_range_query(entt::entity requester, const Vec3& center, 
                                     float min_distance, float max_distance, uint32_t layer_mask = 0xFFFFFFFF);

    // === 懒加载形状查询 ===

    /**
     * 请求球体重叠查询
     */
    void request_sphere_overlap_query(entt::entity requester, const Vec3& center, float radius,
                                     uint32_t layer_mask = 0xFFFFFFFF);

    /**
     * 请求盒体重叠查询
     */
    void request_box_overlap_query(entt::entity requester, const Vec3& center, const Vec3& half_extents,
                                  const Quat& rotation = Quat(0, 0, 0, 1), uint32_t layer_mask = 0xFFFFFFFF);

    // === 高级查询功能 ===

    /**
     * 请求持续碰撞监控
     * 监控实体的持续碰撞状态
     */
    void request_persistent_contact_monitoring(entt::entity requester, entt::entity other_entity,
                                             float duration_threshold = 0.5f, float force_threshold = 0.1f);

    /**
     * 请求包含检测
     * 检测实体是否在指定边界内
     */
    void request_containment_detection(entt::entity requester, const Vec3& bounds_min, const Vec3& bounds_max,
                                     float check_interval = 0.1f);

    // === 查询管理 ===

    /**
     * 处理所有待处理的查询（由适配器调用）
     */
    void process_pending_queries(float delta_time);

    /**
     * 取消实体的所有查询
     */
    void cancel_entity_queries(entt::entity entity);

    /**
     * 取消特定类型的查询
     */
    void cancel_raycast_queries(entt::entity entity);
    void cancel_area_monitoring(entt::entity entity);
    void cancel_plane_intersection_monitoring(entt::entity entity);

    // === 配置和统计 ===

    /**
     * 设置最大每帧查询数
     */
    void set_max_queries_per_frame(int max_queries) { max_queries_per_frame_ = max_queries; }

    /**
     * 获取查询统计信息
     */
    struct QueryStatistics {
        size_t pending_raycast_queries = 0;
        size_t pending_overlap_queries = 0;
        size_t active_area_monitors = 0;
        size_t active_plane_intersections = 0;
        size_t processed_queries_this_frame = 0;
        float average_query_time_ms = 0.0f;
    };

    QueryStatistics get_query_statistics() const;

    /**
     * 启用/禁用调试模式
     */
    void set_debug_mode(bool debug) { debug_mode_ = debug; }

private:
    // 核心系统引用
    EventManager& event_manager_;
    PhysicsWorldManager& physics_world_;
    entt::registry& registry_;

    // 配置参数
    int max_queries_per_frame_ = 100;
    bool debug_mode_ = false;

    // 统计信息
    mutable QueryStatistics statistics_;

    // === 内部辅助方法 ===

    /**
     * 确保实体有查询组件
     */
    void ensure_query_component(entt::entity entity);

    /**
     * 确保实体有待处理查询标记
     */
    void ensure_pending_query_tag(entt::entity entity, int priority = 0);

    /**
     * 检测查询的相交维度类型
     */
    PhysicsEventDimension detect_query_dimension(const Vec3& position, const Vec3& direction = Vec3::sZero());

    /**
     * 调试日志输出
     */
    void debug_log(const std::string& message) const;

    /**
     * 更新统计信息
     */
    void update_statistics() const;
};

} // namespace portal_core
