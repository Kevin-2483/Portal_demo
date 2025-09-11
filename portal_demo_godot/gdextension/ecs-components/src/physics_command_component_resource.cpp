#include "physics_command_component_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "game_core_manager.h"
#include "component_registrar.h"
#include "physics_command_component_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "game_core_manager.h"

using namespace godot;

void PhysicsCommandComponentResource::_bind_methods()
{
    // 力操作屬性
    ClassDB::bind_method(D_METHOD("set_add_force", "force"), &PhysicsCommandComponentResource::set_add_force);
    ClassDB::bind_method(D_METHOD("get_add_force"), &PhysicsCommandComponentResource::get_add_force);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "add_force"), "set_add_force", "get_add_force");

    ClassDB::bind_method(D_METHOD("set_add_impulse", "impulse"), &PhysicsCommandComponentResource::set_add_impulse);
    ClassDB::bind_method(D_METHOD("get_add_impulse"), &PhysicsCommandComponentResource::get_add_impulse);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "add_impulse"), "set_add_impulse", "get_add_impulse");

    ClassDB::bind_method(D_METHOD("set_add_torque", "torque"), &PhysicsCommandComponentResource::set_add_torque);
    ClassDB::bind_method(D_METHOD("get_add_torque"), &PhysicsCommandComponentResource::get_add_torque);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "add_torque"), "set_add_torque", "get_add_torque");

    ClassDB::bind_method(D_METHOD("set_add_angular_impulse", "impulse"), &PhysicsCommandComponentResource::set_add_angular_impulse);
    ClassDB::bind_method(D_METHOD("get_add_angular_impulse"), &PhysicsCommandComponentResource::get_add_angular_impulse);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "add_angular_impulse"), "set_add_angular_impulse", "get_add_angular_impulse");

    // 速度設置屬性
    ClassDB::bind_method(D_METHOD("set_set_linear_velocity", "velocity"), &PhysicsCommandComponentResource::set_set_linear_velocity);
    ClassDB::bind_method(D_METHOD("get_set_linear_velocity"), &PhysicsCommandComponentResource::get_set_linear_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "set_linear_velocity"), "set_set_linear_velocity", "get_set_linear_velocity");

    ClassDB::bind_method(D_METHOD("set_use_set_linear_velocity", "use"), &PhysicsCommandComponentResource::set_use_set_linear_velocity);
    ClassDB::bind_method(D_METHOD("get_use_set_linear_velocity"), &PhysicsCommandComponentResource::get_use_set_linear_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_set_linear_velocity"), "set_use_set_linear_velocity", "get_use_set_linear_velocity");

    ClassDB::bind_method(D_METHOD("set_set_angular_velocity", "velocity"), &PhysicsCommandComponentResource::set_set_angular_velocity);
    ClassDB::bind_method(D_METHOD("get_set_angular_velocity"), &PhysicsCommandComponentResource::get_set_angular_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "set_angular_velocity"), "set_set_angular_velocity", "get_set_angular_velocity");

    ClassDB::bind_method(D_METHOD("set_use_set_angular_velocity", "use"), &PhysicsCommandComponentResource::set_use_set_angular_velocity);
    ClassDB::bind_method(D_METHOD("get_use_set_angular_velocity"), &PhysicsCommandComponentResource::get_use_set_angular_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_set_angular_velocity"), "set_use_set_angular_velocity", "get_use_set_angular_velocity");

    // 位置設置屬性
    ClassDB::bind_method(D_METHOD("set_set_position", "position"), &PhysicsCommandComponentResource::set_set_position);
    ClassDB::bind_method(D_METHOD("get_set_position"), &PhysicsCommandComponentResource::get_set_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "set_position"), "set_set_position", "get_set_position");

    ClassDB::bind_method(D_METHOD("set_use_set_position", "use"), &PhysicsCommandComponentResource::set_use_set_position);
    ClassDB::bind_method(D_METHOD("get_use_set_position"), &PhysicsCommandComponentResource::get_use_set_position);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_set_position"), "set_use_set_position", "get_use_set_position");

    ClassDB::bind_method(D_METHOD("set_set_rotation", "rotation"), &PhysicsCommandComponentResource::set_set_rotation);
    ClassDB::bind_method(D_METHOD("get_set_rotation"), &PhysicsCommandComponentResource::get_set_rotation);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "set_rotation"), "set_set_rotation", "get_set_rotation");

    ClassDB::bind_method(D_METHOD("set_use_set_rotation", "use"), &PhysicsCommandComponentResource::set_use_set_rotation);
    ClassDB::bind_method(D_METHOD("get_use_set_rotation"), &PhysicsCommandComponentResource::get_use_set_rotation);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_set_rotation"), "set_use_set_rotation", "get_use_set_rotation");

    // 物理屬性控制
    ClassDB::bind_method(D_METHOD("set_gravity_scale", "scale"), &PhysicsCommandComponentResource::set_gravity_scale);
    ClassDB::bind_method(D_METHOD("get_gravity_scale"), &PhysicsCommandComponentResource::get_gravity_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.1"), 
                 "set_gravity_scale", "get_gravity_scale");

    ClassDB::bind_method(D_METHOD("set_use_gravity_scale", "use"), &PhysicsCommandComponentResource::set_use_gravity_scale);
    ClassDB::bind_method(D_METHOD("get_use_gravity_scale"), &PhysicsCommandComponentResource::get_use_gravity_scale);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_gravity_scale"), "set_use_gravity_scale", "get_use_gravity_scale");

    ClassDB::bind_method(D_METHOD("set_linear_damping", "damping"), &PhysicsCommandComponentResource::set_linear_damping);
    ClassDB::bind_method(D_METHOD("get_linear_damping"), &PhysicsCommandComponentResource::get_linear_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "linear_damping", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), 
                 "set_linear_damping", "get_linear_damping");

    ClassDB::bind_method(D_METHOD("set_use_linear_damping", "use"), &PhysicsCommandComponentResource::set_use_linear_damping);
    ClassDB::bind_method(D_METHOD("get_use_linear_damping"), &PhysicsCommandComponentResource::get_use_linear_damping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_linear_damping"), "set_use_linear_damping", "get_use_linear_damping");

    ClassDB::bind_method(D_METHOD("set_angular_damping", "damping"), &PhysicsCommandComponentResource::set_angular_damping);
    ClassDB::bind_method(D_METHOD("get_angular_damping"), &PhysicsCommandComponentResource::get_angular_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angular_damping", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), 
                 "set_angular_damping", "get_angular_damping");

    ClassDB::bind_method(D_METHOD("set_use_angular_damping", "use"), &PhysicsCommandComponentResource::set_use_angular_damping);
    ClassDB::bind_method(D_METHOD("get_use_angular_damping"), &PhysicsCommandComponentResource::get_use_angular_damping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_angular_damping"), "set_use_angular_damping", "get_use_angular_damping");

    // 狀態控制
    ClassDB::bind_method(D_METHOD("set_activate_body", "activate"), &PhysicsCommandComponentResource::set_activate_body);
    ClassDB::bind_method(D_METHOD("get_activate_body"), &PhysicsCommandComponentResource::get_activate_body);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "activate_body"), "set_activate_body", "get_activate_body");

    ClassDB::bind_method(D_METHOD("set_use_activate", "use"), &PhysicsCommandComponentResource::set_use_activate);
    ClassDB::bind_method(D_METHOD("get_use_activate"), &PhysicsCommandComponentResource::get_use_activate);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_activate"), "set_use_activate", "get_use_activate");

    ClassDB::bind_method(D_METHOD("set_deactivate_body", "deactivate"), &PhysicsCommandComponentResource::set_deactivate_body);
    ClassDB::bind_method(D_METHOD("get_deactivate_body"), &PhysicsCommandComponentResource::get_deactivate_body);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deactivate_body"), "set_deactivate_body", "get_deactivate_body");

    ClassDB::bind_method(D_METHOD("set_use_deactivate", "use"), &PhysicsCommandComponentResource::set_use_deactivate);
    ClassDB::bind_method(D_METHOD("get_use_deactivate"), &PhysicsCommandComponentResource::get_use_deactivate);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_deactivate"), "set_use_deactivate", "get_use_deactivate");

    // 執行控制
    ClassDB::bind_method(D_METHOD("set_command_timing", "timing"), &PhysicsCommandComponentResource::set_command_timing);
    ClassDB::bind_method(D_METHOD("get_command_timing"), &PhysicsCommandComponentResource::get_command_timing);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "command_timing", PROPERTY_HINT_ENUM, "Immediate:0,Before Physics:1,After Physics:2"), 
                 "set_command_timing", "get_command_timing");

    ClassDB::bind_method(D_METHOD("set_execute_once", "once"), &PhysicsCommandComponentResource::set_execute_once);
    ClassDB::bind_method(D_METHOD("get_execute_once"), &PhysicsCommandComponentResource::get_execute_once);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "execute_once"), "set_execute_once", "get_execute_once");

    ClassDB::bind_method(D_METHOD("set_execution_delay", "delay"), &PhysicsCommandComponentResource::set_execution_delay);
    ClassDB::bind_method(D_METHOD("get_execution_delay"), &PhysicsCommandComponentResource::get_execution_delay);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "execution_delay", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), 
                 "set_execution_delay", "get_execution_delay");

    // 便利方法
    ClassDB::bind_method(D_METHOD("clear_all_commands"), &PhysicsCommandComponentResource::clear_all_commands);
    ClassDB::bind_method(D_METHOD("add_simple_force", "force"), &PhysicsCommandComponentResource::add_simple_force);
    ClassDB::bind_method(D_METHOD("add_simple_impulse", "impulse"), &PhysicsCommandComponentResource::add_simple_impulse);
    ClassDB::bind_method(D_METHOD("teleport_to", "position", "rotation_euler"), &PhysicsCommandComponentResource::teleport_to);
}

