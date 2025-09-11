#pragma once

#include "ecs_component_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// 引入C++ ECS組件定義
#include "core/components/physics_command_component.h"

using namespace godot;

/**
 * 物理命令組件資源
 * 讓設計師可以在 Godot 編輯器中預設物理操作
 * 如添加力、設置速度、瞬移等
 */
class PhysicsCommandComponentResource : public ECSComponentResource
{
    GDCLASS(PhysicsCommandComponentResource, ECSComponentResource)

private:
    // 基本力操作
    Vector3 add_force;
    Vector3 add_impulse;
    Vector3 add_torque;
    Vector3 add_angular_impulse;
    
    // 速度設置
    Vector3 set_linear_velocity;
    Vector3 set_angular_velocity;
    bool use_set_linear_velocity;
    bool use_set_angular_velocity;
    
    // 位置操作
    Vector3 set_position;
    Vector3 set_rotation; // 歐拉角
    bool use_set_position;
    bool use_set_rotation;
    
    // 物理屬性控制
    float gravity_scale;
    float linear_damping;
    float angular_damping;
    bool use_gravity_scale;
    bool use_linear_damping;
    bool use_angular_damping;
    
    // 狀態控制
    bool activate_body;
    bool deactivate_body;
    bool use_activate;
    bool use_deactivate;
    
    // 執行時機
    int command_timing; // 0=Immediate, 1=BeforePhysics, 2=AfterPhysics
    
    // 執行控制
    bool execute_once; // true=只執行一次, false=每幀執行
    float execution_delay; // 延遲執行時間（秒）

protected:
    static void _bind_methods();

public:
    PhysicsCommandComponentResource();
    ~PhysicsCommandComponentResource();

    // 力操作屬性
    void set_add_force(const Vector3& p_force);
    Vector3 get_add_force() const;
    void set_add_impulse(const Vector3& p_impulse);
    Vector3 get_add_impulse() const;
    void set_add_torque(const Vector3& p_torque);
    Vector3 get_add_torque() const;
    void set_add_angular_impulse(const Vector3& p_impulse);
    Vector3 get_add_angular_impulse() const;
    
    // 速度設置屬性
    void set_set_linear_velocity(const Vector3& p_velocity);
    Vector3 get_set_linear_velocity() const;
    void set_use_set_linear_velocity(bool p_use);
    bool get_use_set_linear_velocity() const;
    void set_set_angular_velocity(const Vector3& p_velocity);
    Vector3 get_set_angular_velocity() const;
    void set_use_set_angular_velocity(bool p_use);
    bool get_use_set_angular_velocity() const;
    
    // 位置設置屬性
    void set_set_position(const Vector3& p_position);
    Vector3 get_set_position() const;
    void set_use_set_position(bool p_use);
    bool get_use_set_position() const;
    void set_set_rotation(const Vector3& p_rotation);
    Vector3 get_set_rotation() const;
    void set_use_set_rotation(bool p_use);
    bool get_use_set_rotation() const;
    
    // 物理屬性控制
    void set_gravity_scale(float p_scale);
    float get_gravity_scale() const;
    void set_use_gravity_scale(bool p_use);
    bool get_use_gravity_scale() const;
    void set_linear_damping(float p_damping);
    float get_linear_damping() const;
    void set_use_linear_damping(bool p_use);
    bool get_use_linear_damping() const;
    void set_angular_damping(float p_damping);
    float get_angular_damping() const;
    void set_use_angular_damping(bool p_use);
    bool get_use_angular_damping() const;
    
    // 狀態控制
    void set_activate_body(bool p_activate);
    bool get_activate_body() const;
    void set_use_activate(bool p_use);
    bool get_use_activate() const;
    void set_deactivate_body(bool p_deactivate);
    bool get_deactivate_body() const;
    void set_use_deactivate(bool p_use);
    bool get_use_deactivate() const;
    
    // 執行控制
    void set_command_timing(int p_timing);
    int get_command_timing() const;
    void set_execute_once(bool p_once);
    bool get_execute_once() const;
    void set_execution_delay(float p_delay);
    float get_execution_delay() const;

    // 實現基類的純虛函數
    virtual bool apply_to_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool remove_from_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool has_component(const entt::registry& registry, entt::entity entity) const override;
    virtual String get_component_type_name() const override;
    virtual void sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node) override;
    
    // 便利方法
    void clear_all_commands();
    void add_simple_force(const Vector3& force);
    void add_simple_impulse(const Vector3& impulse);
    void teleport_to(const Vector3& position, const Vector3& rotation_euler);

private:
    // 輔助方法
    portal_core::PhysicsCommandTiming get_cpp_timing() const;
    void create_commands_for_component(portal_core::PhysicsCommandComponent& cpp_component) const;
};
