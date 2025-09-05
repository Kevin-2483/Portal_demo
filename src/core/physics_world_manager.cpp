#include "physics_world_manager.h"
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <iostream>
#include <cstdarg>

// Jolt Physics 錯誤處理回調
static void TraceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << "[Jolt] " << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    std::cerr << "[Jolt Assert] " << inFile << ":" << inLine << ": (" << inExpression << ") " 
              << (inMessage != nullptr ? inMessage : "") << std::endl;
    return true;
}
#endif

namespace portal_core {

// 靜態實例
std::unique_ptr<PhysicsWorldManager> PhysicsWorldManager::instance_ = nullptr;

// ObjectLayerPairFilterImpl 實現
bool ObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const {
    switch (inObject1) {
        case PhysicsLayers::STATIC:
            return inObject2 == PhysicsLayers::DYNAMIC || inObject2 == PhysicsLayers::KINEMATIC;
        case PhysicsLayers::DYNAMIC:
            return true; // 動態物體與所有層碰撞
        case PhysicsLayers::KINEMATIC:
            return inObject2 == PhysicsLayers::STATIC || inObject2 == PhysicsLayers::DYNAMIC;
        case PhysicsLayers::TRIGGER:
            return inObject2 == PhysicsLayers::DYNAMIC || inObject2 == PhysicsLayers::KINEMATIC;
        default:
            JPH_ASSERT(false);
            return false;
    }
}

// BroadPhaseLayerInterfaceImpl 實現
BroadPhaseLayerInterfaceImpl::BroadPhaseLayerInterfaceImpl() {
    object_to_broad_phase_[PhysicsLayers::STATIC] = PhysicsBroadPhaseLayers::STATIC;
    object_to_broad_phase_[PhysicsLayers::DYNAMIC] = PhysicsBroadPhaseLayers::DYNAMIC;
    object_to_broad_phase_[PhysicsLayers::KINEMATIC] = PhysicsBroadPhaseLayers::KINEMATIC;
    object_to_broad_phase_[PhysicsLayers::TRIGGER] = PhysicsBroadPhaseLayers::TRIGGER;
}

uint BroadPhaseLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
    return PhysicsBroadPhaseLayers::NUM_LAYERS;
}

BroadPhaseLayer BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer inLayer) const {
    JPH_ASSERT(inLayer < PhysicsLayers::NUM_LAYERS);
    return object_to_broad_phase_[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const {
    switch ((BroadPhaseLayer::Type)inLayer) {
        case (BroadPhaseLayer::Type)PhysicsBroadPhaseLayers::STATIC: return "STATIC";
        case (BroadPhaseLayer::Type)PhysicsBroadPhaseLayers::DYNAMIC: return "DYNAMIC";
        case (BroadPhaseLayer::Type)PhysicsBroadPhaseLayers::KINEMATIC: return "KINEMATIC";
        case (BroadPhaseLayer::Type)PhysicsBroadPhaseLayers::TRIGGER: return "TRIGGER";
        default: JPH_ASSERT(false); return "INVALID";
    }
}
#endif

// ObjectVsBroadPhaseLayerFilterImpl 實現
bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const {
    switch (inLayer1) {
        case PhysicsLayers::STATIC:
            return inLayer2 == PhysicsBroadPhaseLayers::DYNAMIC || inLayer2 == PhysicsBroadPhaseLayers::KINEMATIC;
        case PhysicsLayers::DYNAMIC:
            return true;
        case PhysicsLayers::KINEMATIC:
            return inLayer2 == PhysicsBroadPhaseLayers::STATIC || inLayer2 == PhysicsBroadPhaseLayers::DYNAMIC;
        case PhysicsLayers::TRIGGER:
            return inLayer2 == PhysicsBroadPhaseLayers::DYNAMIC || inLayer2 == PhysicsBroadPhaseLayers::KINEMATIC;
        default:
            JPH_ASSERT(false);
            return false;
    }
}

// PhysicsContactListener 實現
    ValidateResult PhysicsContactListener::OnContactValidate(
    const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, 
    const CollideShapeResult& inCollisionResult) {
    // 允許所有接觸，可以在這裡添加自定義過濾邏輯
    return ValidateResult::AcceptAllContactsForThisBodyPair;
}

void PhysicsContactListener::OnContactAdded(const Body& inBody1, const Body& inBody2, 
                                          const ContactManifold& inManifold, 
                                          ContactSettings& ioSettings) {
    if (contact_added_callback_) {
        contact_added_callback_(inBody1.GetID(), inBody2.GetID());
    }
}

void PhysicsContactListener::OnContactPersisted(const Body& inBody1, const Body& inBody2, 
                                              const ContactManifold& inManifold, 
                                              ContactSettings& ioSettings) {
    // 可以在這裡處理持續接觸邏輯
}

void PhysicsContactListener::OnContactRemoved(const SubShapeIDPair& inSubShapePair) {
    if (contact_removed_callback_) {
        // 注意：在接觸移除時，我們沒有直接的Body引用，需要從SubShapeIDPair獲取
        contact_removed_callback_(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());
    }
}

// PhysicsActivationListener 實現
void PhysicsActivationListener::OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) {
    if (body_activated_callback_) {
        body_activated_callback_(inBodyID, inBodyUserData);
    }
}

void PhysicsActivationListener::OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) {
    if (body_deactivated_callback_) {
        body_deactivated_callback_(inBodyID, inBodyUserData);
    }
}