PhysicsCommandComponentResource::PhysicsCommandComponentResource()
    : add_force(Vector3(0.0f, 0.0f, 0.0f))
    , add_impulse(Vector3(0.0f, 0.0f, 0.0f))
    , add_torque(Vector3(0.0f, 0.0f, 0.0f))
    , add_angular_impulse(Vector3(0.0f, 0.0f, 0.0f))
    , set_linear_velocity(Vector3(0.0f, 0.0f, 0.0f))
    , set_angular_velocity(Vector3(0.0f, 0.0f, 0.0f))
    , use_set_linear_velocity(false)
    , use_set_angular_velocity(false)
    , set_position(Vector3(0.0f, 0.0f, 0.0f))
    , set_rotation(Vector3(0.0f, 0.0f, 0.0f))
    , use_set_position(false)
    , use_set_rotation(false)
    , gravity_scale(1.0f)
    , linear_damping(0.05f)
    , angular_damping(0.05f)
    , use_gravity_scale(false)
    , use_linear_damping(false)
    , use_angular_damping(false)
    , activate_body(false)
    , deactivate_body(false)
    , use_activate(false)
    , use_deactivate(false)
    , command_timing(1) // Before Physics 預設
    , execute_once(true)
    , execution_delay(0.0f)
{
}

PhysicsCommandComponentResource::~PhysicsCommandComponentResource()
{
}

