#include "physics_body_component_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include "game_core_manager.h"
#include "component_registrar.h"

// Include the necessary C++ ECS components
#include "transform_component.h"
#include "physics_body_component.h"

using namespace godot;

void PhysicsBodyComponentResource::_bind_methods()
{
    // 物理體類型
    ClassDB::bind_method(D_METHOD("set_body_type", "type"), &PhysicsBodyComponentResource::set_body_type);
    ClassDB::bind_method(D_METHOD("get_body_type"), &PhysicsBodyComponentResource::get_body_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "body_type", PROPERTY_HINT_ENUM, "Static:0,Dynamic:1,Kinematic:2,Trigger:3"), 
                 "set_body_type", "get_body_type");

    // 形狀屬性
    ClassDB::bind_method(D_METHOD("set_shape_type", "type"), &PhysicsBodyComponentResource::set_shape_type);
    ClassDB::bind_method(D_METHOD("get_shape_type"), &PhysicsBodyComponentResource::get_shape_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "shape_type", PROPERTY_HINT_ENUM, "Box:0,Sphere:1,Capsule:2"), 
                 "set_shape_type", "get_shape_type");

    ClassDB::bind_method(D_METHOD("set_shape_size", "size"), &PhysicsBodyComponentResource::set_shape_size);
    ClassDB::bind_method(D_METHOD("get_shape_size"), &PhysicsBodyComponentResource::get_shape_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "shape_size"), "set_shape_size", "get_shape_size");

    // 材質屬性
    ClassDB::bind_method(D_METHOD("set_friction", "friction"), &PhysicsBodyComponentResource::set_friction);
    ClassDB::bind_method(D_METHOD("get_friction"), &PhysicsBodyComponentResource::get_friction);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "friction", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), 
                 "set_friction", "get_friction");

    ClassDB::bind_method(D_METHOD("set_restitution", "restitution"), &PhysicsBodyComponentResource::set_restitution);
    ClassDB::bind_method(D_METHOD("get_restitution"), &PhysicsBodyComponentResource::get_restitution);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "restitution", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), 
                 "set_restitution", "get_restitution");

    ClassDB::bind_method(D_METHOD("set_density", "density"), &PhysicsBodyComponentResource::set_density);
    ClassDB::bind_method(D_METHOD("get_density"), &PhysicsBodyComponentResource::get_density);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "1.0,10000.0,1.0"), 
                 "set_density", "get_density");

    // 質量屬性
    ClassDB::bind_method(D_METHOD("set_mass", "mass"), &PhysicsBodyComponentResource::set_mass);
    ClassDB::bind_method(D_METHOD("get_mass"), &PhysicsBodyComponentResource::get_mass);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mass", PROPERTY_HINT_RANGE, "0.1,1000.0,0.1"), 
                 "set_mass", "get_mass");

    ClassDB::bind_method(D_METHOD("set_center_of_mass", "center"), &PhysicsBodyComponentResource::set_center_of_mass);
    ClassDB::bind_method(D_METHOD("get_center_of_mass"), &PhysicsBodyComponentResource::get_center_of_mass);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "center_of_mass"), "set_center_of_mass", "get_center_of_mass");

    // 運動屬性
    ClassDB::bind_method(D_METHOD("set_linear_velocity", "velocity"), &PhysicsBodyComponentResource::set_linear_velocity);
    ClassDB::bind_method(D_METHOD("get_linear_velocity"), &PhysicsBodyComponentResource::get_linear_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "linear_velocity"), "set_linear_velocity", "get_linear_velocity");

    ClassDB::bind_method(D_METHOD("set_angular_velocity", "velocity"), &PhysicsBodyComponentResource::set_angular_velocity);
    ClassDB::bind_method(D_METHOD("get_angular_velocity"), &PhysicsBodyComponentResource::get_angular_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "angular_velocity"), "set_angular_velocity", "get_angular_velocity");

    // 阻尼屬性
    ClassDB::bind_method(D_METHOD("set_linear_damping", "damping"), &PhysicsBodyComponentResource::set_linear_damping);
    ClassDB::bind_method(D_METHOD("get_linear_damping"), &PhysicsBodyComponentResource::get_linear_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "linear_damping", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), 
                 "set_linear_damping", "get_linear_damping");

    ClassDB::bind_method(D_METHOD("set_angular_damping", "damping"), &PhysicsBodyComponentResource::set_angular_damping);
    ClassDB::bind_method(D_METHOD("get_angular_damping"), &PhysicsBodyComponentResource::get_angular_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angular_damping", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), 
                 "set_angular_damping", "get_angular_damping");

    // 重力屬性
    ClassDB::bind_method(D_METHOD("set_gravity_scale", "scale"), &PhysicsBodyComponentResource::set_gravity_scale);
    ClassDB::bind_method(D_METHOD("get_gravity_scale"), &PhysicsBodyComponentResource::get_gravity_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.1"), 
                 "set_gravity_scale", "get_gravity_scale");

    // 物理狀態
    ClassDB::bind_method(D_METHOD("set_is_active", "active"), &PhysicsBodyComponentResource::set_is_active);
    ClassDB::bind_method(D_METHOD("get_is_active"), &PhysicsBodyComponentResource::get_is_active);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_active"), "set_is_active", "get_is_active");

    ClassDB::bind_method(D_METHOD("set_allow_sleeping", "allow"), &PhysicsBodyComponentResource::set_allow_sleeping);
    ClassDB::bind_method(D_METHOD("get_allow_sleeping"), &PhysicsBodyComponentResource::get_allow_sleeping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_sleeping"), "set_allow_sleeping", "get_allow_sleeping");

    ClassDB::bind_method(D_METHOD("set_enable_ccd", "enable"), &PhysicsBodyComponentResource::set_enable_ccd);
    ClassDB::bind_method(D_METHOD("get_enable_ccd"), &PhysicsBodyComponentResource::get_enable_ccd);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_ccd"), "set_enable_ccd", "get_enable_ccd");

    // 運動限制
    ClassDB::bind_method(D_METHOD("set_lock_linear_x", "lock"), &PhysicsBodyComponentResource::set_lock_linear_x);
    ClassDB::bind_method(D_METHOD("get_lock_linear_x"), &PhysicsBodyComponentResource::get_lock_linear_x);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_linear_x"), "set_lock_linear_x", "get_lock_linear_x");

    ClassDB::bind_method(D_METHOD("set_lock_linear_y", "lock"), &PhysicsBodyComponentResource::set_lock_linear_y);
    ClassDB::bind_method(D_METHOD("get_lock_linear_y"), &PhysicsBodyComponentResource::get_lock_linear_y);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_linear_y"), "set_lock_linear_y", "get_lock_linear_y");

    ClassDB::bind_method(D_METHOD("set_lock_linear_z", "lock"), &PhysicsBodyComponentResource::set_lock_linear_z);
    ClassDB::bind_method(D_METHOD("get_lock_linear_z"), &PhysicsBodyComponentResource::get_lock_linear_z);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_linear_z"), "set_lock_linear_z", "get_lock_linear_z");

    ClassDB::bind_method(D_METHOD("set_lock_angular_x", "lock"), &PhysicsBodyComponentResource::set_lock_angular_x);
    ClassDB::bind_method(D_METHOD("get_lock_angular_x"), &PhysicsBodyComponentResource::get_lock_angular_x);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_angular_x"), "set_lock_angular_x", "get_lock_angular_x");

    ClassDB::bind_method(D_METHOD("set_lock_angular_y", "lock"), &PhysicsBodyComponentResource::set_lock_angular_y);
    ClassDB::bind_method(D_METHOD("get_lock_angular_y"), &PhysicsBodyComponentResource::get_lock_angular_y);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_angular_y"), "set_lock_angular_y", "get_lock_angular_y");

    ClassDB::bind_method(D_METHOD("set_lock_angular_z", "lock"), &PhysicsBodyComponentResource::set_lock_angular_z);
    ClassDB::bind_method(D_METHOD("get_lock_angular_z"), &PhysicsBodyComponentResource::get_lock_angular_z);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lock_angular_z"), "set_lock_angular_z", "get_lock_angular_z");

    // 速度限制
    ClassDB::bind_method(D_METHOD("set_max_linear_velocity", "max"), &PhysicsBodyComponentResource::set_max_linear_velocity);
    ClassDB::bind_method(D_METHOD("get_max_linear_velocity"), &PhysicsBodyComponentResource::get_max_linear_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_linear_velocity", PROPERTY_HINT_RANGE, "1.0,1000.0,1.0"), 
                 "set_max_linear_velocity", "get_max_linear_velocity");

    ClassDB::bind_method(D_METHOD("set_max_angular_velocity", "max"), &PhysicsBodyComponentResource::set_max_angular_velocity);
    ClassDB::bind_method(D_METHOD("get_max_angular_velocity"), &PhysicsBodyComponentResource::get_max_angular_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_angular_velocity", PROPERTY_HINT_RANGE, "1.0,100.0,1.0"), 
                 "set_max_angular_velocity", "get_max_angular_velocity");

    // 碰撞過濾
    ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &PhysicsBodyComponentResource::set_collision_layer);
    ClassDB::bind_method(D_METHOD("get_collision_layer"), &PhysicsBodyComponentResource::get_collision_layer);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), 
                 "set_collision_layer", "get_collision_layer");

    ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &PhysicsBodyComponentResource::set_collision_mask);
    ClassDB::bind_method(D_METHOD("get_collision_mask"), &PhysicsBodyComponentResource::get_collision_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), 
                 "set_collision_mask", "get_collision_mask");

    ClassDB::bind_method(D_METHOD("set_collision_group", "group"), &PhysicsBodyComponentResource::set_collision_group);
    ClassDB::bind_method(D_METHOD("get_collision_group"), &PhysicsBodyComponentResource::get_collision_group);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_group", PROPERTY_HINT_RANGE, "-32768,32767,1"), 
                 "set_collision_group", "get_collision_group");

    // 便利方法
    ClassDB::bind_method(D_METHOD("set_box_shape", "half_extents"), &PhysicsBodyComponentResource::set_box_shape);
    ClassDB::bind_method(D_METHOD("set_sphere_shape", "radius"), &PhysicsBodyComponentResource::set_sphere_shape);
    ClassDB::bind_method(D_METHOD("set_capsule_shape", "radius", "height"), &PhysicsBodyComponentResource::set_capsule_shape);
    ClassDB::bind_method(D_METHOD("set_dynamic_body"), &PhysicsBodyComponentResource::set_dynamic_body);
    ClassDB::bind_method(D_METHOD("set_static_body"), &PhysicsBodyComponentResource::set_static_body);
    ClassDB::bind_method(D_METHOD("set_kinematic_body"), &PhysicsBodyComponentResource::set_kinematic_body);
    ClassDB::bind_method(D_METHOD("set_trigger_body"), &PhysicsBodyComponentResource::set_trigger_body);
}

