#pragma once

#include "physics_event_types.h"
#include <unordered_set>

namespace portal_core {

// === 实体事件组件类型 ===

/**
 * 区域监控组件
 * 持续监控指定区域内的实体
 */
struct AreaMonitorComponent {
    using is_event_component = void;  // 标记为事件组件
    
    Vec3 center;
    float radius = 1.0f;
    std::unordered_set<entt::entity> entities_in_area;
    uint32_t layer_mask = 0xFFFFFFFF;
    bool active = true;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    // 监控状态
    size_t last_entity_count = 0;
    float last_update_time = 0.0f;
    float update_interval = 0.1f;  // 更新间隔（秒）
    
    AreaMonitorComponent() = default;
    AreaMonitorComponent(const Vec3& c, float r, PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : center(c), radius(r), dimension(dim) {}
        
    // 检查实体是否在区域内
    bool contains_entity(entt::entity entity) const {
        return entities_in_area.find(entity) != entities_in_area.end();
    }
    
    // 获取区域内实体数量
    size_t get_entity_count() const {
        return entities_in_area.size();
    }
};

/**
 * 包含检测组件
 * 检测实体是否在特定边界内
 */
struct ContainmentComponent {
    using is_event_component = void;
    
    Vec3 bounds_min;
    Vec3 bounds_max;
    std::unordered_set<entt::entity> contained_entities;
    bool active = true;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    // 检测设置
    bool notify_on_enter = true;
    bool notify_on_exit = true;
    float last_check_time = 0.0f;
    float check_interval = 0.1f;
    
    ContainmentComponent() = default;
    ContainmentComponent(const Vec3& min_bounds, const Vec3& max_bounds, 
                        PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : bounds_min(min_bounds), bounds_max(max_bounds), dimension(dim) {}
    
    // 检查点是否在边界内
    bool contains_point(const Vec3& point) const {
        if (dimension == PhysicsEventDimension::DIMENSION_2D) {
            return point.GetX() >= bounds_min.GetX() && point.GetX() <= bounds_max.GetX() &&
                   point.GetY() >= bounds_min.GetY() && point.GetY() <= bounds_max.GetY();
        } else {
            return point.GetX() >= bounds_min.GetX() && point.GetX() <= bounds_max.GetX() &&
                   point.GetY() >= bounds_min.GetY() && point.GetY() <= bounds_max.GetY() &&
                   point.GetZ() >= bounds_min.GetZ() && point.GetZ() <= bounds_max.GetZ();
        }
    }
};

/**
 * 平面相交组件
 * 检测实体与平面的相交状态
 */
struct PlaneIntersectionComponent {
    using is_event_component = void;
    
    Vec3 plane_normal;
    float plane_distance = 0.0f;
    entt::entity monitored_entity;
    bool was_above = true;           // 上次检测时是否在平面上方
    bool active = true;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    // 相交检测设置
    float last_check_time = 0.0f;
    float check_interval = 0.016f;   // 每帧检测
    bool notify_on_cross = true;
    
    PlaneIntersectionComponent() = default;
    PlaneIntersectionComponent(const Vec3& normal, float distance, entt::entity entity,
                              PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : plane_normal(normal), plane_distance(distance), monitored_entity(entity), dimension(dim) {}
    
    // 检查点是否在平面上方
    bool is_point_above(const Vec3& point) const {
        float distance_to_plane = plane_normal.Dot(point) - plane_distance;
        return distance_to_plane > 0.0f;
    }
};

/**
 * 持续碰撞组件
 * 跟踪持续的碰撞状态
 */
struct PersistentContactComponent {
    using is_event_component = void;
    
    entt::entity other_entity;
    Vec3 contact_point;
    Vec3 contact_normal;
    float contact_duration = 0.0f;
    float contact_force = 0.0f;
    bool active = true;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    // 持续接触设置
    float force_threshold = 0.1f;    // 力的阈值
    float duration_threshold = 0.5f; // 持续时间阈值
    bool notify_on_threshold = true;
    
    PersistentContactComponent() = default;
    PersistentContactComponent(entt::entity other, const Vec3& point, const Vec3& normal,
                              PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : other_entity(other), contact_point(point), contact_normal(normal), dimension(dim) {}
};

/**
 * 区域状态更新组件
 * 当区域监控状态发生变化时使用
 */
struct AreaStatusUpdateComponent {
    using is_event_component = void;
    
    size_t current_entity_count = 0;
    size_t previous_entity_count = 0;
    float update_time = 0.0f;
    bool entities_added = false;
    bool entities_removed = false;
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    
    AreaStatusUpdateComponent() = default;
    AreaStatusUpdateComponent(size_t count, PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : current_entity_count(count), dimension(dim) {}
};

} // namespace portal_core
