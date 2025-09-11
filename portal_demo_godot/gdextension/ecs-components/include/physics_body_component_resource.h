#pragma once

#include "ipresettable_resource.h"  // 改为继承自可预设资源接口
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// 引入C++ ECS組件定義
#include "core/components/physics_body_component.h"

using namespace godot;

/**
 * 物理體組件資源
 * 讓設計師可以在 Godot 編輯器中配置物理體屬性
 * 包括形狀、質量、阻尼、碰撞過濾等所有物理參數
 * 
 * 继承自 IPresettableResource，自动获得预设保存/加载功能
 */
class PhysicsBodyComponentResource : public IPresettableResource
{
    GDCLASS(PhysicsBodyComponentResource, IPresettableResource)

private:
    // 物理體類型
    int body_type; // 0=Static, 1=Dynamic, 2=Kinematic, 3=Trigger
    
    // 形狀屬性
    int shape_type; // 0=Box, 1=Sphere, 2=Capsule
    Vector3 shape_size; // 盒子的半尺寸 或 球體半徑(x) 或 膠囊半徑(x)+高度(y)
    
    // 材質屬性
    float friction;
    float restitution;
    float density;
    
    // 質量屬性（僅動態物體）
    float mass;
    Vector3 center_of_mass;
    
    // 運動屬性
    Vector3 linear_velocity;
    Vector3 angular_velocity;
    
    // 阻尼
    float linear_damping;
    float angular_damping;
    
    // 重力
    float gravity_scale;
    
    // 物理狀態
    bool is_active;
    bool allow_sleeping;
    bool enable_ccd; // 連續碰撞檢測
    
    // 運動限制
    bool lock_linear_x;
    bool lock_linear_y;
    bool lock_linear_z;
    bool lock_angular_x;
    bool lock_angular_y;
    bool lock_angular_z;
    
    // 速度限制
    float max_linear_velocity;
    float max_angular_velocity;
    
    // 碰撞過濾
    int collision_layer;
    int collision_mask;
    int collision_group;

protected:
    static void _bind_methods();

public:
    PhysicsBodyComponentResource();
    ~PhysicsBodyComponentResource();

    // 物理體類型屬性
    void set_body_type(int p_type);
    int get_body_type() const;
    
    // 形狀屬性
    void set_shape_type(int p_type);
    int get_shape_type() const;
    void set_shape_size(const Vector3& p_size);
    Vector3 get_shape_size() const;
    
    // 材質屬性
    void set_friction(float p_friction);
    float get_friction() const;
    void set_restitution(float p_restitution);
    float get_restitution() const;
    void set_density(float p_density);
    float get_density() const;
    
    // 質量屬性
    void set_mass(float p_mass);
    float get_mass() const;
    void set_center_of_mass(const Vector3& p_center);
    Vector3 get_center_of_mass() const;
    
    // 運動屬性
    void set_linear_velocity(const Vector3& p_velocity);
    Vector3 get_linear_velocity() const;
    void set_angular_velocity(const Vector3& p_velocity);
    Vector3 get_angular_velocity() const;
    
    // 阻尼屬性
    void set_linear_damping(float p_damping);
    float get_linear_damping() const;
    void set_angular_damping(float p_damping);
    float get_angular_damping() const;
    
    // 重力屬性
    void set_gravity_scale(float p_scale);
    float get_gravity_scale() const;
    
    // 物理狀態
    void set_is_active(bool p_active);
    bool get_is_active() const;
    void set_allow_sleeping(bool p_allow);
    bool get_allow_sleeping() const;
    void set_enable_ccd(bool p_enable);
    bool get_enable_ccd() const;
    
    // 運動限制
    void set_lock_linear_x(bool p_lock);
    bool get_lock_linear_x() const;
    void set_lock_linear_y(bool p_lock);
    bool get_lock_linear_y() const;
    void set_lock_linear_z(bool p_lock);
    bool get_lock_linear_z() const;
    void set_lock_angular_x(bool p_lock);
    bool get_lock_angular_x() const;
    void set_lock_angular_y(bool p_lock);
    bool get_lock_angular_y() const;
    void set_lock_angular_z(bool p_lock);
    bool get_lock_angular_z() const;
    
    // 速度限制
    void set_max_linear_velocity(float p_max);
    float get_max_linear_velocity() const;
    void set_max_angular_velocity(float p_max);
    float get_max_angular_velocity() const;
    
    // 碰撞過濾
    void set_collision_layer(int p_layer);
    int get_collision_layer() const;
    void set_collision_mask(int p_mask);
    int get_collision_mask() const;
    void set_collision_group(int p_group);
    int get_collision_group() const;

    // 實現基類的純虛函數
    virtual bool apply_to_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool remove_from_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool has_component(const entt::registry& registry, entt::entity entity) const override;
    virtual String get_component_type_name() const override;
    virtual void sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node) override;
    
    // 便利方法
    void set_box_shape(const Vector3& half_extents);
    void set_sphere_shape(float radius);
    void set_capsule_shape(float radius, float height);
    void set_dynamic_body();
    void set_static_body();
    void set_kinematic_body();
    void set_trigger_body();

    // 重写预设相关方法，提供更友好的显示名称
    virtual String get_preset_display_name() const override {
        return "Physics Body";
    }

    // 约束验证方法
    Array validate_constraints() const;
    String get_constraint_warnings() const override;

    // 实现 IPresettableResource 接口的自动填充功能
    virtual Array get_auto_fill_capabilities() const override;
    virtual Dictionary auto_fill_from_node(Node* target_node, const String& capability_name = "") override;

private:
    // 輔助方法
    portal_core::PhysicsBodyType get_cpp_body_type() const;
    portal_core::PhysicsShapeDesc create_cpp_shape() const;
    portal_core::PhysicsMaterial create_cpp_material() const;
    void apply_properties_to_cpp_component(portal_core::PhysicsBodyComponent& cpp_component) const;
    void sync_from_cpp_component(const portal_core::PhysicsBodyComponent& cpp_component);
    
    // 屬性變化處理
    void _on_property_changed();

    // 自动填充辅助方法
    Dictionary auto_fill_from_mesh_instance(Node* node);
    Dictionary auto_fill_from_collision_shape(Node* node);
    Dictionary auto_fill_from_rigid_body(Node* node);
    Dictionary auto_fill_from_static_body(Node* node);
    Vector3 calculate_mesh_bounds(Node* mesh_instance);
    bool extract_collision_shape_data(Node* collision_shape, int& shape_type_out, Vector3& size_out);
};
