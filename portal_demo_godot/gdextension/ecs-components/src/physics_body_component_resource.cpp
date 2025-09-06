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
    
    // 约束验证方法
    ClassDB::bind_method(D_METHOD("validate_constraints"), &PhysicsBodyComponentResource::validate_constraints);
    ClassDB::bind_method(D_METHOD("get_constraint_warnings"), &PhysicsBodyComponentResource::get_constraint_warnings);
    
    // 自动填充方法 (这些方法已在基类 IPresettableResource 中绑定，这里无需重复绑定)
    // 但我们可以添加一些物理组件特有的便利方法
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
void PhysicsBodyComponentResource::set_shape_size(const Vector3& p_size) { 
    shape_size = p_size; 
    _on_property_changed();
}
Vector3 PhysicsBodyComponentResource::get_shape_size() const { return shape_size; }

// 材質屬性
void PhysicsBodyComponentResource::set_friction(float p_friction) { 
    friction = p_friction; 
    _on_property_changed();
}
float PhysicsBodyComponentResource::get_friction() const { return friction; }
void PhysicsBodyComponentResource::set_restitution(float p_restitution) { 
    restitution = p_restitution; 
    _on_property_changed();
}
float PhysicsBodyComponentResource::get_restitution() const { return restitution; }
void PhysicsBodyComponentResource::set_density(float p_density) { 
    density = p_density; 
    _on_property_changed();
}
float PhysicsBodyComponentResource::get_density() const { return density; }

// 質量屬性
void PhysicsBodyComponentResource::set_mass(float p_mass) { 
    mass = p_mass; 
    _on_property_changed();
}
float PhysicsBodyComponentResource::get_mass() const { return mass; }
void PhysicsBodyComponentResource::set_center_of_mass(const Vector3& p_center) { center_of_mass = p_center; }
Vector3 PhysicsBodyComponentResource::get_center_of_mass() const { return center_of_mass; }

// 運動屬性
void PhysicsBodyComponentResource::set_linear_velocity(const Vector3& p_velocity) { linear_velocity = p_velocity; }
Vector3 PhysicsBodyComponentResource::get_linear_velocity() const { return linear_velocity; }
void PhysicsBodyComponentResource::set_angular_velocity(const Vector3& p_velocity) { angular_velocity = p_velocity; }
Vector3 PhysicsBodyComponentResource::get_angular_velocity() const { return angular_velocity; }

// 阻尼屬性
void PhysicsBodyComponentResource::set_linear_damping(float p_damping) { 
    linear_damping = p_damping; 
    _on_property_changed();
}
float PhysicsBodyComponentResource::get_linear_damping() const { return linear_damping; }
void PhysicsBodyComponentResource::set_angular_damping(float p_damping) { 
    angular_damping = p_damping; 
    _on_property_changed();
}
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

