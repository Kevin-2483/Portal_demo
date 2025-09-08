#pragma once

#include "../math_types.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <unordered_set>

namespace portal_core {

// 物理事件维度类型
enum class PhysicsEventDimension {
    DIMENSION_2D = 2,
    DIMENSION_3D = 3,
    AUTO_DETECT = 0  // 根据坐标自动检测
};

// 基础物理事件接口
struct PhysicsEventBase {
    PhysicsEventDimension dimension = PhysicsEventDimension::AUTO_DETECT;
    std::string event_id;
    float timestamp = 0.0f;
    
    PhysicsEventBase() = default;
    PhysicsEventBase(PhysicsEventDimension dim, const std::string& id = "") 
        : dimension(dim), event_id(id) {}
};

// === 立即事件类型 ===

/**
 * 碰撞开始事件
 * 当两个物体开始碰撞时触发
 */
struct CollisionStartEvent : public PhysicsEventBase {
    entt::entity entity_a;
    entt::entity entity_b;
    Vec3 contact_point;
    Vec3 contact_normal;
    float impact_force = 0.0f;
    std::string collision_type = "collision_start";
    
    CollisionStartEvent() = default;
    CollisionStartEvent(entt::entity a, entt::entity b, const Vec3& point, const Vec3& normal, 
                       float force = 0.0f, PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), entity_a(a), entity_b(b), contact_point(point), 
          contact_normal(normal), impact_force(force) {}
};

/**
 * 碰撞结束事件
 * 当两个物体停止碰撞时触发
 */
struct CollisionEndEvent : public PhysicsEventBase {
    entt::entity entity_a;
    entt::entity entity_b;
    Vec3 last_contact_point;
    float contact_duration = 0.0f;
    std::string collision_type = "collision_end";
    
    CollisionEndEvent() = default;
    CollisionEndEvent(entt::entity a, entt::entity b, const Vec3& point = Vec3::sZero(), 
                     float duration = 0.0f, PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), entity_a(a), entity_b(b), 
          last_contact_point(point), contact_duration(duration) {}
};

/**
 * 触发器进入事件
 * 当物体进入触发器区域时触发
 */
struct TriggerEnterEvent : public PhysicsEventBase {
    entt::entity sensor_entity;    // 触发器实体
    entt::entity other_entity;     // 进入的实体
    Vec3 contact_point;
    Vec3 contact_normal;
    std::string trigger_type = "trigger_enter";
    
    TriggerEnterEvent() = default;
    TriggerEnterEvent(entt::entity sensor, entt::entity other, const Vec3& point, const Vec3& normal,
                     PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), sensor_entity(sensor), other_entity(other),
          contact_point(point), contact_normal(normal) {}
};

/**
 * 触发器退出事件
 * 当物体离开触发器区域时触发
 */
struct TriggerExitEvent : public PhysicsEventBase {
    entt::entity sensor_entity;    // 触发器实体
    entt::entity other_entity;     // 离开的实体
    Vec3 last_contact_point;
    float contact_duration = 0.0f;
    std::string trigger_type = "trigger_exit";
    
    TriggerExitEvent() = default;
    TriggerExitEvent(entt::entity sensor, entt::entity other, const Vec3& point = Vec3::sZero(),
                    float duration = 0.0f, PhysicsEventDimension dim = PhysicsEventDimension::AUTO_DETECT)
        : PhysicsEventBase(dim), sensor_entity(sensor), other_entity(other),
          last_contact_point(point), contact_duration(duration) {}
};

} // namespace portal_core
