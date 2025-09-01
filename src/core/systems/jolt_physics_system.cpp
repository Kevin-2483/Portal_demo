#include "jolt_physics_system.h"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <iostream>

// 必需的Jolt Physics宏
JPH_SUPPRESS_WARNINGS

namespace Portal {

JoltPhysicsSystem::JoltPhysicsSystem()
    : registry_(nullptr)
    , next_entity_id_(1)
    , is_initialized_(false)
{
}

JoltPhysicsSystem::~JoltPhysicsSystem() {
    if (is_initialized_) {
        shutdown();
    }
}

bool JoltPhysicsSystem::initialize() {
    if (is_initialized_) {
        return true;
    }

    try {
        setup_jolt();
        is_initialized_ = true;
        std::cout << "Jolt Physics System initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Jolt Physics System: " << e.what() << std::endl;
        cleanup_jolt();
        return false;
    }
}

void JoltPhysicsSystem::shutdown() {
    if (!is_initialized_) {
        return;
    }

    cleanup_jolt();
    entity_id_to_entt_.clear();
    entt_to_entity_id_.clear();
    is_initialized_ = false;
    std::cout << "Jolt Physics System shutdown" << std::endl;
}

void JoltPhysicsSystem::setup_jolt() {
    // 註冊所有Jolt Physics類型
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // 創建臨時分配器
    temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB

    // 創建工作系統
    job_system_ = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, 
        JPH::cMaxPhysicsBarriers, 
        std::thread::hardware_concurrency() - 1
    );

    // 創建廣播相位層介面和對象vs廣播相位層過濾器
    broad_phase_layer_interface_ = std::make_unique<Portal::BPLayerInterface>();
    object_vs_broad_phase_layer_filter_ = std::make_unique<Portal::ObjectVsBroadPhaseLayerFilter>();
    object_vs_object_layer_filter_ = std::make_unique<Portal::ObjectLayerPairFilter>();

    // 創建物理系統
    physics_system_ = std::make_unique<JPH::PhysicsSystem>();
    physics_system_->Init(
        max_bodies_, 
        num_body_mutexes_, 
        max_body_pairs_, 
        max_contact_constraints_,
        *broad_phase_layer_interface_,
        *object_vs_broad_phase_layer_filter_,
        *object_vs_object_layer_filter_
    );

    // 設置重力
    physics_system_->SetGravity(JPH::Vec3(0, -9.81f, 0));
}

