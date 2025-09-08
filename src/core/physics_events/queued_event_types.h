#pragma once

#include "physics_event_types.h"
#include <vector>

namespace portal_core {

// === 队列事件类型 ===

/**
 * 射线检测结果事件
 * 用于批量处理射线检测结果
 */
struct RaycastResultEvent : public PhysicsEventBase {
    entt::entity requester;        // 请求射线检测的实体
    bool hit = false;              // 是否命中
    Vec3 hit_point;                // 命中点
    Vec3 hit_normal;               // 命中表面法线
    float hit_distance = 0.0f;     // 命中距离
    entt::entity hit_entity;       // 命中的实体
    std::string raycast_type = "raycast_result";
    
    RaycastResultEvent() = default;
    RaycastResultEvent(entt::entity req, bool h, const Vec3& point, const Vec3& normal,
                      float distance, entt::entity hit_ent, 
                      PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), hit(h), hit_point(point),
          hit_normal(normal), hit_distance(distance), hit_entity(hit_ent) {}
};

/**
 * 形状查询结果事件
 * 用于复杂形状碰撞检测结果
 */
struct ShapeQueryResultEvent : public PhysicsEventBase {
    entt::entity requester;
    bool has_overlap = false;
    std::vector<entt::entity> overlapping_entities;
    Vec3 query_center;
    Vec3 query_extents;
    std::string query_type = "shape_query";
    
    ShapeQueryResultEvent() = default;
    ShapeQueryResultEvent(entt::entity req, const Vec3& center, const Vec3& extents,
                         PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), query_center(center), query_extents(extents) {}
};

/**
 * 物体激活事件
 * 当物体从睡眠状态激活时触发
 */
struct BodyActivationEvent : public PhysicsEventBase {
    entt::entity entity;
    bool is_active = true;         // true=激活, false=睡眠
    Vec3 activation_position;
    std::string activation_reason = "physics";
    
    BodyActivationEvent() = default;
    BodyActivationEvent(entt::entity ent, bool active, const Vec3& pos,
                       PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), entity(ent), is_active(active), activation_position(pos) {}
};

/**
 * 距离查询结果事件
 * 用于最近物体或范围查询
 */
struct DistanceQueryResultEvent : public PhysicsEventBase {
    entt::entity requester;
    std::vector<std::pair<entt::entity, float>> entities_with_distance;  // 实体和距离对
    Vec3 query_center;
    float max_distance = 0.0f;
    std::string query_type = "distance_query";
    
    DistanceQueryResultEvent() = default;
    DistanceQueryResultEvent(entt::entity req, const Vec3& center, float max_dist,
                            PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), query_center(center), max_distance(max_dist) {}
};

/**
 * 重叠查询结果事件
 * 用于区域重叠检测结果
 */
struct OverlapQueryResultEvent : public PhysicsEventBase {
    entt::entity requester;
    std::vector<entt::entity> overlapping_entities;
    Vec3 query_center;
    float query_radius = 0.0f;
    uint32_t layer_mask = 0xFFFFFFFF;
    std::string query_type = "overlap_query";
    
    OverlapQueryResultEvent() = default;
    OverlapQueryResultEvent(entt::entity req, const Vec3& center, float radius,
                           PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), requester(req), query_center(center), query_radius(radius) {}
};

} // namespace portal_core