// PhysicsWorldManager 實現
PhysicsWorldManager::PhysicsWorldManager() = default;

PhysicsWorldManager::~PhysicsWorldManager() {
    cleanup();
}

PhysicsWorldManager& PhysicsWorldManager::get_instance() {
    if (!instance_) {
        instance_ = std::make_unique<PhysicsWorldManager>();
    }
    return *instance_;
}

bool PhysicsWorldManager::initialize(const PhysicsSettings& settings) {
    if (initialized_) {
        std::cout << "PhysicsWorldManager: Already initialized." << std::endl;
        return true;
    }

    std::cout << "PhysicsWorldManager: Initializing Jolt Physics..." << std::endl;

    if (!initialize_jolt()) {
        std::cerr << "PhysicsWorldManager: Failed to initialize Jolt Physics!" << std::endl;
        return false;
    }

    // 創建過濾器和監聽器
    broad_phase_layer_interface_ = std::make_unique<BroadPhaseLayerInterfaceImpl>();
    object_vs_broad_phase_layer_filter_ = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    object_vs_object_layer_filter_ = std::make_unique<ObjectLayerPairFilterImpl>();
    contact_listener_ = std::make_unique<PhysicsContactListener>();
    activation_listener_ = std::make_unique<PhysicsActivationListener>();

    // 創建臨時分配器
    const uint cTempAllocatorSize = 10 * 1024 * 1024; // 10 MB
    temp_allocator_ = std::make_unique<TempAllocatorImpl>(cTempAllocatorSize);

    // 創建作業系統（使用所有可用核心-1）
    uint num_threads = std::max(1u, std::thread::hardware_concurrency() - 1);
    job_system_ = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, num_threads);

    // 物理系統設定
    const uint cMaxBodies = 65536;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 65536;
    const uint cMaxContactConstraints = 10240;

    // 創建物理系統
    physics_system_ = std::make_unique<PhysicsSystem>();
    physics_system_->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                         *broad_phase_layer_interface_, *object_vs_broad_phase_layer_filter_, 
                         *object_vs_object_layer_filter_);

    // 設置監聽器
    physics_system_->SetBodyActivationListener(activation_listener_.get());
    physics_system_->SetContactListener(contact_listener_.get());

    // 設置物理設定
    physics_system_->SetPhysicsSettings(settings);

    // 優化寬相位
    physics_system_->OptimizeBroadPhase();

    initialized_ = true;
    std::cout << "PhysicsWorldManager: Initialization complete." << std::endl;
    return true;
}

void PhysicsWorldManager::cleanup() {
    if (!initialized_) {
        return;
    }

    std::cout << "PhysicsWorldManager: Cleaning up..." << std::endl;

    // 清理物理系統
    physics_system_.reset();
    job_system_.reset();
    temp_allocator_.reset();

    // 清理過濾器和監聽器
    activation_listener_.reset();
    contact_listener_.reset();
    object_vs_object_layer_filter_.reset();
    object_vs_broad_phase_layer_filter_.reset();
    broad_phase_layer_interface_.reset();

    cleanup_jolt();

    initialized_ = false;
    std::cout << "PhysicsWorldManager: Cleanup complete." << std::endl;
}

bool PhysicsWorldManager::initialize_jolt() {
    // 註冊默認分配器
    RegisterDefaultAllocator();

    // 安裝trace和assert回調
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

    // 創建工廠
    Factory::sInstance = new Factory();

    // 註冊所有物理類型
    RegisterTypes();

    std::cout << "PhysicsWorldManager: Jolt Physics core initialized." << std::endl;
    return true;
}