void JoltPhysicsSystem::cleanup_jolt() {
    physics_system_.reset();
    job_system_.reset();
    temp_allocator_.reset();

    if (JPH::Factory::sInstance) {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
    JPH::UnregisterTypes();
}

void JoltPhysicsSystem::update(float delta_time) {
    if (!is_initialized_ || !physics_system_) {
        return;
    }

    // 更新Jolt Physics
    const int collision_steps = 1;
    physics_system_->Update(delta_time, collision_steps, temp_allocator_.get(), job_system_.get());

    // 同步ECS組件
    sync_transforms_from_jolt();
    sync_velocities_from_jolt();
}

entt::entity JoltPhysicsSystem::create_physics_entity(
    const TransformComponent& transform,
    const CollisionShapeComponent& collision_shape,
    const PhysicsBodyComponent& physics_body
) {
    if (!registry_) {
        throw std::runtime_error("Registry not set");
    }

    // 創建ECS實體
    entt::entity entity = registry_->create();
    
    // 添加組件
    registry_->emplace<TransformComponent>(entity, transform);
    registry_->emplace<CollisionShapeComponent>(entity, collision_shape);
    registry_->emplace<VelocityComponent>(entity);

    // 創建Jolt物理身體
    JPH::Ref<JPH::Shape> shape = create_jolt_shape(collision_shape);
    
    JPH::BodyCreationSettings body_settings(
        shape,
        transform.to_jolt_position(),
        transform.to_jolt_rotation(),
        physics_body.is_dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
        physics_body.is_dynamic ? Portal::PhysicsLayers::MOVING : Portal::PhysicsLayers::NON_MOVING
    );

    body_settings.mMassPropertiesOverride.mMass = physics_body.mass;
    body_settings.mRestitution = physics_body.restitution;
    body_settings.mFriction = physics_body.friction;

    JPH::Body* body = physics_system_->GetBodyInterface().CreateBody(body_settings);
    if (!body) {
        registry_->destroy(entity);
        throw std::runtime_error("Failed to create Jolt physics body");
    }

    // 更新物理身體組件
    PhysicsBodyComponent physics_comp = physics_body;
    physics_comp.body_id = body->GetID();
    registry_->emplace<PhysicsBodyComponent>(entity, physics_comp);

    // 添加到物理世界
    physics_system_->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

    // 註冊ID映射
    EntityId entity_id = next_entity_id_++;
    register_entity_mapping(entity, entity_id);

    return entity;
}

void JoltPhysicsSystem::destroy_physics_entity(entt::entity entity) {
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    // 銷毀Jolt物理身體
    if (auto* physics_body = registry_->try_get<PhysicsBodyComponent>(entity)) {
        physics_system_->GetBodyInterface().RemoveBody(physics_body->body_id);
        physics_system_->GetBodyInterface().DestroyBody(physics_body->body_id);
    }

    // 銷毀幽靈碰撞體
    if (auto* ghost = registry_->try_get<GhostColliderComponent>(entity)) {
        if (ghost->is_active) {
            physics_system_->GetBodyInterface().RemoveBody(ghost->ghost_body_id);
            physics_system_->GetBodyInterface().DestroyBody(ghost->ghost_body_id);
        }
    }

    // 取消ID映射
    unregister_entity_mapping(entity);

    // 銷毀ECS實體
    registry_->destroy(entity);
}

JPH::Ref<JPH::Shape> JoltPhysicsSystem::create_jolt_shape(const CollisionShapeComponent& collision_shape) const {
    switch (collision_shape.shape_type) {
        case CollisionShapeType::Box:
            return new JPH::BoxShape(JPH::Vec3(
                collision_shape.dimensions.x * 0.5f,
                collision_shape.dimensions.y * 0.5f,
                collision_shape.dimensions.z * 0.5f
            ));

        case CollisionShapeType::Sphere:
            return new JPH::SphereShape(collision_shape.dimensions.x);

        case CollisionShapeType::Capsule:
            return new JPH::CapsuleShape(
                collision_shape.dimensions.y * 0.5f,  // 半高
                collision_shape.dimensions.x          // 半徑
            );

        default:
            // 默認使用Box形狀
            return new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
    }
}

// === IPhysicsQuery 實現 ===

Transform JoltPhysicsSystem::get_entity_transform(EntityId entity_id) const {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return Transform();
    }

    const auto* transform_comp = registry_->try_get<TransformComponent>(entity);
    if (!transform_comp) {
        return Transform();
    }

    return transform_comp->to_portal_transform();
}

PhysicsState JoltPhysicsSystem::get_entity_physics_state(EntityId entity_id) const {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return PhysicsState();
    }

    const auto* velocity_comp = registry_->try_get<VelocityComponent>(entity);
    const auto* physics_comp = registry_->try_get<PhysicsBodyComponent>(entity);
    
    PhysicsState state;
    if (velocity_comp) {
        state.linear_velocity = velocity_comp->linear_velocity;
        state.angular_velocity = velocity_comp->angular_velocity;
    }
    if (physics_comp) {
        state.mass = physics_comp->mass;
    }

    return state;
}

bool JoltPhysicsSystem::is_entity_valid(EntityId entity_id) const {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    return registry_ && registry_->valid(entity);
}