PhysicsBodyComponentResource::PhysicsBodyComponentResource()
    : body_type(1) // Dynamic 預設
    , shape_type(0) // Box 預設
    , shape_size(Vector3(1.0f, 1.0f, 1.0f)) // 1x1x1 盒子
    , friction(0.5f)
    , restitution(0.0f)
    , density(1000.0f)
    , mass(1.0f)
    , center_of_mass(Vector3(0.0f, 0.0f, 0.0f))
    , linear_velocity(Vector3(0.0f, 0.0f, 0.0f))
    , angular_velocity(Vector3(0.0f, 0.0f, 0.0f))
    , linear_damping(0.05f)
    , angular_damping(0.05f)
    , gravity_scale(1.0f)
    , is_active(true)
    , allow_sleeping(true)
    , enable_ccd(false)
    , lock_linear_x(false)
    , lock_linear_y(false)
    , lock_linear_z(false)
    , lock_angular_x(false)
    , lock_angular_y(false)
    , lock_angular_z(false)
    , max_linear_velocity(500.0f)
    , max_angular_velocity(47.1f)
    , collision_layer(1)
    , collision_mask(0xFFFFFFFF)
    , collision_group(0)
{
}

PhysicsBodyComponentResource::~PhysicsBodyComponentResource()
{
}