// 力操作屬性實現
void PhysicsCommandComponentResource::set_add_force(const Vector3& p_force) { add_force = p_force; }
Vector3 PhysicsCommandComponentResource::get_add_force() const { return add_force; }
void PhysicsCommandComponentResource::set_add_impulse(const Vector3& p_impulse) { add_impulse = p_impulse; }
Vector3 PhysicsCommandComponentResource::get_add_impulse() const { return add_impulse; }
void PhysicsCommandComponentResource::set_add_torque(const Vector3& p_torque) { add_torque = p_torque; }
Vector3 PhysicsCommandComponentResource::get_add_torque() const { return add_torque; }
void PhysicsCommandComponentResource::set_add_angular_impulse(const Vector3& p_impulse) { add_angular_impulse = p_impulse; }
Vector3 PhysicsCommandComponentResource::get_add_angular_impulse() const { return add_angular_impulse; }

// 速度設置屬性實現
void PhysicsCommandComponentResource::set_set_linear_velocity(const Vector3& p_velocity) { set_linear_velocity = p_velocity; }
Vector3 PhysicsCommandComponentResource::get_set_linear_velocity() const { return set_linear_velocity; }
void PhysicsCommandComponentResource::set_use_set_linear_velocity(bool p_use) { use_set_linear_velocity = p_use; }
bool PhysicsCommandComponentResource::get_use_set_linear_velocity() const { return use_set_linear_velocity; }
void PhysicsCommandComponentResource::set_set_angular_velocity(const Vector3& p_velocity) { set_angular_velocity = p_velocity; }
Vector3 PhysicsCommandComponentResource::get_set_angular_velocity() const { return set_angular_velocity; }
void PhysicsCommandComponentResource::set_use_set_angular_velocity(bool p_use) { use_set_angular_velocity = p_use; }
bool PhysicsCommandComponentResource::get_use_set_angular_velocity() const { return use_set_angular_velocity; }

