#ifndef PHYSICS_COMPONENTS_H
#define PHYSICS_COMPONENTS_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include "../portal_core/lib/include/portal_types.h"

namespace Portal {

/**
 * 物理身體組件 - 與Jolt Physics Body關聯
 */
struct PhysicsBodyComponent {
    JPH::BodyID body_id;           // Jolt物理身體ID
    bool is_dynamic;               // 是否為動態物體
    bool is_kinematic;             // 是否為運動學物體
    float mass;                    // 質量
    float restitution;             // 彈性係數
    float friction;                // 摩擦係數
    
    PhysicsBodyComponent() 
        : body_id(JPH::BodyID()), is_dynamic(true), is_kinematic(false)
        , mass(1.0f), restitution(0.5f), friction(0.5f) {}
};

/**
 * 變換組件 - 位置、旋轉、縮放
 */
struct TransformComponent {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    
    TransformComponent() : scale(1.0f, 1.0f, 1.0f) {}
    
    Transform to_portal_transform() const {
        return Transform(position, rotation, scale);
    }
    
    void from_portal_transform(const Transform& transform) {
        position = transform.position;
        rotation = transform.rotation;
        scale = transform.scale;
    }
    
    // 與Jolt物理引擎的轉換
    JPH::Vec3 to_jolt_position() const {
        return JPH::Vec3(position.x, position.y, position.z);
    }
    
    JPH::Quat to_jolt_rotation() const {
        return JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);
    }
    
    void from_jolt_transform(const JPH::Vec3& pos, const JPH::Quat& rot) {
        position = Vector3(pos.GetX(), pos.GetY(), pos.GetZ());
        rotation = Quaternion(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
    }
};

/**
 * 速度組件
 */
struct VelocityComponent {
    Vector3 linear_velocity;
    Vector3 angular_velocity;
    
    VelocityComponent() = default;
    
    JPH::Vec3 to_jolt_linear() const {
        return JPH::Vec3(linear_velocity.x, linear_velocity.y, linear_velocity.z);
    }
    
    JPH::Vec3 to_jolt_angular() const {
        return JPH::Vec3(angular_velocity.x, angular_velocity.y, angular_velocity.z);
    }
    
    void from_jolt_velocity(const JPH::Vec3& linear, const JPH::Vec3& angular) {
        linear_velocity = Vector3(linear.GetX(), linear.GetY(), linear.GetZ());
        angular_velocity = Vector3(angular.GetX(), angular.GetY(), angular.GetZ());
    }
};

/**
 * 碰撞形狀組件
 */
enum class CollisionShapeType {
    Box,
    Sphere,
    Capsule,
    ConvexHull,
    Mesh
};

struct CollisionShapeComponent {
    CollisionShapeType shape_type;
    Vector3 dimensions;    // 對於Box是尺寸，對於Sphere是半徑等
    
    CollisionShapeComponent(CollisionShapeType type = CollisionShapeType::Box) 
        : shape_type(type), dimensions(1.0f, 1.0f, 1.0f) {}
};

/**
 * 傳送門相關組件
 */
struct PortalEntityComponent {
    PortalId associated_portal;
    bool can_teleport;
    bool is_teleporting;
    
    PortalEntityComponent() 
        : associated_portal(INVALID_PORTAL_ID)
        , can_teleport(true), is_teleporting(false) {}
};

/**
 * 幽靈碰撞體組件 - 用於傳送門穿越期間
 */
struct GhostColliderComponent {
    JPH::BodyID ghost_body_id;     // 幽靈碰撞體的ID
    PortalId source_portal; // 源傳送門
    PortalId target_portal; // 目標傳送門
    bool is_active;
    
    GhostColliderComponent() 
        : ghost_body_id(JPH::BodyID())
        , source_portal(INVALID_PORTAL_ID)
        , target_portal(INVALID_PORTAL_ID)
        , is_active(false) {}
};

/**
 * 渲染組件 - 基本的渲染屬性
 */
struct RenderComponent {
    bool visible;
    float opacity;
    bool cast_shadows;
    bool receive_shadows;
    
    RenderComponent() 
        : visible(true), opacity(1.0f)
        , cast_shadows(true), receive_shadows(true) {}
};

} // namespace Portal

#endif // PHYSICS_COMPONENTS_H