// 物理體類型
void PhysicsBodyComponentResource::set_body_type(int p_type) { body_type = p_type; }
int PhysicsBodyComponentResource::get_body_type() const { return body_type; }

// 形狀屬性
void PhysicsBodyComponentResource::set_shape_type(int p_type) { shape_type = p_type; }
int PhysicsBodyComponentResource::get_shape_type() const { return shape_type; }
void PhysicsBodyComponentResource::set_shape_size(const Vector3& p_size) { shape_size = p_size; }
Vector3 PhysicsBodyComponentResource::get_shape_size() const { return shape_size; }

// 材質屬性
void PhysicsBodyComponentResource::set_friction(float p_friction) { friction = p_friction; }
float PhysicsBodyComponentResource::get_friction() const { return friction; }
void PhysicsBodyComponentResource::set_restitution(float p_restitution) { restitution = p_restitution; }
float PhysicsBodyComponentResource::get_restitution() const { return restitution; }
void PhysicsBodyComponentResource::set_density(float p_density) { density = p_density; }
float PhysicsBodyComponentResource::get_density() const { return density; }

// 質量屬性
void PhysicsBodyComponentResource::set_mass(float p_mass) { mass = p_mass; }
float PhysicsBodyComponentResource::get_mass() const { return mass; }
void PhysicsBodyComponentResource::set_center_of_mass(const Vector3& p_center) { center_of_mass = p_center; }
Vector3 PhysicsBodyComponentResource::get_center_of_mass() const { return center_of_mass; }

