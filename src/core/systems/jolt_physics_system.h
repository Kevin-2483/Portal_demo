#ifndef JOLT_PHYSICS_SYSTEM_H
#define JOLT_PHYSICS_SYSTEM_H

#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>

#include "../components/physics_components.h"
#include "../portal_core/lib/include/portal_interfaces.h"
#include <memory>

namespace Portal {

// 物理層定義
namespace PhysicsLayers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace PhysicsBroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}

// 廣播相位層接口實現
class BPLayerInterface : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterface() {
        mObjectToBroadPhase[PhysicsLayers::NON_MOVING] = PhysicsBroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[PhysicsLayers::MOVING] = PhysicsBroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override {
        return PhysicsBroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < PhysicsLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[PhysicsLayers::NUM_LAYERS];
};

// 對象層過濾器實現
class ObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case PhysicsLayers::NON_MOVING:
                return inObject2 == PhysicsLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// 對象vs廣播相位層過濾器實現
class ObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case PhysicsLayers::NON_MOVING:
                return inLayer2 == PhysicsBroadPhaseLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

/**
 * Jolt物理系統 - 實現Portal物理接口
 */
class JoltPhysicsSystem : public IPhysicsQuery, public IPhysicsManipulator {
public:
    JoltPhysicsSystem();
    ~JoltPhysicsSystem();

    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // 每幀更新
    void update(float delta_time);
    
    // ECS整合
    void set_registry(entt::registry* registry) { registry_ = registry; }
    entt::registry* get_registry() const { return registry_; }

    // === 實體管理 ===
    
    /**
     * 創建物理實體
     */
    entt::entity create_physics_entity(
        const TransformComponent& transform,
        const CollisionShapeComponent& collision_shape,
        const PhysicsBodyComponent& physics_body
    );
    
    /**
     * 銷毀物理實體
     */
    void destroy_physics_entity(entt::entity entity);
    
    /**
     * 從EntityId轉換為entt::entity（需要維護映射）
     */
    entt::entity entity_id_to_entt_entity(EntityId entity_id) const;
    EntityId entt_entity_to_entity_id(entt::entity entity) const;

    // === IPhysicsQuery 實現 ===
    
    Transform get_entity_transform(EntityId entity_id) const override;
    PhysicsState get_entity_physics_state(EntityId entity_id) const override;
    bool is_entity_valid(EntityId entity_id) const override;
    void get_entity_bounds(EntityId entity_id, Vector3& min_bounds, Vector3& max_bounds) const override;
    bool raycast(const Vector3& start, const Vector3& end, EntityId ignore_entity = INVALID_ENTITY_ID) const override;

    // === IPhysicsManipulator 實現 ===
    
    void set_entity_transform(EntityId entity_id, const Transform& transform) override;
    void set_entity_physics_state(EntityId entity_id, const PhysicsState& physics_state) override;
    void set_entity_collision_enabled(EntityId entity_id, bool enabled) override;
    
    // 幽靈碰撞體管理
    bool create_ghost_collider(EntityId entity_id, const Transform& ghost_transform) override;
    void update_ghost_collider(EntityId entity_id, const Transform& ghost_transform, const PhysicsState& ghost_physics) override;
    void destroy_ghost_collider(EntityId entity_id) override;
    bool has_ghost_collider(EntityId entity_id) const override;

    // === 物理系統訪問 ===
    JPH::PhysicsSystem* get_jolt_physics_system() { return physics_system_.get(); }
    const JPH::PhysicsSystem* get_jolt_physics_system() const { return physics_system_.get(); }

private:
    // Jolt Physics 初始化
    void setup_jolt();
    void cleanup_jolt();
    
    // 形狀創建輔助方法
    JPH::Ref<JPH::Shape> create_jolt_shape(const CollisionShapeComponent& collision_shape) const;
    
    // 同步ECS與Jolt Physics
    void sync_transforms_from_jolt();
    void sync_velocities_from_jolt();
    void sync_transform_to_jolt(entt::entity entity);
    void sync_velocity_to_jolt(entt::entity entity);
    
    // ID映射管理
    void register_entity_mapping(entt::entity entt_entity, EntityId entity_id);
    void unregister_entity_mapping(entt::entity entt_entity);

    // 核心Jolt組件
    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator_;
    std::unique_ptr<JPH::JobSystemThreadPool> job_system_;
    std::unique_ptr<JPH::PhysicsSystem> physics_system_;
    
    // Jolt 接口組件
    std::unique_ptr<BPLayerInterface> broad_phase_layer_interface_;
    std::unique_ptr<ObjectVsBroadPhaseLayerFilter> object_vs_broad_phase_layer_filter_;
    std::unique_ptr<ObjectLayerPairFilter> object_vs_object_layer_filter_;
    
    // ECS註冊表
    entt::registry* registry_;
    
    // ID映射表
    std::unordered_map<EntityId, entt::entity> entity_id_to_entt_;
    std::unordered_map<entt::entity, EntityId> entt_to_entity_id_;
    EntityId next_entity_id_;
    
    // 物理設定
    const unsigned int max_bodies_ = 10240;
    const unsigned int max_body_pairs_ = 65536;
    const unsigned int max_contact_constraints_ = 10240;
    const unsigned int num_body_mutexes_ = 0;
    
    bool is_initialized_;
};

/**
 * 物理更新系統 - 處理ECS組件的同步
 */
class PhysicsUpdateSystem {
public:
    explicit PhysicsUpdateSystem(JoltPhysicsSystem* physics_system)
        : physics_system_(physics_system) {}

    void update(float delta_time);

private:
    void update_transforms();
    void update_velocities();
    void update_ghost_colliders();

    JoltPhysicsSystem* physics_system_;
};

} // namespace Portal

#endif // JOLT_PHYSICS_SYSTEM_H
