#pragma once

#include "physics_event_types.h"
#include "../components/physics_command_component.h"
#include <vector>

// 前向声明
namespace portal_core {
    struct AreaMonitorComponent;
}

namespace portal_core {

// === 事件驱动的查询组件 (基于现有PhysicsQueryComponent扩展) ===

/**
 * 事件驱动的物理查询组件
 * 扩展现有的PhysicsQueryComponent以支持事件系统
 * 懒加载的物理查询请求容器，与EventManager集成
 */
struct PhysicsEventQueryComponent {
    using is_event_component = void;
    
    // 继承现有查询结构，增加事件相关功能
    std::vector<PhysicsQueryComponent::RaycastQuery> raycast_queries;
    std::vector<PhysicsQueryComponent::OverlapQuery> overlap_queries;
    std::vector<PhysicsQueryComponent::DistanceQuery> distance_queries;
    
    // 事件驱动特性
    bool has_pending_queries = false;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    // 查询设置
    float last_update_time = 0.0f;
    float update_interval = 0.1f;    // 查询更新间隔
    int max_queries_per_frame = 10;  // 每帧最大查询数
    bool auto_cleanup_results = true; // 是否自动清理查询结果
    
    // 事件回调设置
    bool notify_on_raycast_hit = true;
    bool notify_on_overlap_change = true;
    bool notify_on_distance_change = true;
    
    PhysicsEventQueryComponent() = default;
    
    // 基于现有PhysicsQueryComponent的兼容方法
    void add_raycast(const Vec3& origin, const Vec3& direction, float max_distance = 1000.0f, uint32_t layer_mask = 0xFFFFFFFF) {
        PhysicsQueryComponent::RaycastQuery query;
        query.origin = origin;
        query.direction = direction.Normalized();
        query.max_distance = max_distance;
        query.layer_mask = layer_mask;
        raycast_queries.push_back(query);
        has_pending_queries = true;
    }
    
    // 添加球体重叠查询 (兼容现有接口)
    void add_sphere_overlap(const Vec3& center, float radius, uint32_t layer_mask = 0xFFFFFFFF) {
        PhysicsQueryComponent::OverlapQuery query;
        query.shape = PhysicsQueryComponent::OverlapQuery::SPHERE;
        query.center = center;
        query.size = Vec3(radius, radius, radius);
        query.layer_mask = layer_mask;
        overlap_queries.push_back(query);
        has_pending_queries = true;
    }
    
    // 添加盒体重叠查询 (兼容现有接口)
    void add_box_overlap(const Vec3& center, const Vec3& half_extents, const Quat& rotation = Quat(0, 0, 0, 1), uint32_t layer_mask = 0xFFFFFFFF) {
        PhysicsQueryComponent::OverlapQuery query;
        query.shape = PhysicsQueryComponent::OverlapQuery::BOX;
        query.center = center;
        query.size = half_extents;
        query.rotation = rotation;
        query.layer_mask = layer_mask;
        overlap_queries.push_back(query);
        has_pending_queries = true;
    }
    
    // 添加距离查询 (兼容现有接口)
    void add_distance_query(const Vec3& point, float max_distance = 100.0f, uint32_t layer_mask = 0xFFFFFFFF) {
        PhysicsQueryComponent::DistanceQuery query;
        query.point = point;
        query.max_distance = max_distance;
        query.layer_mask = layer_mask;
        distance_queries.push_back(query);
        has_pending_queries = true;
    }
    
    // 清空所有查询
    void clear_queries() {
        raycast_queries.clear();
        overlap_queries.clear();
        distance_queries.clear();
        has_pending_queries = false;
    }
    
    // 检查是否有待处理的查询
    bool has_queries() const {
        return !raycast_queries.empty() || !overlap_queries.empty() || !distance_queries.empty();
    }
    
    // 获取总查询数
    size_t get_total_query_count() const {
        return raycast_queries.size() + overlap_queries.size() + distance_queries.size();
    }
};

/**
 * 待处理查询标记组件
 * 标记需要处理物理查询的实体
 */
struct PendingQueryTag {
    using is_event_component = void;
    
    float added_time = 0.0f;
    int priority = 0;  // 优先级，数值越低优先级越高
    bool use_physics_command = false;  // 是否使用现有的PhysicsCommandComponent
    