// 位置設置屬性實現
void PhysicsCommandComponentResource::set_set_position(const Vector3& p_position) { set_position = p_position; }
Vector3 PhysicsCommandComponentResource::get_set_position() const { return set_position; }
void PhysicsCommandComponentResource::set_use_set_position(bool p_use) { use_set_position = p_use; }
bool PhysicsCommandComponentResource::get_use_set_position() const { return use_set_position; }
void PhysicsCommandComponentResource::set_set_rotation(const Vector3& p_rotation) { set_rotation = p_rotation; }
Vector3 PhysicsCommandComponentResource::get_set_rotation() const { return set_rotation; }
void PhysicsCommandComponentResource::set_use_set_rotation(bool p_use) { use_set_rotation = p_use; }
bool PhysicsCommandComponentResource::get_use_set_rotation() const { return use_set_rotation; }

// 物理屬性控制實現
void PhysicsCommandComponentResource::set_gravity_scale(float p_scale) { gravity_scale = p_scale; }
float PhysicsCommandComponentResource::get_gravity_scale() const { return gravity_scale; }
void PhysicsCommandComponentResource::set_use_gravity_scale(bool p_use) { use_gravity_scale = p_use; }
bool PhysicsCommandComponentResource::get_use_gravity_scale() const { return use_gravity_scale; }
void PhysicsCommandComponentResource::set_linear_damping(float p_damping) { linear_damping = p_damping; }
float PhysicsCommandComponentResource::get_linear_damping() const { return linear_damping; }
void PhysicsCommandComponentResource::set_use_linear_damping(bool p_use) { use_linear_damping = p_use; }
bool PhysicsCommandComponentResource::get_use_linear_damping() const { return use_linear_damping; }
void PhysicsCommandComponentResource::set_angular_damping(float p_damping) { angular_damping = p_damping; }
float PhysicsCommandComponentResource::get_angular_damping() const { return angular_damping; }
void PhysicsCommandComponentResource::set_use_angular_damping(bool p_use) { use_angular_damping = p_use; }
bool PhysicsCommandComponentResource::get_use_angular_damping() const { return use_angular_damping; }

// 狀態控制實現
void PhysicsCommandComponentResource::set_activate_body(bool p_activate) { activate_body = p_activate; }
bool PhysicsCommandComponentResource::get_activate_body() const { return activate_body; }
void PhysicsCommandComponentResource::set_use_activate(bool p_use) { use_activate = p_use; }
bool PhysicsCommandComponentResource::get_use_activate() const { return use_activate; }
void PhysicsCommandComponentResource::set_deactivate_body(bool p_deactivate) { deactivate_body = p_deactivate; }
bool PhysicsCommandComponentResource::get_deactivate_body() const { return deactivate_body; }
void PhysicsCommandComponentResource::set_use_deactivate(bool p_use) { use_deactivate = p_use; }
bool PhysicsCommandComponentResource::get_use_deactivate() const { return use_deactivate; }

// 執行控制實現
void PhysicsCommandComponentResource::set_command_timing(int p_timing) { command_timing = p_timing; }
int PhysicsCommandComponentResource::get_command_timing() const { return command_timing; }
void PhysicsCommandComponentResource::set_execute_once(bool p_once) { execute_once = p_once; }
bool PhysicsCommandComponentResource::get_execute_once() const { return execute_once; }
void PhysicsCommandComponentResource::set_execution_delay(float p_delay) { execution_delay = p_delay; }
float PhysicsCommandComponentResource::get_execution_delay() const { return execution_delay; }