void PhysicsWorldManager::cleanup_jolt() {
    // 清理類型註冊
    UnregisterTypes();

    // 刪除工廠
    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    std::cout << "PhysicsWorldManager: Jolt Physics core cleaned up." << std::endl;
}

void PhysicsWorldManager::update(float delta_time) {
    if (!initialized_) {
        return;
    }

    accumulated_time_ += delta_time;

    // 固定時間步進
    while (accumulated_time_ >= fixed_timestep_) {
        physics_system_->Update(fixed_timestep_, collision_steps_, temp_allocator_.get(), job_system_.get());
        accumulated_time_ -= fixed_timestep_;
    }
}

BodyID PhysicsWorldManager::create_body(const PhysicsBodyDesc& desc) {
    if (!initialized_) {
        return BodyID();
    }

    // 創建形狀
    RefConst<Shape> shape = create_shape(desc.shape);
    if (!shape) {
        std::cerr << "PhysicsWorldManager: Failed to create shape for body." << std::endl;
        return BodyID();
    }

    // 創建物理材質（直接在BodyCreationSettings中設置）
    // Jolt中摩擦力和彈性係數是直接在BodyCreationSettings中設置的

    // 創建body設定
    BodyCreationSettings body_settings(shape, desc.position, desc.rotation, 
                                     get_motion_type(desc.body_type), get_object_layer(desc.body_type));
    
    body_settings.mLinearVelocity = desc.linear_velocity;
    body_settings.mAngularVelocity = desc.angular_velocity;
    body_settings.mAllowSleeping = desc.allow_sleeping;
    body_settings.mUserData = desc.user_data;
    body_settings.mFriction = desc.material.friction;
    body_settings.mRestitution = desc.material.restitution;

    // 只對動態和運動學物體設置質量屬性，靜態物體不需要
    if (desc.body_type != PhysicsBodyType::STATIC && desc.body_type != PhysicsBodyType::TRIGGER) {
        // 動態和運動學物體設置質量
        body_settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        body_settings.mMassPropertiesOverride.mMass = desc.material.density;
    }
    // 靜態物體不設置任何質量屬性，保持默認的 CalculateMassAndInertia

    // 創建並添加body，靜態物體不需要激活
    EActivation activation = (desc.body_type == PhysicsBodyType::STATIC || desc.body_type == PhysicsBodyType::TRIGGER) 
                           ? EActivation::DontActivate : EActivation::Activate;

    // 創建並添加body
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    BodyID body_id = body_interface.CreateAndAddBody(body_settings, activation);

    if (body_id.IsInvalid()) {
        std::cerr << "PhysicsWorldManager: Failed to create physics body." << std::endl;
    }

    return body_id;
}

void PhysicsWorldManager::destroy_body(BodyID body_id) {
    if (!initialized_ || body_id.IsInvalid()) {
        return;
    }

    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.RemoveBody(body_id);
    body_interface.DestroyBody(body_id);
}

bool PhysicsWorldManager::has_body(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) {
        return false;
    }

    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.IsAdded(body_id);
}

RefConst<Shape> PhysicsWorldManager::create_shape(const PhysicsShapeDesc& desc) {
    switch (desc.type) {
        case PhysicsShapeType::BOX: {
            return new BoxShape(desc.size * 0.5f); // Jolt使用半尺寸
        }
        case PhysicsShapeType::SPHERE: {
            return new SphereShape(desc.radius);
        }
        case PhysicsShapeType::CAPSULE: {
            return new CapsuleShape(desc.height * 0.5f, desc.radius);
        }
        case PhysicsShapeType::CYLINDER: {
            return new CylinderShape(desc.height * 0.5f, desc.radius);
        }
        case PhysicsShapeType::CONVEX_HULL: {
            if (desc.vertices.empty()) {
                std::cerr << "PhysicsWorldManager: No vertices provided for convex hull." << std::endl;
                return nullptr;
            }
            Array<Vec3> vertices;
            for (const Vec3& v : desc.vertices) {
                vertices.push_back(v);
            }
            ConvexHullShapeSettings settings(vertices);
            ShapeSettings::ShapeResult result = settings.Create();
            if (result.HasError()) {
                std::cerr << "PhysicsWorldManager: Failed to create convex hull: " 
                         << result.GetError().c_str() << std::endl;
                return nullptr;
            }
            return result.Get();
        }
        case PhysicsShapeType::MESH: {
            if (desc.vertices.empty() || desc.indices.empty()) {
                std::cerr << "PhysicsWorldManager: No vertices or indices provided for mesh." << std::endl;
                return nullptr;
            }
            
            // 創建三角形列表
            TriangleList triangles;
            for (size_t i = 0; i < desc.indices.size(); i += 3) {
                if (i + 2 < desc.indices.size()) {
                    Triangle triangle(desc.vertices[desc.indices[i]], 
                                    desc.vertices[desc.indices[i + 1]], 
                                    desc.vertices[desc.indices[i + 2]]);
                    triangles.push_back(triangle);
                }
            }
            
            MeshShapeSettings settings(triangles);
            ShapeSettings::ShapeResult result = settings.Create();
            if (result.HasError()) {
                std::cerr << "PhysicsWorldManager: Failed to create mesh: " 
                         << result.GetError().c_str() << std::endl;
                return nullptr;
            }
            return result.Get();
        }
        default:
            std::cerr << "PhysicsWorldManager: Unsupported shape type." << std::endl;
            return nullptr;
    }
}