// 運動屬性
void PhysicsBodyComponentResource::set_linear_velocity(const Vector3& p_velocity) { linear_velocity = p_velocity; }
Vector3 PhysicsBodyComponentResource::get_linear_velocity() const { return linear_velocity; }
void PhysicsBodyComponentResource::set_angular_velocity(const Vector3& p_velocity) { angular_velocity = p_velocity; }
Vector3 PhysicsBodyComponentResource::get_angular_velocity() const { return angular_velocity; }

// 阻尼屬性
void PhysicsBodyComponentResource::set_linear_damping(float p_damping) { linear_damping = p_damping; }
float PhysicsBodyComponentResource::get_linear_damping() const { return linear_damping; }
void PhysicsBodyComponentResource::set_angular_damping(float p_damping) { angular_damping = p_damping; }
float PhysicsBodyComponentResource::get_angular_damping() const { return angular_damping; }

// 重力屬性
void PhysicsBodyComponentResource::set_gravity_scale(float p_scale) { gravity_scale = p_scale; }
float PhysicsBodyComponentResource::get_gravity_scale() const { return gravity_scale; }

// 物理狀態
void PhysicsBodyComponentResource::set_is_active(bool p_active) { is_active = p_active; }
bool PhysicsBodyComponentResource::get_is_active() const { return is_active; }
void PhysicsBodyComponentResource::set_allow_sleeping(bool p_allow) { allow_sleeping = p_allow; }
bool PhysicsBodyComponentResource::get_allow_sleeping() const { return allow_sleeping; }
void PhysicsBodyComponentResource::set_enable_ccd(bool p_enable) { enable_ccd = p_enable; }
bool PhysicsBodyComponentResource::get_enable_ccd() const { return enable_ccd; }

// 運動限制
void PhysicsBodyComponentResource::set_lock_linear_x(bool p_lock) { lock_linear_x = p_lock; }
bool PhysicsBodyComponentResource::get_lock_linear_x() const { return lock_linear_x; }
void PhysicsBodyComponentResource::set_lock_linear_y(bool p_lock) { lock_linear_y = p_lock; }
bool PhysicsBodyComponentResource::get_lock_linear_y() const { return lock_linear_y; }
void PhysicsBodyComponentResource::set_lock_linear_z(bool p_lock) { lock_linear_z = p_lock; }
bool PhysicsBodyComponentResource::get_lock_linear_z() const { return lock_linear_z; }
void PhysicsBodyComponentResource::set_lock_angular_x(bool p_lock) { lock_angular_x = p_lock; }
bool PhysicsBodyComponentResource::get_lock_angular_x() const { return lock_angular_x; }
void PhysicsBodyComponentResource::set_lock_angular_y(bool p_lock) { lock_angular_y = p_lock; }
bool PhysicsBodyComponentResource::get_lock_angular_y() const { return lock_angular_y; }
void PhysicsBodyComponentResource::set_lock_angular_z(bool p_lock) { lock_angular_z = p_lock; }
bool PhysicsBodyComponentResource::get_lock_angular_z() const { return lock_angular_z; }