// 實現基類純虛函數
bool PhysicsCommandComponentResource::apply_to_entity(entt::registry& registry, entt::entity entity)
{
    // 創建或獲取現有的物理命令組件
    portal_core::PhysicsCommandComponent* cpp_component = nullptr;
    
    if (registry.any_of<portal_core::PhysicsCommandComponent>(entity)) {
        cpp_component = &registry.get<portal_core::PhysicsCommandComponent>(entity);
    } else {
        cpp_component = &registry.emplace<portal_core::PhysicsCommandComponent>(entity);
    }
    
    // 創建所有配置的命令
    create_commands_for_component(*cpp_component);
    
    UtilityFunctions::print("PhysicsCommandComponent applied to entity: ", (int)entity);
    return true;
}

bool PhysicsCommandComponentResource::remove_from_entity(entt::registry& registry, entt::entity entity)
{
    if (registry.any_of<portal_core::PhysicsCommandComponent>(entity)) {
        registry.remove<portal_core::PhysicsCommandComponent>(entity);
        UtilityFunctions::print("PhysicsCommandComponent removed from entity: ", (int)entity);
        return true;
    }
    return false;
}

bool PhysicsCommandComponentResource::has_component(const entt::registry& registry, entt::entity entity) const
{
    return registry.any_of<portal_core::PhysicsCommandComponent>(entity);
}

String PhysicsCommandComponentResource::get_component_type_name() const
{
    return "PhysicsCommandComponent";
}

void PhysicsCommandComponentResource::sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node)
{
    // 物理命令組件不需要同步到節點，因為它們是一次性的操作
}

// 便利方法實現
void PhysicsCommandComponentResource::clear_all_commands()
{
    add_force = Vector3(0.0f, 0.0f, 0.0f);
    add_impulse = Vector3(0.0f, 0.0f, 0.0f);
    add_torque = Vector3(0.0f, 0.0f, 0.0f);
    add_angular_impulse = Vector3(0.0f, 0.0f, 0.0f);
    use_set_linear_velocity = false;
    use_set_angular_velocity = false;
    use_set_position = false;
    use_set_rotation = false;
    use_gravity_scale = false;
    use_linear_damping = false;
    use_angular_damping = false;
    use_activate = false;
    use_deactivate = false;
}

void PhysicsCommandComponentResource::add_simple_force(const Vector3& force)
{
    add_force = force;
}

void PhysicsCommandComponentResource::add_simple_impulse(const Vector3& impulse)
{
    add_impulse = impulse;
}

void PhysicsCommandComponentResource::teleport_to(const Vector3& position, const Vector3& rotation_euler)
{
    set_position = position;
    set_rotation = rotation_euler;
    use_set_position = true;
    use_set_rotation = true;
}

// 私有輔助方法
portal_core::PhysicsCommandTiming PhysicsCommandComponentResource::get_cpp_timing() const
{
    switch (command_timing) {
        case 0: return portal_core::PhysicsCommandTiming::IMMEDIATE;
        case 1: return portal_core::PhysicsCommandTiming::BEFORE_PHYSICS_STEP;
        case 2: return portal_core::PhysicsCommandTiming::AFTER_PHYSICS_STEP;
        default: return portal_core::PhysicsCommandTiming::BEFORE_PHYSICS_STEP;
    }
}

