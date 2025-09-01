#include "portal_game_world.h"
#include <iostream>

namespace Portal {

PortalGameWorld::PortalGameWorld()
    : is_initialized_(false)
    , accumulated_time_(0.0f)
{
}

PortalGameWorld::~PortalGameWorld() {
    if (is_initialized_) {
        shutdown();
    }
}

bool PortalGameWorld::initialize() {
    if (is_initialized_) {
        return true;
    }

    std::cout << "Initializing Portal Game World..." << std::endl;

    try {
        // 初始化物理系統
        if (!initialize_physics_system()) {
            std::cerr << "Failed to initialize physics system" << std::endl;
            return false;
        }

        // 初始化渲染系統
        if (!initialize_render_system()) {
            std::cerr << "Failed to initialize render system" << std::endl;
            return false;
        }

        // 初始化傳送門系統
        if (!initialize_portal_system()) {
            std::cerr << "Failed to initialize portal system" << std::endl;
            return false;
        }

        // 創建更新系統
        physics_update_system_ = std::make_unique<PhysicsUpdateSystem>(physics_system_.get());
        render_update_system_ = std::make_unique<RenderUpdateSystem>(render_system_.get());

        is_initialized_ = true;
        std::cout << "Portal Game World initialized successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception during initialization: " << e.what() << std::endl;
        shutdown();
        return false;
    }
}

void PortalGameWorld::shutdown() {
    if (!is_initialized_) {
        return;
    }

    std::cout << "Shutting down Portal Game World..." << std::endl;

    // 清理更新系統
    physics_update_system_.reset();
    render_update_system_.reset();

    // 清理核心系統
    if (portal_manager_) {
        portal_manager_->shutdown();
        portal_manager_.reset();
    }

    if (render_system_) {
        render_system_->shutdown();
        render_system_.reset();
    }

    if (physics_system_) {
        physics_system_->shutdown();
        physics_system_.reset();
    }

    // 清理ECS
    registry_.clear();

    is_initialized_ = false;
    accumulated_time_ = 0.0f;
    std::cout << "Portal Game World shutdown complete" << std::endl;
}

void PortalGameWorld::update(float delta_time) {
    if (!is_initialized_) {
        return;
    }

    accumulated_time_ += delta_time;

    // 使用固定時間步長更新物理
    while (accumulated_time_ >= FIXED_TIMESTEP) {
        update_physics(FIXED_TIMESTEP);
        accumulated_time_ -= FIXED_TIMESTEP;
    }

    // 更新傳送門系統（使用實際時間步長）
    update_portal_system(delta_time);

    // 更新渲染系統
    update_render_system();
}

bool PortalGameWorld::initialize_physics_system() {
    physics_system_ = std::make_unique<JoltPhysicsSystem>();
    physics_system_->set_registry(&registry_);
    return physics_system_->initialize();
}

bool PortalGameWorld::initialize_render_system() {
    render_system_ = std::make_unique<RenderSystem>();
    render_system_->set_registry(&registry_);
    return render_system_->initialize();
}

bool PortalGameWorld::initialize_portal_system() {
    // 設置宿主接口
    HostInterfaces interfaces;
    interfaces.physics_query = physics_system_.get();
    interfaces.physics_manipulator = physics_system_.get();
    interfaces.render_query = render_system_.get();
    interfaces.render_manipulator = render_system_.get();
    interfaces.event_handler = nullptr; // 可選

    portal_manager_ = std::make_unique<PortalManager>(interfaces);
    return portal_manager_->initialize();
}

void PortalGameWorld::update_physics(float delta_time) {
    if (physics_system_) {
        physics_system_->update(delta_time);
    }

    if (physics_update_system_) {
        physics_update_system_->update(delta_time);
    }
}

void PortalGameWorld::update_portal_system(float delta_time) {
    if (portal_manager_) {
        portal_manager_->update(delta_time);
    }
}

void PortalGameWorld::update_render_system() {
    if (render_update_system_) {
        render_update_system_->update();
    }
}

// === 實體創建工廠方法 ===

entt::entity PortalGameWorld::create_physics_entity(
    const Vector3& position,
    const Quaternion& rotation,
    CollisionShapeType shape_type,
    const Vector3& dimensions,
    bool is_dynamic,
    float mass
) {
    if (!is_initialized_ || !physics_system_) {
        return entt::null;
    }

    // 設置組件
    TransformComponent transform;
    transform.position = position;
    transform.rotation = rotation;
    transform.scale = Vector3(1, 1, 1);

    CollisionShapeComponent collision_shape(shape_type);
    collision_shape.dimensions = dimensions;

    PhysicsBodyComponent physics_body;
    physics_body.is_dynamic = is_dynamic;
    physics_body.is_kinematic = false;
    physics_body.mass = mass;

    // 創建實體
    entt::entity entity = physics_system_->create_physics_entity(transform, collision_shape, physics_body);

    // 添加渲染組件
    registry_.emplace<RenderComponent>(entity);

    return entity;
}

entt::entity PortalGameWorld::create_portal_entity(
    const Vector3& center,
    const Vector3& normal,
    const Vector3& up,
    float width,
    float height
) {
    if (!is_initialized_ || !portal_manager_) {
        return entt::null;
    }

    // 創建傳送門平面
    PortalPlane plane;
    plane.center = center;
    plane.normal = normal.normalized();
    plane.up = up.normalized();
    plane.right = plane.normal.cross(plane.up).normalized();
    plane.width = width;
    plane.height = height;

    // 在傳送門管理器中創建傳送門
    PortalId portal_id = portal_manager_->create_portal(plane);
    if (portal_id == INVALID_PORTAL_ID) {
        return entt::null;
    }

    // 創建ECS實體表示傳送門
    entt::entity entity = registry_.create();

    // 添加變換組件
    TransformComponent transform;
    transform.position = center;
    // 計算旋轉（從normal和up向量）
    // 這裡需要實現從兩個向量到四元數的轉換
    registry_.emplace<TransformComponent>(entity, transform);

    // 添加傳送門組件
    PortalEntityComponent portal_comp;
    portal_comp.associated_portal = portal_id;
    registry_.emplace<PortalEntityComponent>(entity, portal_comp);

    // 添加渲染組件
    registry_.emplace<RenderComponent>(entity);

    return entity;
}

entt::entity PortalGameWorld::create_static_entity(
    const Vector3& position,
    const Quaternion& rotation,
    CollisionShapeType shape_type,
    const Vector3& dimensions
) {
    return create_physics_entity(position, rotation, shape_type, dimensions, false, 0.0f);
}

void PortalGameWorld::destroy_entity(entt::entity entity) {
    if (!registry_.valid(entity)) {
        return;
    }

    // 檢查是否是傳送門實體
    if (auto* portal_comp = registry_.try_get<PortalEntityComponent>(entity)) {
        if (portal_comp->associated_portal != INVALID_PORTAL_ID) {
            // 取消註冊並銷毀傳送門
            if (portal_manager_) {
                portal_manager_->unregister_entity(physics_system_->entt_entity_to_entity_id(entity));
                portal_manager_->destroy_portal(portal_comp->associated_portal);
            }
        }
    }

    // 銷毀物理實體
    if (physics_system_) {
        physics_system_->destroy_physics_entity(entity);
    }

    // ECS實體在physics_system_->destroy_physics_entity中已經被銷毀
}

// === 傳送門管理輔助方法 ===

bool PortalGameWorld::link_portal_entities(entt::entity portal1, entt::entity portal2) {
    if (!registry_.valid(portal1) || !registry_.valid(portal2) || !portal_manager_) {
        return false;
    }

    auto* portal_comp1 = registry_.try_get<PortalEntityComponent>(portal1);
    auto* portal_comp2 = registry_.try_get<PortalEntityComponent>(portal2);

    if (!portal_comp1 || !portal_comp2) {
        return false;
    }

    return portal_manager_->link_portals(portal_comp1->associated_portal, portal_comp2->associated_portal);
}

void PortalGameWorld::register_entity_for_teleportation(entt::entity entity) {
    if (!registry_.valid(entity) || !portal_manager_ || !physics_system_) {
        return;
    }

    EntityId entity_id = physics_system_->entt_entity_to_entity_id(entity);
    if (entity_id != INVALID_ENTITY_ID) {
        portal_manager_->register_entity(entity_id);
    }
}

void PortalGameWorld::unregister_entity_for_teleportation(entt::entity entity) {
    if (!registry_.valid(entity) || !portal_manager_ || !physics_system_) {
        return;
    }

    EntityId entity_id = physics_system_->entt_entity_to_entity_id(entity);
    if (entity_id != INVALID_ENTITY_ID) {
        portal_manager_->unregister_entity(entity_id);
    }
}

// === 相機管理 ===

void PortalGameWorld::set_main_camera(const CameraParams& camera) {
    if (render_system_) {
        render_system_->set_main_camera(camera);
    }
}

CameraParams PortalGameWorld::get_main_camera() const {
    if (render_system_) {
        return render_system_->get_main_camera();
    }
    return CameraParams();
}

// === 調試和統計 ===

size_t PortalGameWorld::get_entity_count() const {
    size_t count = 0;
    // 使用視圖來計算所有活躍實體
    auto all_entities = registry_.view<entt::entity>();
    for (auto entity : all_entities) {
        (void)entity; // 消除未使用的變量警告
        ++count;
    }
    return count;
}

size_t PortalGameWorld::get_portal_count() const {
    if (portal_manager_) {
        return portal_manager_->get_portal_count();
    }
    return 0;
}

size_t PortalGameWorld::get_physics_body_count() const {
    if (!physics_system_) {
        return 0;
    }

    // 計算擁有物理身體組件的實體數量
    auto view = registry_.view<PhysicsBodyComponent>();
    return view.size();
}

// === 場景管理 ===

void PortalGameWorld::clear_scene() {
    if (!is_initialized_) {
        return;
    }

    // 清理所有傳送門
    if (portal_manager_) {
        // 獲取所有傳送門實體並銷毀
        auto portal_view = registry_.view<PortalEntityComponent>();
        std::vector<entt::entity> portals_to_destroy;
        for (auto entity : portal_view) {
            portals_to_destroy.push_back(entity);
        }
        for (auto entity : portals_to_destroy) {
            destroy_entity(entity);
        }
    }

    // 清理所有物理實體
    if (physics_system_) {
        auto physics_view = registry_.view<PhysicsBodyComponent>();
        std::vector<entt::entity> entities_to_destroy;
        for (auto entity : physics_view) {
            entities_to_destroy.push_back(entity);
        }
        for (auto entity : entities_to_destroy) {
            destroy_entity(entity);
        }
    }

    // 清理剩餘的ECS實體
    registry_.clear();
}

void PortalGameWorld::reset_physics() {
    if (physics_system_) {
        // 重置所有物理身體的速度
        auto view = registry_.view<VelocityComponent>();
        for (auto entity : view) {
            auto& velocity = view.get<VelocityComponent>(entity);
            velocity.linear_velocity = Vector3(0, 0, 0);
            velocity.angular_velocity = Vector3(0, 0, 0);
        }
    }
}

// === PortalGameWorldBuilder 實現 ===

PortalGameWorldBuilder::PortalGameWorldBuilder()
    : gravity_(0, -9.81f, 0)
    , max_bodies_(10240)
    , max_recursion_depth_(3)
{
    // 設置默認主相機
    main_camera_.position = Vector3(0, 0, 5);
    main_camera_.rotation = Quaternion(0, 0, 0, 1);
    main_camera_.fov = 75.0f;
    main_camera_.near_plane = 0.1f;
    main_camera_.far_plane = 1000.0f;
    main_camera_.aspect_ratio = 16.0f / 9.0f;
}

PortalGameWorldBuilder& PortalGameWorldBuilder::with_gravity(const Vector3& gravity) {
    gravity_ = gravity;
    return *this;
}

PortalGameWorldBuilder& PortalGameWorldBuilder::with_max_bodies(unsigned int max_bodies) {
    max_bodies_ = max_bodies;
    return *this;
}

PortalGameWorldBuilder& PortalGameWorldBuilder::with_main_camera(const CameraParams& camera) {
    main_camera_ = camera;
    return *this;
}

PortalGameWorldBuilder& PortalGameWorldBuilder::with_max_recursion_depth(int depth) {
    max_recursion_depth_ = depth;
    return *this;
}

std::unique_ptr<PortalGameWorld> PortalGameWorldBuilder::build() {
    auto world = std::make_unique<PortalGameWorld>();

    if (!world->initialize()) {
        return nullptr;
    }

    // 設置配置參數
    world->set_main_camera(main_camera_);

    return world;
}

} // namespace Portal