// 速度限制
void PhysicsBodyComponentResource::set_max_linear_velocity(float p_max) { max_linear_velocity = p_max; }
float PhysicsBodyComponentResource::get_max_linear_velocity() const { return max_linear_velocity; }
void PhysicsBodyComponentResource::set_max_angular_velocity(float p_max) { max_angular_velocity = p_max; }
float PhysicsBodyComponentResource::get_max_angular_velocity() const { return max_angular_velocity; }

// 碰撞過濾
void PhysicsBodyComponentResource::set_collision_layer(int p_layer) { collision_layer = p_layer; }
int PhysicsBodyComponentResource::get_collision_layer() const { return collision_layer; }
void PhysicsBodyComponentResource::set_collision_mask(int p_mask) { collision_mask = p_mask; }
int PhysicsBodyComponentResource::get_collision_mask() const { return collision_mask; }
void PhysicsBodyComponentResource::set_collision_group(int p_group) { collision_group = p_group; }
int PhysicsBodyComponentResource::get_collision_group() const { return collision_group; }

// 實現基類純虛函數
bool PhysicsBodyComponentResource::apply_to_entity(entt::registry& registry, entt::entity entity)
{
    // 創建C++ ECS組件
    portal_core::PhysicsBodyComponent cpp_component(get_cpp_body_type(), create_cpp_shape());
    
    // 應用所有屬性
    apply_properties_to_cpp_component(cpp_component);
    
    // 添加或替換組件到實體
    registry.emplace_or_replace<portal_core::PhysicsBodyComponent>(entity, std::move(cpp_component));
    
    UtilityFunctions::print("PhysicsBodyComponent applied to entity: ", (int)entity);
    return true;
}

bool PhysicsBodyComponentResource::remove_from_entity(entt::registry& registry, entt::entity entity)
{
    if (registry.any_of<portal_core::PhysicsBodyComponent>(entity)) {
        registry.remove<portal_core::PhysicsBodyComponent>(entity);
        UtilityFunctions::print("PhysicsBodyComponent removed from entity: ", (int)entity);
        return true;
    }
    return false;
}

bool PhysicsBodyComponentResource::has_component(const entt::registry& registry, entt::entity entity) const
{
    return registry.any_of<portal_core::PhysicsBodyComponent>(entity);
}

String PhysicsBodyComponentResource::get_component_type_name() const
{
    return "PhysicsBodyComponent";
}

void PhysicsBodyComponentResource::sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node)
{
    // 只对动态物体进行位置同步
    if (body_type != 1) { // 1 = Dynamic
        return;
    }
    
    // 检查目标节点是否是 Node3D
    Node3D* node3d = Object::cast_to<Node3D>(target_node);
    if (!node3d) {
        return;
    }
    
    // 获取物理体组件
    auto* physics_body = registry.try_get<portal_core::PhysicsBodyComponent>(entity);
    if (!physics_body || !physics_body->is_valid()) {
        return;
    }
    
    // 获取Transform组件（物理系统会更新这个组件）
    auto* transform_comp = registry.try_get<portal_core::TransformComponent>(entity);
    if (!transform_comp) {
        return;
    }
    
    // 将 C++ Transform 转换为 Godot Transform
    Vector3 godot_position(transform_comp->position.x(), transform_comp->position.y(), transform_comp->position.z());
    Quaternion godot_rotation(transform_comp->rotation.x(), transform_comp->rotation.y(), transform_comp->rotation.z(), transform_comp->rotation.w());
    Vector3 godot_scale(transform_comp->scale.x(), transform_comp->scale.y(), transform_comp->scale.z());
    
    // 更新 Godot 节点的变换
    node3d->set_position(godot_position);
    node3d->set_quaternion(godot_rotation);
    node3d->set_scale(godot_scale);
}

// 便利方法
void PhysicsBodyComponentResource::set_box_shape(const Vector3& half_extents)
{
    shape_type = 0; // Box
    shape_size = half_extents;
}

void PhysicsBodyComponentResource::set_sphere_shape(float radius)
{
    shape_type = 1; // Sphere
    shape_size = Vector3(radius, radius, radius);
}

