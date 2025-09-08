#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include "math_types.h"
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/CollideShape.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <thread>

JPH_SUPPRESS_WARNINGS

namespace portal_core {

// 使用Jolt的命名空間
using namespace JPH;
using namespace JPH::literals;

// 物理層次定義
namespace PhysicsLayers {
    static constexpr ObjectLayer STATIC = 0;
    static constexpr ObjectLayer DYNAMIC = 1;
    static constexpr ObjectLayer KINEMATIC = 2;
    static constexpr ObjectLayer TRIGGER = 3;
    static constexpr ObjectLayer NUM_LAYERS = 4;
}

// 寬相位層次
namespace PhysicsBroadPhaseLayers {
    static constexpr BroadPhaseLayer STATIC(0);
    static constexpr BroadPhaseLayer DYNAMIC(1);
    static constexpr BroadPhaseLayer KINEMATIC(2);
    static constexpr BroadPhaseLayer TRIGGER(3);
    static constexpr uint NUM_LAYERS(4);
}

// 物理體類型
enum class PhysicsBodyType {
    STATIC,      // 靜態物體
    DYNAMIC,     // 動態物體
    KINEMATIC,   // 運動學物體
    TRIGGER      // 觸發器
};

// 形狀類型
enum class PhysicsShapeType {
    BOX,
    SPHERE,
    CAPSULE,
    CYLINDER,
    CONVEX_HULL,
    MESH,
    HEIGHT_FIELD
};

// 物理材質
struct PhysicsMaterial {
    float friction = 0.2f;
    float restitution = 0.0f;
    float density = 1000.0f;
    
    PhysicsMaterial() = default;
    PhysicsMaterial(float f, float r, float d) : friction(f), restitution(r), density(d) {}
};

// 物理形狀描述
struct PhysicsShapeDesc {
    PhysicsShapeType type = PhysicsShapeType::BOX;
    Vec3 size = Vec3(1.0f, 1.0f, 1.0f);  // 盒子尺寸或球體半徑等
    std::vector<Vec3> vertices;           // 凸包或網格的頂點
    std::vector<uint32_t> indices;        // 網格的索引
    float radius = 0.5f;                  // 球體/膠囊/圓柱體半徑
    float height = 1.0f;                  // 膠囊/圓柱體高度
    
    PhysicsShapeDesc() = default;
    
    // 預定義形狀
    static PhysicsShapeDesc box(const Vec3& size) {
        PhysicsShapeDesc desc;
        desc.type = PhysicsShapeType::BOX;
        desc.size = size;
        return desc;
    }
    
    static PhysicsShapeDesc sphere(float radius) {
        PhysicsShapeDesc desc;
        desc.type = PhysicsShapeType::SPHERE;
        desc.radius = radius;
        return desc;
    }
    
    static PhysicsShapeDesc capsule(float radius, float height) {
        PhysicsShapeDesc desc;
        desc.type = PhysicsShapeType::CAPSULE;
        desc.radius = radius;
        desc.height = height;
        return desc;
    }
};

// 物理體創建設定
struct PhysicsBodyDesc {
    PhysicsBodyType body_type = PhysicsBodyType::DYNAMIC;
    PhysicsShapeDesc shape;
    PhysicsMaterial material;
    JPH::RVec3 position = JPH::RVec3::sZero();
    JPH::Quat rotation = JPH::Quat::sIdentity();
    JPH::Vec3 linear_velocity = JPH::Vec3::sZero();
    JPH::Vec3 angular_velocity = JPH::Vec3::sZero();
    bool allow_sleeping = true;
    float motion_quality = 1.0f;  // 運動品質設定
    uint64_t user_data = 0;       // 用戶數據
    
    PhysicsBodyDesc() = default;
};

// 對象層過濾器實現
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override;
};

// 寬相位層接口實現
class BroadPhaseLayerInterfaceImpl : public BroadPhaseLayerInterface {
public:
    BroadPhaseLayerInterfaceImpl();
    
    virtual uint GetNumBroadPhaseLayers() const override;
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override;
    
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override;
#endif

private:
    BroadPhaseLayer object_to_broad_phase_[PhysicsLayers::NUM_LAYERS];
};

// 對象與寬相位層過濾器實現
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override;
};

// 接觸監聽器
class PhysicsContactListener : public ContactListener {
public:
    // 接觸驗證回調
    virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, 
                                           RVec3Arg inBaseOffset, 
                                           const CollideShapeResult& inCollisionResult) override;
    
    // 接觸添加回調
    virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, 
                               const ContactManifold& inManifold, 
                               ContactSettings& ioSettings) override;
    
    // 接觸持續回調
    virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, 
                                   const ContactManifold& inManifold, 
                                   ContactSettings& ioSettings) override;
    
    // 接觸移除回調
    virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override;
    
    // 事件回調函數類型 - 包含詳細接觸信息
    // 参数：body1_id, body2_id, contact_point, contact_normal, impulse_magnitude
    using ContactEventCallback = std::function<void(BodyID, BodyID, const Vec3&, const Vec3&, float)>;
    
    void set_contact_added_callback(ContactEventCallback callback) { 
        contact_added_callback_ = std::move(callback); 
    }
    
    void set_contact_removed_callback(ContactEventCallback callback) { 
        contact_removed_callback_ = std::move(callback); 
    }

private:
    ContactEventCallback contact_added_callback_;
    ContactEventCallback contact_removed_callback_;
};

// 身體激活監聽器
class PhysicsActivationListener : public BodyActivationListener {
public:
    virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override;
    virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override;
    