void JoltPhysicsSystem::get_entity_bounds(EntityId entity_id, Vector3& min_bounds, Vector3& max_bounds) const {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        min_bounds = Vector3(-0.5f, -0.5f, -0.5f);
        max_bounds = Vector3(0.5f, 0.5f, 0.5f);
        return;
    }

    const auto* collision_shape = registry_->try_get<CollisionShapeComponent>(entity);
    if (!collision_shape) {
        min_bounds = Vector3(-0.5f, -0.5f, -0.5f);
        max_bounds = Vector3(0.5f, 0.5f, 0.5f);
        return;
    }

    // 根據形狀類型計算邊界框
    switch (collision_shape->shape_type) {
        case CollisionShapeType::Box:
            min_bounds = Vector3(
                -collision_shape->dimensions.x * 0.5f,
                -collision_shape->dimensions.y * 0.5f,
                -collision_shape->dimensions.z * 0.5f
            );
            max_bounds = Vector3(
                collision_shape->dimensions.x * 0.5f,
                collision_shape->dimensions.y * 0.5f,
                collision_shape->dimensions.z * 0.5f
            );
            break;

        case CollisionShapeType::Sphere: {
            float radius = collision_shape->dimensions.x;
            min_bounds = Vector3(-radius, -radius, -radius);
            max_bounds = Vector3(radius, radius, radius);
            break;
        }

        case CollisionShapeType::Capsule: {
            float radius = collision_shape->dimensions.x;
            float half_height = collision_shape->dimensions.y * 0.5f;
            min_bounds = Vector3(-radius, -half_height - radius, -radius);
            max_bounds = Vector3(radius, half_height + radius, radius);
            break;
        }

        default:
            min_bounds = Vector3(-0.5f, -0.5f, -0.5f);
            max_bounds = Vector3(0.5f, 0.5f, 0.5f);
            break;
    }
}

bool JoltPhysicsSystem::raycast(const Vector3& start, const Vector3& end, EntityId ignore_entity) const {
    if (!physics_system_) {
        return false;
    }

    JPH::Vec3 jolt_start(start.x, start.y, start.z);
    JPH::Vec3 jolt_direction = JPH::Vec3(end.x - start.x, end.y - start.y, end.z - start.z);

    JPH::RRayCast ray(JPH::RVec3(jolt_start), jolt_direction);
    JPH::RayCastResult result;

    JPH::BodyID ignore_body_id;
    if (ignore_entity != INVALID_ENTITY_ID) {
        entt::entity ignore_entt_entity = entity_id_to_entt_entity(ignore_entity);
        if (registry_ && registry_->valid(ignore_entt_entity)) {
            const auto* physics_comp = registry_->try_get<PhysicsBodyComponent>(ignore_entt_entity);
            if (physics_comp) {
                ignore_body_id = physics_comp->body_id;
            }
        }
    }

    // 執行射線檢測  
    bool hit = physics_system_->GetNarrowPhaseQuery().CastRay(ray, result);

    // 如果命中的是忽略的實體，則視為未命中
    if (hit && result.mBodyID == ignore_body_id) {
        return false;
    }

    return hit;
}

// === IPhysicsManipulator 實現 ===

void JoltPhysicsSystem::set_entity_transform(EntityId entity_id, const Transform& transform) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    // 更新ECS組件
    auto* transform_comp = registry_->try_get<TransformComponent>(entity);
    if (transform_comp) {
        transform_comp->from_portal_transform(transform);
    }

    // 同步到Jolt Physics
    sync_transform_to_jolt(entity);
}

void JoltPhysicsSystem::set_entity_physics_state(EntityId entity_id, const PhysicsState& physics_state) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    // 更新速度組件
    auto* velocity_comp = registry_->try_get<VelocityComponent>(entity);
    if (velocity_comp) {
        velocity_comp->linear_velocity = physics_state.linear_velocity;
        velocity_comp->angular_velocity = physics_state.angular_velocity;
    }

    // 同步到Jolt Physics
    sync_velocity_to_jolt(entity);
}

void JoltPhysicsSystem::set_entity_collision_enabled(EntityId entity_id, bool enabled) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    const auto* physics_comp = registry_->try_get<PhysicsBodyComponent>(entity);
    if (!physics_comp) {
        return;
    }

    // 在Jolt中啟用/禁用碰撞
    if (enabled) {
        physics_system_->GetBodyInterface().AddBody(physics_comp->body_id, JPH::EActivation::DontActivate);
    } else {
        physics_system_->GetBodyInterface().RemoveBody(physics_comp->body_id);
    }
}

// === 幽靈碰撞體管理 ===