ObjectLayer PhysicsWorldManager::get_object_layer(PhysicsBodyType type) {
    switch (type) {
        case PhysicsBodyType::STATIC: return PhysicsLayers::STATIC;
        case PhysicsBodyType::DYNAMIC: return PhysicsLayers::DYNAMIC;
        case PhysicsBodyType::KINEMATIC: return PhysicsLayers::KINEMATIC;
        case PhysicsBodyType::TRIGGER: return PhysicsLayers::TRIGGER;
        default: return PhysicsLayers::DYNAMIC;
    }
}

EMotionType PhysicsWorldManager::get_motion_type(PhysicsBodyType type) {
    switch (type) {
        case PhysicsBodyType::STATIC: return EMotionType::Static;
        case PhysicsBodyType::DYNAMIC: return EMotionType::Dynamic;
        case PhysicsBodyType::KINEMATIC: return EMotionType::Kinematic;
        case PhysicsBodyType::TRIGGER: return EMotionType::Static; // 觸發器通常是靜態的
        default: return EMotionType::Dynamic;
    }
}

// 物理體控制方法
void PhysicsWorldManager::set_body_position(BodyID body_id, const RVec3& position) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.SetPosition(body_id, position, EActivation::Activate);
}

void PhysicsWorldManager::set_body_rotation(BodyID body_id, const Quat& rotation) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.SetRotation(body_id, rotation, EActivation::Activate);
}

void PhysicsWorldManager::set_body_linear_velocity(BodyID body_id, const Vec3& velocity) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.SetLinearVelocity(body_id, velocity);
}

void PhysicsWorldManager::set_body_angular_velocity(BodyID body_id, const Vec3& velocity) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.SetAngularVelocity(body_id, velocity);
}

void PhysicsWorldManager::add_force(BodyID body_id, const Vec3& force) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.AddForce(body_id, force);
}

void PhysicsWorldManager::add_impulse(BodyID body_id, const Vec3& impulse) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.AddImpulse(body_id, impulse);
}

void PhysicsWorldManager::add_torque(BodyID body_id, const Vec3& torque) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.AddTorque(body_id, torque);
}

void PhysicsWorldManager::add_angular_impulse(BodyID body_id, const Vec3& impulse) {
    if (!initialized_ || body_id.IsInvalid()) return;
    BodyInterface& body_interface = physics_system_->GetBodyInterface();
    body_interface.AddAngularImpulse(body_id, impulse);
}

// 物理體查詢方法
RVec3 PhysicsWorldManager::get_body_position(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) return RVec3::sZero();
    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.GetCenterOfMassPosition(body_id);
}

Quat PhysicsWorldManager::get_body_rotation(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) return Quat::sIdentity();
    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.GetRotation(body_id);
}

Vec3 PhysicsWorldManager::get_body_linear_velocity(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) return Vec3::sZero();
    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.GetLinearVelocity(body_id);
}

Vec3 PhysicsWorldManager::get_body_angular_velocity(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) return Vec3::sZero();
    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.GetAngularVelocity(body_id);
}

bool PhysicsWorldManager::is_body_active(BodyID body_id) const {
    if (!initialized_ || body_id.IsInvalid()) return false;
    const BodyInterface& body_interface = physics_system_->GetBodyInterface();
    return body_interface.IsActive(body_id);
}