// 约束验证方法实现
Array PhysicsBodyComponentResource::validate_constraints() const
{
    Array warnings;
    
    // 检查物理体类型和质量
    if (body_type == 1 && mass <= 0.0f) { // Dynamic body
        warnings.append("Dynamic body mass must be greater than 0. Current: " + String::num(mass));
    }
    
    // 检查形状尺寸
    switch (shape_type) {
        case 0: // Box
            if (shape_size.x <= 0.0f || shape_size.y <= 0.0f || shape_size.z <= 0.0f) {
                warnings.append("Box size must be positive in all dimensions. Current: " + 
                    String::num(shape_size.x) + ", " + String::num(shape_size.y) + ", " + String::num(shape_size.z));
            }
            break;
        case 1: // Sphere  
            if (shape_size.x <= 0.0f) {
                warnings.append("Sphere radius must be positive. Current: " + String::num(shape_size.x));
            }
            break;
        case 2: // Capsule
            if (shape_size.x <= 0.0f) {
                warnings.append("Capsule radius must be positive. Current: " + String::num(shape_size.x));
            }
            if (shape_size.y <= 0.0f) {
                warnings.append("Capsule height must be positive. Current: " + String::num(shape_size.y));
            }
            break;
    }
    
    // 检查材质属性
    if (friction < 0.0f) {
        warnings.append("Friction must be non-negative. Current: " + String::num(friction));
    }
    
    if (restitution < 0.0f || restitution > 1.0f) {
        warnings.append("Restitution must be between 0.0 and 1.0. Current: " + String::num(restitution));
    }
    
    if ((body_type == 1 || body_type == 2) && density <= 0.0f) { // Dynamic or Kinematic
        warnings.append("Dynamic/Kinematic body density must be positive. Current: " + String::num(density));
    }
    
    // 检查阻尼值
    if (linear_damping < 0.0f || linear_damping > 1.0f) {
        warnings.append("Linear damping must be between 0.0 and 1.0. Current: " + String::num(linear_damping));
    }
    
    if (angular_damping < 0.0f || angular_damping > 1.0f) {
        warnings.append("Angular damping must be between 0.0 and 1.0. Current: " + String::num(angular_damping));
    }
    
    // 检查速度限制
    if (max_linear_velocity <= 0.0f) {
        warnings.append("Max linear velocity must be positive. Current: " + String::num(max_linear_velocity));
    }
    
    if (max_angular_velocity <= 0.0f) {
        warnings.append("Max angular velocity must be positive. Current: " + String::num(max_angular_velocity));
    }
    
    // 检查重力缩放
    if (gravity_scale < 0.0f) {
        warnings.append("Gravity scale cannot be negative. Current: " + String::num(gravity_scale));
    }
    
    return warnings;
}

String PhysicsBodyComponentResource::get_constraint_warnings() const
{
    Array warnings = validate_constraints();
    if (warnings.is_empty()) {
        return "";
    }
    
    String result = "⚠️ Constraint Warnings:\n";
    for (int i = 0; i < warnings.size(); i++) {
        result += "• " + warnings[i].operator String() + "\n";
    }
    
    return result;
}

// 屬性變化處理方法
void PhysicsBodyComponentResource::_on_property_changed()
{
    // 執行約束檢查
    get_constraint_warnings(); // 這會觸發約束檢查
    
    // 觸發changed信號，讓ECS系統和UI知道屬性已更改
    emit_changed();
}

// ===== 自动填充功能实现 =====