bool JoltPhysicsSystem::create_ghost_collider(EntityId entity_id, const Transform& ghost_transform) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return false;
    }

    // 檢查是否已經存在幽靈碰撞體
    auto* ghost_comp = registry_->try_get<GhostColliderComponent>(entity);
    if (!ghost_comp) {
        ghost_comp = &registry_->emplace<GhostColliderComponent>(entity);
    }

    if (ghost_comp->is_active) {
        return true; // 已經存在
    }

    // 獲取原始碰撞形狀
    const auto* collision_shape = registry_->try_get<CollisionShapeComponent>(entity);
    const auto* physics_body = registry_->try_get<PhysicsBodyComponent>(entity);
    
    if (!collision_shape || !physics_body) {
        return false;
    }

    // 創建幽靈碰撞體
    JPH::Ref<JPH::Shape> shape = create_jolt_shape(*collision_shape);
    
    TransformComponent ghost_transform_comp;
    ghost_transform_comp.from_portal_transform(ghost_transform);

    JPH::BodyCreationSettings ghost_settings(
        shape,
        ghost_transform_comp.to_jolt_position(),
        ghost_transform_comp.to_jolt_rotation(),
        JPH::EMotionType::Kinematic, // 幽靈碰撞體通常是運動學的
        Portal::PhysicsLayers::MOVING
    );

    JPH::Body* ghost_body = physics_system_->GetBodyInterface().CreateBody(ghost_settings);
    if (!ghost_body) {
        return false;
    }

    ghost_comp->ghost_body_id = ghost_body->GetID();
    ghost_comp->is_active = true;

    physics_system_->GetBodyInterface().AddBody(ghost_body->GetID(), JPH::EActivation::DontActivate);

    return true;
}

void JoltPhysicsSystem::update_ghost_collider(EntityId entity_id, const Transform& ghost_transform, const PhysicsState& ghost_physics) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    auto* ghost_comp = registry_->try_get<GhostColliderComponent>(entity);
    if (!ghost_comp || !ghost_comp->is_active) {
        return;
    }

    TransformComponent ghost_transform_comp;
    ghost_transform_comp.from_portal_transform(ghost_transform);

    VelocityComponent ghost_velocity_comp;
    ghost_velocity_comp.linear_velocity = ghost_physics.linear_velocity;
    ghost_velocity_comp.angular_velocity = ghost_physics.angular_velocity;

    // 更新幽靈碰撞體的位置和速度
    JPH::BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.SetPositionAndRotation(
        ghost_comp->ghost_body_id,
        ghost_transform_comp.to_jolt_position(),
        ghost_transform_comp.to_jolt_rotation(),
        JPH::EActivation::DontActivate
    );

    body_interface.SetLinearAndAngularVelocity(
        ghost_comp->ghost_body_id,
        ghost_velocity_comp.to_jolt_linear(),
        ghost_velocity_comp.to_jolt_angular()
    );
}

void JoltPhysicsSystem::destroy_ghost_collider(EntityId entity_id) {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return;
    }

    auto* ghost_comp = registry_->try_get<GhostColliderComponent>(entity);
    if (!ghost_comp || !ghost_comp->is_active) {
        return;
    }

    physics_system_->GetBodyInterface().RemoveBody(ghost_comp->ghost_body_id);
    physics_system_->GetBodyInterface().DestroyBody(ghost_comp->ghost_body_id);

    ghost_comp->ghost_body_id = JPH::BodyID();
    ghost_comp->is_active = false;
}

bool JoltPhysicsSystem::has_ghost_collider(EntityId entity_id) const {
    entt::entity entity = entity_id_to_entt_entity(entity_id);
    if (!registry_ || !registry_->valid(entity)) {
        return false;
    }

    const auto* ghost_comp = registry_->try_get<GhostColliderComponent>(entity);
    return ghost_comp && ghost_comp->is_active;
}

// === 私有輔助方法 ===