    // 事件回調函數類型
    using ActivationEventCallback = std::function<void(BodyID, uint64)>;
    
    void set_body_activated_callback(ActivationEventCallback callback) { 
        body_activated_callback_ = std::move(callback); 
    }
    
    void set_body_deactivated_callback(ActivationEventCallback callback) { 
        body_deactivated_callback_ = std::move(callback); 
    }

private:
    ActivationEventCallback body_activated_callback_;
    ActivationEventCallback body_deactivated_callback_;
};

// 物理世界管理器
class PhysicsWorldManager {
public:
    PhysicsWorldManager();
    ~PhysicsWorldManager();
    
    // 單例實例
    static PhysicsWorldManager& get_instance();
    
    // 初始化和清理
    bool initialize(const PhysicsSettings& settings = PhysicsSettings());
    void cleanup();
    bool is_initialized() const { return initialized_; }
    
    // 物理步進
    void update(float delta_time);
    void set_fixed_timestep(float timestep) { fixed_timestep_ = timestep; }
    
    // 物理體管理
    BodyID create_body(const PhysicsBodyDesc& desc);
    void destroy_body(BodyID body_id);
    bool has_body(BodyID body_id) const;
    
    // 物理體控制
    void set_body_position(BodyID body_id, const RVec3& position);
    void set_body_rotation(BodyID body_id, const Quat& rotation);
    void set_body_linear_velocity(BodyID body_id, const Vec3& velocity);
    void set_body_angular_velocity(BodyID body_id, const Vec3& velocity);
    void add_force(BodyID body_id, const Vec3& force);
    void add_impulse(BodyID body_id, const Vec3& impulse);
    void add_torque(BodyID body_id, const Vec3& torque);
    void add_angular_impulse(BodyID body_id, const Vec3& impulse);
    
    // 物理體查詢
    RVec3 get_body_position(BodyID body_id) const;
    Quat get_body_rotation(BodyID body_id) const;
    Vec3 get_body_linear_velocity(BodyID body_id) const;
    Vec3 get_body_angular_velocity(BodyID body_id) const;
    bool is_body_active(BodyID body_id) const;
    
    // 物理查詢
    struct RaycastResult {
        bool hit = false;
        BodyID body_id;
        Vec3 hit_point;
        Vec3 hit_normal;
        float distance = 0.0f;
    };
    
    RaycastResult raycast(const RVec3& origin, const Vec3& direction, float max_distance = 1000.0f);
    std::vector<BodyID> overlap_sphere(const RVec3& center, float radius);
    std::vector<BodyID> overlap_box(const RVec3& center, const Vec3& half_extents, const Quat& rotation = Quat::sIdentity());
    
    // 事件回調設置
    void set_contact_added_callback(PhysicsContactListener::ContactEventCallback callback);
    void set_contact_removed_callback(PhysicsContactListener::ContactEventCallback callback);
    void set_body_activated_callback(PhysicsActivationListener::ActivationEventCallback callback);
    void set_body_deactivated_callback(PhysicsActivationListener::ActivationEventCallback callback);
    
    // 獲取底層系統訪問（高級用途）
    JPH::PhysicsSystem& get_physics_system() { return *physics_system_; }
    const JPH::PhysicsSystem& get_physics_system() const { return *physics_system_; }
    JPH::BodyInterface& get_body_interface() { return physics_system_->GetBodyInterface(); }
    const JPH::BodyInterface& get_body_interface() const { return physics_system_->GetBodyInterface(); }
    
    // 世界設定
    void set_gravity(const Vec3& gravity);
    Vec3 get_gravity() const;
    
    // 調試和統計
    void enable_debug_rendering(bool enable) { debug_rendering_enabled_ = enable; }
    bool is_debug_rendering_enabled() const { return debug_rendering_enabled_; }
    
    struct PhysicsStats {
        uint32_t num_bodies = 0;
        uint32_t num_active_bodies = 0;
        uint32_t num_contacts = 0;
        float simulation_time = 0.0f;
    };
    
    PhysicsStats get_stats() const;

private:
    // 內部初始化
    bool initialize_jolt();
    void cleanup_jolt();
    
    // 形狀創建輔助函數
    RefConst<Shape> create_shape(const PhysicsShapeDesc& desc);
    ObjectLayer get_object_layer(PhysicsBodyType type);
    EMotionType get_motion_type(PhysicsBodyType type);
    
    // 初始化狀態
    bool initialized_ = false;
    
    // Jolt Physics 組件
    std::unique_ptr<JPH::PhysicsSystem> physics_system_;
    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator_;
    std::unique_ptr<JPH::JobSystemThreadPool> job_system_;
    
    // 過濾器和監聽器
    std::unique_ptr<BroadPhaseLayerInterfaceImpl> broad_phase_layer_interface_;
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> object_vs_broad_phase_layer_filter_;
    std::unique_ptr<ObjectLayerPairFilterImpl> object_vs_object_layer_filter_;
    std::unique_ptr<PhysicsContactListener> contact_listener_;
    std::unique_ptr<PhysicsActivationListener> activation_listener_;
    
    // 時間管理
    float fixed_timestep_ = 1.0f / 60.0f;
    float accumulated_time_ = 0.0f;
    int collision_steps_ = 1;
    
    // 調試設定
    bool debug_rendering_enabled_ = false;
    
    // 統計數據
    mutable PhysicsStats last_stats_;
    
    // 靜態實例
    static std::unique_ptr<PhysicsWorldManager> instance_;
};

} // namespace portal_core