void PhysicsCommandComponentResource::create_commands_for_component(portal_core::PhysicsCommandComponent& cpp_component) const
{
    auto timing = get_cpp_timing();
    
    // 添加力命令
    if (add_force.length() > 0.001f) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::ADD_FORCE, 
                    portal_core::Vector3(add_force.x, add_force.y, add_force.z)), execution_delay);
        } else {
            cpp_component.add_force(portal_core::Vector3(add_force.x, add_force.y, add_force.z), timing);
        }
    }
    
    // 添加衝量命令
    if (add_impulse.length() > 0.001f) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::ADD_IMPULSE, 
                    portal_core::Vec3(add_impulse.x, add_impulse.y, add_impulse.z)), execution_delay);
        } else {
            cpp_component.add_impulse(portal_core::Vec3(add_impulse.x, add_impulse.y, add_impulse.z), timing);
        }
    }
    
    // 添加扭矩命令
    if (add_torque.length() > 0.001f) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::ADD_TORQUE, 
                    portal_core::Vec3(add_torque.x, add_torque.y, add_torque.z)), execution_delay);
        } else {
            cpp_component.add_torque(portal_core::Vec3(add_torque.x, add_torque.y, add_torque.z), timing);
        }
    }
    
    // 添加角衝量命令 - 使用延遲命令方式，因為沒有直接的便利方法
    if (add_angular_impulse.length() > 0.001f) {
        portal_core::PhysicsCommand angular_impulse_cmd(
            portal_core::PhysicsCommandType::ADD_ANGULAR_IMPULSE, 
            portal_core::Vec3(add_angular_impulse.x, add_angular_impulse.y, add_angular_impulse.z)
        );
        angular_impulse_cmd.timing = timing;
        angular_impulse_cmd.command_id = cpp_component.next_command_id++;
        
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(angular_impulse_cmd, execution_delay);
        } else {
            // 直接添加到相應的命令隊列
            switch (timing) {
                case portal_core::PhysicsCommandTiming::IMMEDIATE:
                    cpp_component.immediate_commands.push_back(angular_impulse_cmd);
                    break;
                case portal_core::PhysicsCommandTiming::BEFORE_PHYSICS_STEP:
                    cpp_component.before_physics_commands.push_back(angular_impulse_cmd);
                    break;
                case portal_core::PhysicsCommandTiming::AFTER_PHYSICS_STEP:
                    cpp_component.after_physics_commands.push_back(angular_impulse_cmd);
                    break;
                default:
                    cpp_component.before_physics_commands.push_back(angular_impulse_cmd);
                    break;
            }
        }
    }
    
    // 設置線性速度命令
    if (use_set_linear_velocity) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::SET_LINEAR_VELOCITY, 
                    portal_core::Vec3(set_linear_velocity.x, set_linear_velocity.y, set_linear_velocity.z)), execution_delay);
        } else {
            cpp_component.set_linear_velocity(portal_core::Vec3(set_linear_velocity.x, set_linear_velocity.y, set_linear_velocity.z), timing);
        }
    }
    
    // 設置角速度命令
    if (use_set_angular_velocity) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::SET_ANGULAR_VELOCITY, 
                    portal_core::Vec3(set_angular_velocity.x, set_angular_velocity.y, set_angular_velocity.z)), execution_delay);
        } else {
            cpp_component.set_angular_velocity(portal_core::Vec3(set_angular_velocity.x, set_angular_velocity.y, set_angular_velocity.z), timing);
        }
    }
    
    // 設置位置命令
    if (use_set_position) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::SET_POSITION, 
                    portal_core::Vec3(set_position.x, set_position.y, set_position.z)), execution_delay);
        } else {
            cpp_component.set_position(portal_core::Vec3(set_position.x, set_position.y, set_position.z), timing);
        }
    }
    
    // 瞬移命令（位置+旋轉）
    if (use_set_position && use_set_rotation) {
        // 將歐拉角轉換為四元數
        portal_core::Quaternion quat = portal_core::Quaternion::sEulerAngles(
            portal_core::Vector3(set_rotation.x, set_rotation.y, set_rotation.z)
        );
        
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::TELEPORT, 
                    std::make_pair(portal_core::Vector3(set_position.x, set_position.y, set_position.z), quat)), execution_delay);
        } else {
            cpp_component.teleport(
                portal_core::Vector3(set_position.x, set_position.y, set_position.z), 
                quat, timing);
        }
    }
    
    // 重力縮放命令
    if (use_gravity_scale) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::SET_GRAVITY_SCALE, gravity_scale), execution_delay);
        } else {
            cpp_component.set_gravity_scale(gravity_scale, timing);
        }
    }
    
    // 激活/停用命令
    if (use_activate && activate_body) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::ACTIVATE), execution_delay);
        } else {
            cpp_component.activate(timing);
        }
    }
    
    if (use_deactivate && deactivate_body) {
        if (execution_delay > 0.0f) {
            cpp_component.add_delayed_command(
                portal_core::PhysicsCommand(portal_core::PhysicsCommandType::DEACTIVATE), execution_delay);
        } else {
            cpp_component.deactivate(timing);
        }
    }
    
    // 設置命令是否在執行後自動清除
    cpp_component.clear_after_execution = execute_once;
}

REGISTER_COMPONENT_RESOURCE(PhysicsCommandComponentResource)