void JoltPhysicsSystem::sync_transforms_from_jolt() {
    if (!registry_) return;

    auto view = registry_->view<TransformComponent, PhysicsBodyComponent>();
    view.each([&](auto entity, auto& transform_comp, const auto& physics_comp) {
        if (physics_comp.is_dynamic) {
            JPH::Vec3 position;
            JPH::Quat rotation;
            physics_system_->GetBodyInterface().GetPositionAndRotation(
                physics_comp.body_id, position, rotation
            );

            transform_comp.from_jolt_transform(position, rotation);
        }
    });
}

void JoltPhysicsSystem::sync_velocities_from_jolt() {
    if (!registry_) return;

    auto view = registry_->view<VelocityComponent, PhysicsBodyComponent>();
    view.each([&](auto entity, auto& velocity_comp, const auto& physics_comp) {
        if (physics_comp.is_dynamic) {
            JPH::Vec3 linear_velocity = physics_system_->GetBodyInterface().GetLinearVelocity(physics_comp.body_id);
            JPH::Vec3 angular_velocity = physics_system_->GetBodyInterface().GetAngularVelocity(physics_comp.body_id);

            velocity_comp.from_jolt_velocity(linear_velocity, angular_velocity);
        }
    });
}

void JoltPhysicsSystem::sync_transform_to_jolt(entt::entity entity) {
    if (!registry_) return;

    const auto* transform_comp = registry_->try_get<TransformComponent>(entity);
    const auto* physics_comp = registry_->try_get<PhysicsBodyComponent>(entity);

    if (transform_comp && physics_comp) {
        physics_system_->GetBodyInterface().SetPositionAndRotation(
            physics_comp->body_id,
            transform_comp->to_jolt_position(),
            transform_comp->to_jolt_rotation(),
            JPH::EActivation::Activate
        );
    }
}

void JoltPhysicsSystem::sync_velocity_to_jolt(entt::entity entity) {
    if (!registry_) return;

    const auto* velocity_comp = registry_->try_get<VelocityComponent>(entity);
    const auto* physics_comp = registry_->try_get<PhysicsBodyComponent>(entity);

    if (velocity_comp && physics_comp) {
        physics_system_->GetBodyInterface().SetLinearAndAngularVelocity(
            physics_comp->body_id,
            velocity_comp->to_jolt_linear(),
            velocity_comp->to_jolt_angular()
        );
    }
}

// === ID映射管理 ===

entt::entity JoltPhysicsSystem::entity_id_to_entt_entity(EntityId entity_id) const {
    auto it = entity_id_to_entt_.find(entity_id);
    if (it != entity_id_to_entt_.end()) {
        return it->second;
    }
    return entt::null;
}

EntityId JoltPhysicsSystem::entt_entity_to_entity_id(entt::entity entity) const {
    auto it = entt_to_entity_id_.find(entity);
    if (it != entt_to_entity_id_.end()) {
        return it->second;
    }
    return INVALID_ENTITY_ID;
}

void JoltPhysicsSystem::register_entity_mapping(entt::entity entt_entity, EntityId entity_id) {
    entity_id_to_entt_[entity_id] = entt_entity;
    entt_to_entity_id_[entt_entity] = entity_id;
}

void JoltPhysicsSystem::unregister_entity_mapping(entt::entity entt_entity) {
    auto it = entt_to_entity_id_.find(entt_entity);
    if (it != entt_to_entity_id_.end()) {
        EntityId entity_id = it->second;
        entity_id_to_entt_.erase(entity_id);
        entt_to_entity_id_.erase(it);
    }
}

// === PhysicsUpdateSystem 實現 ===

void PhysicsUpdateSystem::update(float delta_time) {
    update_transforms();
    update_velocities();
    update_ghost_colliders();
}

void PhysicsUpdateSystem::update_transforms() {
    // 由JoltPhysicsSystem::update()調用sync_transforms_from_jolt()處理
}

void PhysicsUpdateSystem::update_velocities() {
    // 由JoltPhysicsSystem::update()調用sync_velocities_from_jolt()處理
}

void PhysicsUpdateSystem::update_ghost_colliders() {
    // 這裡可以添加幽靈碰撞體的特殊更新邏輯
    // 例如檢查穿越狀態，決定是否需要更新幽靈碰撞體位置
}

} // namespace Portal