void PhysicsBodyComponentResource::set_capsule_shape(float radius, float height)
{
    shape_type = 2; // Capsule
    shape_size = Vector3(radius, height, radius);
}

void PhysicsBodyComponentResource::set_dynamic_body()
{
    body_type = 1; // Dynamic
}

void PhysicsBodyComponentResource::set_static_body()
{
    body_type = 0; // Static
}

void PhysicsBodyComponentResource::set_kinematic_body()
{
    body_type = 2; // Kinematic
}

void PhysicsBodyComponentResource::set_trigger_body()
{
    body_type = 3; // Trigger
}

// 私有輔助方法
portal_core::PhysicsBodyType PhysicsBodyComponentResource::get_cpp_body_type() const
{
    switch (body_type) {
        case 0: return portal_core::PhysicsBodyType::STATIC;
        case 1: return portal_core::PhysicsBodyType::DYNAMIC;
        case 2: return portal_core::PhysicsBodyType::KINEMATIC;
        case 3: return portal_core::PhysicsBodyType::TRIGGER;
        default: return portal_core::PhysicsBodyType::DYNAMIC;
    }
}

portal_core::PhysicsShapeDesc PhysicsBodyComponentResource::create_cpp_shape() const
{
    switch (shape_type) {
        case 0: // Box
            return portal_core::PhysicsShapeDesc::box(
                portal_core::Vec3(shape_size.x, shape_size.y, shape_size.z)
            );
        case 1: // Sphere
            return portal_core::PhysicsShapeDesc::sphere(shape_size.x);
        case 2: // Capsule
            return portal_core::PhysicsShapeDesc::capsule(shape_size.x, shape_size.y);
        default:
            return portal_core::PhysicsShapeDesc::box(
                portal_core::Vec3(1.0f, 1.0f, 1.0f)
            );
    }
}

portal_core::PhysicsMaterial PhysicsBodyComponentResource::create_cpp_material() const
{
    portal_core::PhysicsMaterial material;
    material.friction = friction;
    material.restitution = restitution;
    material.density = density;
    return material;
}

void PhysicsBodyComponentResource::apply_properties_to_cpp_component(portal_core::PhysicsBodyComponent& cpp_component) const
{
    // 設置材質
    cpp_component.material = create_cpp_material();
    
    // 設置質量屬性
    cpp_component.mass = mass;
    cpp_component.center_of_mass = portal_core::Vec3(center_of_mass.x, center_of_mass.y, center_of_mass.z);
    
    // 設置運動屬性
    cpp_component.linear_velocity = portal_core::Vec3(linear_velocity.x, linear_velocity.y, linear_velocity.z);
    cpp_component.angular_velocity = portal_core::Vec3(angular_velocity.x, angular_velocity.y, angular_velocity.z);
    
    // 設置阻尼
    cpp_component.linear_damping = linear_damping;
    cpp_component.angular_damping = angular_damping;
    
    // 設置重力
    cpp_component.gravity_scale = gravity_scale;
    
    // 設置物理狀態
    cpp_component.is_active = is_active;
    cpp_component.allow_sleeping = allow_sleeping;
    cpp_component.enable_ccd = enable_ccd;
    
    // 設置運動限制
    cpp_component.lock_linear_x = lock_linear_x;
    cpp_component.lock_linear_y = lock_linear_y;
    cpp_component.lock_linear_z = lock_linear_z;
    cpp_component.lock_angular_x = lock_angular_x;
    cpp_component.lock_angular_y = lock_angular_y;
    cpp_component.lock_angular_z = lock_angular_z;
    
    // 設置速度限制
    cpp_component.max_linear_velocity = max_linear_velocity;
    cpp_component.max_angular_velocity = max_angular_velocity;
    
    // 設置碰撞過濾
    cpp_component.collision_filter.collision_layer = static_cast<uint32_t>(collision_layer);
    cpp_component.collision_filter.collision_mask = static_cast<uint32_t>(collision_mask);
    cpp_component.collision_filter.collision_group = static_cast<int16_t>(collision_group);
}

REGISTER_COMPONENT_RESOURCE(PhysicsBodyComponentResource)