Array PhysicsBodyComponentResource::get_auto_fill_capabilities() const
{
    Array capabilities;
    
    // 从 MeshInstance3D 提取形状
    Dictionary mesh_capability;
    mesh_capability["source_node_type"] = "MeshInstance3D";
    mesh_capability["capability_name"] = "Mesh Shape";
    mesh_capability["description"] = "Extract box shape from mesh bounds (AABB)";
    mesh_capability["supported_properties"] = Array::make("shape_type", "shape_size");
    capabilities.append(mesh_capability);
    
    // 从 CollisionShape3D 提取碰撞数据
    Dictionary collision_capability;
    collision_capability["source_node_type"] = "CollisionShape3D";
    collision_capability["capability_name"] = "Collision Shape";
    collision_capability["description"] = "Copy shape type and size from collision shape";
    collision_capability["supported_properties"] = Array::make("shape_type", "shape_size");
    capabilities.append(collision_capability);
    
    // 从 RigidBody3D 提取物理属性
    Dictionary rigid_body_capability;
    rigid_body_capability["source_node_type"] = "RigidBody3D";
    rigid_body_capability["capability_name"] = "Physics Properties";
    rigid_body_capability["description"] = "Extract mass, damping, and physics settings";
    rigid_body_capability["supported_properties"] = Array::make("body_type", "mass", "linear_damping", "angular_damping", "gravity_scale");
    capabilities.append(rigid_body_capability);
    
    // 从 StaticBody3D 设置静态体
    Dictionary static_body_capability;
    static_body_capability["source_node_type"] = "StaticBody3D";
    static_body_capability["capability_name"] = "Static Body";
    static_body_capability["description"] = "Configure as static physics body";
    static_body_capability["supported_properties"] = Array::make("body_type");
    capabilities.append(static_body_capability);
    
    return capabilities;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_node(Node* target_node, const String& capability_name)
{
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "";
    
    if (!target_node) {
        result["error_message"] = "Target node is null";
        return result;
    }
    
    String node_class = target_node->get_class();
    
    // 根据节点类型和能力名称选择合适的填充方法
    if (capability_name.is_empty()) {
        // 自动选择最佳匹配
        if (node_class == "MeshInstance3D" || target_node->is_class("MeshInstance3D")) {
            return auto_fill_from_mesh_instance(target_node);
        } else if (node_class == "CollisionShape3D" || target_node->is_class("CollisionShape3D")) {
            return auto_fill_from_collision_shape(target_node);
        } else if (node_class == "RigidBody3D" || target_node->is_class("RigidBody3D")) {
            return auto_fill_from_rigid_body(target_node);
        } else if (node_class == "StaticBody3D" || target_node->is_class("StaticBody3D")) {
            return auto_fill_from_static_body(target_node);
        }
    } else {
        // 使用指定的能力
        if (capability_name == "Mesh Shape" && (node_class == "MeshInstance3D" || target_node->is_class("MeshInstance3D"))) {
            return auto_fill_from_mesh_instance(target_node);
        } else if (capability_name == "Collision Shape" && (node_class == "CollisionShape3D" || target_node->is_class("CollisionShape3D"))) {
            return auto_fill_from_collision_shape(target_node);
        } else if (capability_name == "Physics Properties" && (node_class == "RigidBody3D" || target_node->is_class("RigidBody3D"))) {
            return auto_fill_from_rigid_body(target_node);
        } else if (capability_name == "Static Body" && (node_class == "StaticBody3D" || target_node->is_class("StaticBody3D"))) {
            return auto_fill_from_static_body(target_node);
        }
    }
    
    result["error_message"] = "No suitable auto-fill capability found for node type: " + node_class;
    return result;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_mesh_instance(Node* node)
{
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "Mesh Shape";
    
    if (!node || !node->is_class("MeshInstance3D")) {
        result["error_message"] = "Node is not a MeshInstance3D";
        return result;
    }
    
    // 获取网格资源
    if (!node->has_method("get_mesh")) {
        result["error_message"] = "MeshInstance3D has no get_mesh method";
        return result;
    }
    
    Variant mesh_var = node->call("get_mesh");
    if (mesh_var.get_type() != Variant::OBJECT) {
        result["error_message"] = "No mesh resource found";
        return result;
    }
    
    Object* mesh_obj = mesh_var;
    if (!mesh_obj) {
        result["error_message"] = "Mesh object is null";
        return result;
    }
    
    String mesh_class = mesh_obj->get_class();
    Dictionary values;
    int shape_type = 0; // 默认为Box
    Vector3 shape_size;
    
    // 根据网格类型设置形状
    if (mesh_class == "SphereMesh") {
        shape_type = 1; // Sphere
        if (mesh_obj->has_method("get_radius")) {
            Variant radius_var = mesh_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                float radius = radius_var;
                shape_size = Vector3(radius, radius, radius); // 球体使用半径
            } else {
                shape_size = Vector3(0.5f, 0.5f, 0.5f); // 默认半径
            }
        } else {
            shape_size = Vector3(0.5f, 0.5f, 0.5f); // 默认半径
        }
    } else if (mesh_class == "CapsuleMesh") {
        shape_type = 2; // Capsule
        float radius = 0.5f;
        float height = 1.0f;
        if (mesh_obj->has_method("get_radius")) {
            Variant radius_var = mesh_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                radius = radius_var;
            }
        }
        if (mesh_obj->has_method("get_height")) {
            Variant height_var = mesh_obj->call("get_height");
            if (height_var.get_type() == Variant::FLOAT) {
                height = height_var;
            }
        }
        shape_size = Vector3(radius, height, radius); // 胶囊：半径(x), 高度(y), 半径(z)
    } else {
        // 其他网格类型（包括BoxMesh、CylinderMesh等）当作Box处理
        shape_type = 0; // Box
        Vector3 bounds = calculate_mesh_bounds(node);
        if (bounds.length() <= 0.0f) {
            result["error_message"] = "Unable to calculate mesh bounds";
            return result;
        }
        shape_size = bounds; // 保存完整尺寸，让物理引擎转换为半尺寸
    }
    
    values["shape_type"] = shape_type;
    values["shape_size"] = shape_size;
    
    // 应用属性
    set_shape_type(shape_type);
    set_shape_size(shape_size);
    
    result["success"] = true;
    result["property_values"] = values;
    return result;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_collision_shape(Node* node)
{
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "Collision Shape";
    
    int shape_type_out;
    Vector3 size_out;
    
    if (!extract_collision_shape_data(node, shape_type_out, size_out)) {
        result["error_message"] = "Unable to extract collision shape data";
        return result;
    }
    
    Dictionary values;
    values["shape_type"] = shape_type_out;
    values["shape_size"] = size_out;
    
    // 应用属性
    set_shape_type(shape_type_out);
    set_shape_size(size_out);
    
    result["success"] = true;
    result["property_values"] = values;
    return result;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_rigid_body(Node* node)
{
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "Physics Properties";
    
    // 使用反射获取 RigidBody3D 的属性
    Dictionary values;
    values["body_type"] = 1; // Dynamic
    
    // 尝试获取常见的物理属性
    if (node->has_method("get_mass")) {
        Variant mass_var = node->call("get_mass");
        if (mass_var.get_type() == Variant::FLOAT) {
            values["mass"] = mass_var;
            set_mass(mass_var);
        }
    }
    
    if (node->has_method("get_linear_damp")) {
        Variant damp_var = node->call("get_linear_damp");
        if (damp_var.get_type() == Variant::FLOAT) {
            values["linear_damping"] = damp_var;
            set_linear_damping(damp_var);
        }
    }
    
    if (node->has_method("get_angular_damp")) {
        Variant damp_var = node->call("get_angular_damp");
        if (damp_var.get_type() == Variant::FLOAT) {
            values["angular_damping"] = damp_var;
            set_angular_damping(damp_var);
        }
    }
    
    if (node->has_method("get_gravity_scale")) {
        Variant gravity_var = node->call("get_gravity_scale");
        if (gravity_var.get_type() == Variant::FLOAT) {
            values["gravity_scale"] = gravity_var;
            set_gravity_scale(gravity_var);
        }
    }
    
    // 应用动态体类型
    set_body_type(1);
    
    result["success"] = true;
    result["property_values"] = values;
    return result;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_static_body(Node* node)
{
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "Static Body";
    
    Dictionary values;
    values["body_type"] = 0; // Static
    
    // 应用静态体类型
    set_body_type(0);
    
    result["success"] = true;
    result["property_values"] = values;
    return result;
}

Vector3 PhysicsBodyComponentResource::calculate_mesh_bounds(Node* mesh_instance)
{
    if (!mesh_instance || !mesh_instance->is_class("MeshInstance3D")) {
        return Vector3();
    }
    
    // 尝试获取网格资源
    if (!mesh_instance->has_method("get_mesh")) {
        return Vector3();
    }
    
    Variant mesh_var = mesh_instance->call("get_mesh");
    if (mesh_var.get_type() != Variant::OBJECT) {
        return Vector3();
    }
    
    Object* mesh_obj = mesh_var;
    if (!mesh_obj) {
        return Vector3();
    }
    
    String mesh_class = mesh_obj->get_class();
    
    // 根据网格类型返回不同的尺寸
    if (mesh_class == "SphereMesh") {
        // 球体网格 - 获取半径
        if (mesh_obj->has_method("get_radius")) {
            Variant radius_var = mesh_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                float radius = radius_var;
                return Vector3(radius * 2.0f, radius * 2.0f, radius * 2.0f); // 返回直径作为尺寸
            }
        }
        return Vector3(1.0f, 1.0f, 1.0f); // 默认球体尺寸
    } else if (mesh_class == "CapsuleMesh") {
        // 胶囊网格 - 获取半径和高度
        float radius = 0.5f;
        float height = 1.0f;
        if (mesh_obj->has_method("get_radius")) {
            Variant radius_var = mesh_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                radius = radius_var;
            }
        }
        if (mesh_obj->has_method("get_height")) {
            Variant height_var = mesh_obj->call("get_height");
            if (height_var.get_type() == Variant::FLOAT) {
                height = height_var;
            }
        }
        return Vector3(radius * 2.0f, height, radius * 2.0f); // 返回胶囊的包围盒尺寸
    } else if (mesh_class == "CylinderMesh") {
        // 圆柱网格 - 获取半径和高度
        float top_radius = 0.5f;
        float bottom_radius = 0.5f;
        float height = 1.0f;
        if (mesh_obj->has_method("get_top_radius")) {
            Variant radius_var = mesh_obj->call("get_top_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                top_radius = radius_var;
            }
        }
        if (mesh_obj->has_method("get_bottom_radius")) {
            Variant radius_var = mesh_obj->call("get_bottom_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                bottom_radius = radius_var;
            }
        }
        if (mesh_obj->has_method("get_height")) {
            Variant height_var = mesh_obj->call("get_height");
            if (height_var.get_type() == Variant::FLOAT) {
                height = height_var;
            }
        }
        float max_radius = top_radius > bottom_radius ? top_radius : bottom_radius;
        return Vector3(max_radius * 2.0f, height, max_radius * 2.0f); // 返回圆柱的包围盒尺寸
    } else {
        // 其他网格类型（包括BoxMesh）- 使用AABB
        if (!mesh_obj->has_method("get_aabb")) {
            return Vector3();
        }
        
        // 获取AABB
        Variant aabb_var = mesh_obj->call("get_aabb");
        if (aabb_var.get_type() != Variant::AABB) {
            return Vector3();
        }
        
        AABB aabb = aabb_var;
        return aabb.size;
    }
}

bool PhysicsBodyComponentResource::extract_collision_shape_data(Node* collision_shape, int& shape_type_out, Vector3& size_out)
{
    if (!collision_shape || !collision_shape->is_class("CollisionShape3D")) {
        return false;
    }
    
    if (!collision_shape->has_method("get_shape")) {
        return false;
    }
    
    Variant shape_var = collision_shape->call("get_shape");
    if (shape_var.get_type() != Variant::OBJECT) {
        return false;
    }
    
    Object* shape_obj = shape_var;
    if (!shape_obj) {
        return false;
    }
    
    String shape_class = shape_obj->get_class();
    
    if (shape_class == "BoxShape3D") {
        shape_type_out = 0; // Box
        if (shape_obj->has_method("get_size")) {
            Variant size_var = shape_obj->call("get_size");
            if (size_var.get_type() == Variant::VECTOR3) {
                size_out = Vector3(size_var) * 0.5f; // 转换为半尺寸
                return true;
            }
        }
    } else if (shape_class == "SphereShape3D") {
        shape_type_out = 1; // Sphere
        if (shape_obj->has_method("get_radius")) {
            Variant radius_var = shape_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                float radius = radius_var;
                size_out = Vector3(radius, radius, radius);
                return true;
            }
        }
    } else if (shape_class == "CapsuleShape3D") {
        shape_type_out = 2; // Capsule
        float radius = 0.5f;
        float height = 2.0f;
        
        if (shape_obj->has_method("get_radius")) {
            Variant radius_var = shape_obj->call("get_radius");
            if (radius_var.get_type() == Variant::FLOAT) {
                radius = radius_var;
            }
        }
        
        if (shape_obj->has_method("get_height")) {
            Variant height_var = shape_obj->call("get_height");
            if (height_var.get_type() == Variant::FLOAT) {
                height = height_var;
            }
        }
        
        size_out = Vector3(radius, height, radius);
        return true;
    }
    
    return false;
}

REGISTER_COMPONENT_RESOURCE(PhysicsBodyComponentResource)