    PendingQueryTag() = default;
    PendingQueryTag(int prio, bool use_cmd = false) : priority(prio), use_physics_command(use_cmd) {}
};

/**
 * 查询请求事件
 * 用于请求物理查询的事件
 */
struct RequestRaycastEvent : public PhysicsEventBase {
    entt::entity requester;
    Vec3 origin;
    Vec3 direction;
    float max_distance = 100.0f;
    uint32_t layer_mask = 0xFFFFFFFF;
    std::string request_id;
    
    RequestRaycastEvent() = default;
    RequestRaycastEvent(entt::entity req, const Vec3& orig, const Vec3& dir, float max_dist = 100.0f,
                       PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), origin(orig), direction(dir), max_distance(max_dist) {}
};

/**
 * 区域监控请求事件
 */
struct RequestAreaMonitoringEvent : public PhysicsEventBase {
    entt::entity requester;
    Vec3 center;
    float radius = 1.0f;
    uint32_t layer_mask = 0xFFFFFFFF;
    float update_interval = 0.1f;
    std::string monitor_id;
    
    RequestAreaMonitoringEvent() = default;
    RequestAreaMonitoringEvent(entt::entity req, const Vec3& c, float r,
                              PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), center(c), radius(r) {}
};

// === 懒加载查询工厂函数 ===

/**
 * 查询工厂类
 * 提供便捷的查询创建方法，支持懒加载
 */
class PhysicsQueryFactory {
public:
    /**
     * 为实体创建射线查询（懒加载）
     * 如果实体没有查询组件，会自动创建
     */
    static void create_raycast_query(entt::registry& registry, entt::entity entity,
                                   const Vec3& origin, const Vec3& direction, float max_distance = 1000.0f) {
        // 检查是否已有事件查询组件
        if (!registry.all_of<PhysicsEventQueryComponent>(entity)) {
            registry.emplace<PhysicsEventQueryComponent>(entity);
        }
        
        auto& query_comp = registry.get<PhysicsEventQueryComponent>(entity);
        query_comp.add_raycast(origin, direction, max_distance);
        
        // 标记需要处理
        if (!registry.all_of<PendingQueryTag>(entity)) {
            registry.emplace<PendingQueryTag>(entity);
        }
    }
    
    /**
     * 为实体创建区域监控（懒加载）
     */
    static void create_area_monitoring(entt::registry& registry, entt::entity entity,
                                     const Vec3& center, float radius, uint32_t layer_mask = 0xFFFFFFFF) {
        // 检查是否已有查询组件
        if (!registry.all_of<PhysicsEventQueryComponent>(entity)) {
            auto& query_comp = registry.emplace<PhysicsEventQueryComponent>(entity);
            query_comp.add_sphere_overlap(center, radius, layer_mask);
        } else {
            auto& query_comp = registry.get<PhysicsEventQueryComponent>(entity);
            query_comp.add_sphere_overlap(center, radius, layer_mask);
        }
        
        // 标记需要处理
        if (!registry.all_of<PendingQueryTag>(entity)) {
            registry.emplace<PendingQueryTag>(entity);
        }
    }
    
    /**
     * 使用现有PhysicsCommandComponent创建查询（兼容性）
     */
    static void create_raycast_via_command(entt::registry& registry, entt::entity entity,
                                         const Vec3& origin, const Vec3& direction, float max_distance = 1000.0f) {
        // 检查是否已有命令组件
        if (!registry.all_of<PhysicsCommandComponent>(entity)) {
            registry.emplace<PhysicsCommandComponent>(entity);
        }
        
        auto& cmd_comp = registry.get<PhysicsCommandComponent>(entity);
        
        // 使用自定义命令创建射线查询（因为RAYCAST命令需要特殊处理）
        cmd_comp.add_custom_command([origin, direction, max_distance]() {
            // 这里将在物理系统中处理射线查询
            // 实际的查询逻辑由物理系统处理
        }, PhysicsCommandTiming::BEFORE_PHYSICS_STEP);
        
        // 标记使用命令系统
        if (!registry.all_of<PendingQueryTag>(entity)) {
            registry.emplace<PendingQueryTag>(entity, 0, true);  // use_physics_command = true
        }
    }
};

} // namespace portal_core