// 物理查詢方法
PhysicsWorldManager::RaycastResult PhysicsWorldManager::raycast(const RVec3& origin, const Vec3& direction, float max_distance) {
    RaycastResult result;
    if (!initialized_) return result;

    RRayCast ray;
    ray.mOrigin = origin;
    ray.mDirection = direction * max_distance;
    RayCastResult hit;
    
    if (physics_system_->GetNarrowPhaseQuery().CastRay(ray, hit)) {
        result.hit = true;
        result.body_id = hit.mBodyID;
        result.hit_point = Vec3(origin + direction * hit.mFraction * max_distance);
        result.distance = hit.mFraction * max_distance;
        
        // 獲取法線需要更詳細的查詢
        const BodyLockInterface& lock_interface = physics_system_->GetBodyLockInterface();
        BodyLockRead lock(lock_interface, result.body_id);
        if (lock.Succeeded()) {
            const Body& body = lock.GetBody();
            // 使用Body的GetWorldSpaceSurfaceNormal方法獲取表面法線
            Vec3 contact_point = result.hit_point;
            result.hit_normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, RVec3(contact_point));
        }
    }

    return result;
}

std::vector<BodyID> PhysicsWorldManager::overlap_sphere(const RVec3 &center, float radius)
{
  std::vector<BodyID> results;
  if (!initialized_)
    return results;

  // 創建球體形狀進行重疊測試
  Ref<SphereShape> sphere = new SphereShape(radius);

  AllHitCollisionCollector<CollideShapeCollector> collector;
  RMat44 transform = RMat44::sTranslation(center);
  
  // 創建CollideShapeSettings
  CollideShapeSettings settings;
  settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
  settings.mCollectFacesMode = ECollectFacesMode::NoFaces;
  
  physics_system_->GetNarrowPhaseQuery().CollideShape(sphere, Vec3::sReplicate(1.0f),
                                                     transform, settings, RVec3::sZero(), collector);

  for (const CollideShapeResult &hit : collector.mHits)
  {
    results.push_back(hit.mBodyID2);
  }

  return results;
}

std::vector<BodyID> PhysicsWorldManager::overlap_box(const RVec3& center, const Vec3& half_extents, const Quat& rotation) {
    std::vector<BodyID> results;
    if (!initialized_) return results;

    // 創建盒子形狀進行重疊測試
    Ref<BoxShape> box = new BoxShape(half_extents);
    
    AllHitCollisionCollector<CollideShapeCollector> collector;
    RMat44 transform = RMat44::sRotationTranslation(rotation, center);
    
    // 創建CollideShapeSettings
    CollideShapeSettings settings;
    settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
    settings.mCollectFacesMode = ECollectFacesMode::NoFaces;
    
    physics_system_->GetNarrowPhaseQuery().CollideShape(box, Vec3::sReplicate(1.0f), 
                                                       transform, settings, RVec3::sZero(), collector);

    for (const CollideShapeResult& hit : collector.mHits) {
        results.push_back(hit.mBodyID2);
    }

    return results;
}

// 事件回調設置
void PhysicsWorldManager::set_contact_added_callback(PhysicsContactListener::ContactEventCallback callback) {
    if (contact_listener_) {
        contact_listener_->set_contact_added_callback(std::move(callback));
    }
}

void PhysicsWorldManager::set_contact_removed_callback(PhysicsContactListener::ContactEventCallback callback) {
    if (contact_listener_) {
        contact_listener_->set_contact_removed_callback(std::move(callback));
    }
}

void PhysicsWorldManager::set_body_activated_callback(PhysicsActivationListener::ActivationEventCallback callback) {
    if (activation_listener_) {
        activation_listener_->set_body_activated_callback(std::move(callback));
    }
}

void PhysicsWorldManager::set_body_deactivated_callback(PhysicsActivationListener::ActivationEventCallback callback) {
    if (activation_listener_) {
        activation_listener_->set_body_deactivated_callback(std::move(callback));
    }
}

// 世界設定
void PhysicsWorldManager::set_gravity(const Vec3& gravity) {
    if (!initialized_) return;
    physics_system_->SetGravity(gravity);
}

Vec3 PhysicsWorldManager::get_gravity() const {
    if (!initialized_) return Vec3(0, -9.81f, 0);
    return physics_system_->GetGravity();
}

// 統計信息
PhysicsWorldManager::PhysicsStats PhysicsWorldManager::get_stats() const {
    PhysicsStats stats;
    if (!initialized_) return stats;

    stats.num_bodies = physics_system_->GetNumBodies();
    stats.num_active_bodies = physics_system_->GetNumActiveBodies(EBodyType::RigidBody);
    
    // 其他統計數據可以通過Jolt的內部API獲取
    return stats;
}

} // namespace portal_core
