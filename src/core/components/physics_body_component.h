#pragma once

#include "../physics_world_manager.h"
#include "../math_types.h"
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <unordered_set>
#include <functional>
#include <iostream>
#include <algorithm>
#include <entt/entt.hpp>

namespace portal_core {

/**
 * 物理體組件
 * 包含物理屬性和Jolt物理引擎的BodyID映射
 */
struct PhysicsBodyComponent {
    // Jolt物理體ID
    JPH::BodyID body_id;
    
    // 物理體類型
    PhysicsBodyType body_type = PhysicsBodyType::DYNAMIC;
    
    // 形狀描述
    PhysicsShapeDesc shape;
    
    // 材質屬性
    PhysicsMaterial material;
    
    // 運動屬性
    Vec3 linear_velocity = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 angular_velocity = Vec3(0.0f, 0.0f, 0.0f);
    
    // 物理狀態標誌
    bool is_active = true;
    bool allow_sleeping = true;
    bool is_kinematic = false;
    bool is_trigger = false;
    
    // 質量屬性（僅動態物體有效）
    float mass = 1.0f;
    Vec3 center_of_mass = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 inertia_tensor = Vec3(1.0f, 1.0f, 1.0f);
    
    // 運動限制
    bool lock_linear_x = false;
    bool lock_linear_y = false;
    bool lock_linear_z = false;
    bool lock_angular_x = false;
    bool lock_angular_y = false;
    bool lock_angular_z = false;
    
    // 阻尼
    float linear_damping = 0.05f;
    float angular_damping = 0.05f;
    
    // 最大速度限制
    float max_linear_velocity = 500.0f;
    float max_angular_velocity = 47.1f; // 7.5 * 2 * PI 轉/秒
    
    // 重力縮放（1.0 = 正常重力，0.0 = 無重力）
    float gravity_scale = 1.0f;
    
    // 連續碰撞檢測（CCD）設定
    bool enable_ccd = false;
    float ccd_motion_threshold = 1.0f;
    
    // 用戶數據
    uint64_t user_data = 0;
    
    // 碰撞過濾
    struct CollisionFilter {
        uint32_t collision_layer = 1;     // 此物體所在的層
        uint32_t collision_mask = 0xFFFFFFFF;  // 可以碰撞的層遮罩
        int16_t collision_group = 0;      // 碰撞組（負值表示不碰撞同組）
    } collision_filter;
    
    // 構造函數
    PhysicsBodyComponent() = default;
    
    PhysicsBodyComponent(PhysicsBodyType type, const PhysicsShapeDesc& shape_desc) 
        : body_type(type), shape(shape_desc) {
        
        // 根據類型設置預設值
        switch (type) {
            case PhysicsBodyType::STATIC:
                is_active = false;
                allow_sleeping = false;
                mass = 0.0f;
                break;
            case PhysicsBodyType::KINEMATIC:
                is_kinematic = true;
                mass = 0.0f;
                break;
            case PhysicsBodyType::TRIGGER:
                is_trigger = true;
                mass = 0.0f;
                break;
            case PhysicsBodyType::DYNAMIC:
            default:
                // 保持預設值
                break;
        }
    }
    
    // 輔助方法
    
    /**
     * 檢查物理體是否有效
     */
    bool is_valid() const {
        return !body_id.IsInvalid();
    }
    
    /**
     * 設置為盒子形狀（安全版本）
     */
    void set_box_shape(const Vec3& size) {
        Vec3 safe_size = size;
        bool modified = false;
        
        // 檢查並修正無效尺寸
        if (safe_size.GetX() <= 0.0f) {
            safe_size.SetX(0.1f);
            modified = true;
        }
        if (safe_size.GetY() <= 0.0f) {
            safe_size.SetY(0.1f);
            modified = true;
        }
        if (safe_size.GetZ() <= 0.0f) {
            safe_size.SetZ(0.1f);
            modified = true;
        }
        
        if (modified) {
            std::cerr << "PhysicsBodyComponent: Warning - Invalid box size detected and corrected from (" 
                      << size.GetX() << ", " << size.GetY() << ", " << size.GetZ() << ") to ("
                      << safe_size.GetX() << ", " << safe_size.GetY() << ", " << safe_size.GetZ() << ")" << std::endl;
        }
        
        shape = PhysicsShapeDesc::box(safe_size);
    }
    
    /**
     * 設置為球體形狀（安全版本）
     */
    void set_sphere_shape(float radius) {
        float safe_radius = radius;
        
        if (safe_radius <= 0.0f) {
            safe_radius = 0.5f;
            std::cerr << "PhysicsBodyComponent: Warning - Invalid sphere radius " << radius 
                      << " corrected to " << safe_radius << std::endl;
        }
        
        shape = PhysicsShapeDesc::sphere(safe_radius);
    }
    
    /**
     * 設置為膠囊形狀（安全版本）
     */
    void set_capsule_shape(float radius, float height) {
        float safe_radius = radius;
        float safe_height = height;
        bool modified = false;
        
        if (safe_radius <= 0.0f) {
            safe_radius = 0.5f;
            modified = true;
        }
        if (safe_height <= 0.0f) {
            safe_height = 1.0f;
            modified = true;
        }
        
        if (modified) {
            std::cerr << "PhysicsBodyComponent: Warning - Invalid capsule dimensions (" 
                      << radius << ", " << height << ") corrected to ("
                      << safe_radius << ", " << safe_height << ")" << std::endl;
        }
        
        shape = PhysicsShapeDesc::capsule(safe_radius, safe_height);
    }
    
    /**
     * 設置材質屬性（安全版本）
     */
    void set_material(float friction, float restitution, float density = 1000.0f) {
        float safe_friction = friction;
        float safe_restitution = restitution;
        float safe_density = density;
        bool modified = false;
        
        // 修正摩擦係數
        if (safe_friction < 0.0f) {
            safe_friction = 0.0f;
            modified = true;
        }
        
        // 修正彈性係數
        if (safe_restitution < 0.0f) {
            safe_restitution = 0.0f;
            modified = true;
        } else if (safe_restitution > 1.0f) {
            safe_restitution = 1.0f;
            modified = true;
        }
        
        // 修正密度（針對需要質量的物體類型）
        if ((body_type == PhysicsBodyType::DYNAMIC || body_type == PhysicsBodyType::KINEMATIC) 
            && safe_density <= 0.0f) {
            safe_density = 1000.0f;
            modified = true;
        }
        
        if (modified) {
            std::cerr << "PhysicsBodyComponent: Warning - Invalid material properties ("
                      << friction << ", " << restitution << ", " << density << ") corrected to ("
                      << safe_friction << ", " << safe_restitution << ", " << safe_density << ")" << std::endl;
        }
        
        material.friction = safe_friction;
        material.restitution = safe_restitution;
        material.density = safe_density;
    }
    
    /**
     * 設置碰撞過濾
     */
    void set_collision_filter(uint32_t layer, uint32_t mask, int16_t group = 0) {
        collision_filter.collision_layer = layer;
        collision_filter.collision_mask = mask;
        collision_filter.collision_group = group;
    }
    
    /**
     * 驗證和修正組件屬性（確保所有值都是安全的）
     */
    bool validate_and_correct() {
        bool corrected = false;
        
        // 修正質量相關屬性
        if (body_type == PhysicsBodyType::DYNAMIC && mass <= 0.0f) {
            mass = 1.0f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Dynamic body mass corrected to 1.0f" << std::endl;
        }
        
        // 修正材質屬性
        if (material.friction < 0.0f) {
            material.friction = 0.0f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Friction corrected to 0.0f" << std::endl;
        }
        
        if (material.restitution < 0.0f || material.restitution > 1.0f) {
            material.restitution = std::clamp(material.restitution, 0.0f, 1.0f);
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Restitution clamped to [0.0, 1.0]" << std::endl;
        }
        
        // 修正密度（對需要質量的物體類型）
        if ((body_type == PhysicsBodyType::DYNAMIC || body_type == PhysicsBodyType::KINEMATIC) 
            && material.density <= 0.0f) {
            material.density = 1000.0f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Density corrected to 1000.0f for " 
                      << (body_type == PhysicsBodyType::DYNAMIC ? "dynamic" : "kinematic") 
                      << " body" << std::endl;
        }
        
        // 修正速度限制
        if (max_linear_velocity <= 0.0f) {
            max_linear_velocity = 500.0f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Max linear velocity corrected to 500.0f" << std::endl;
        }
        
        if (max_angular_velocity <= 0.0f) {
            max_angular_velocity = 47.1f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Max angular velocity corrected to 47.1f" << std::endl;
        }
        
        // 修正阻尼值
        if (linear_damping < 0.0f || linear_damping > 1.0f) {
            linear_damping = std::clamp(linear_damping, 0.0f, 1.0f);
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Linear damping clamped to [0.0, 1.0]" << std::endl;
        }
        
        if (angular_damping < 0.0f || angular_damping > 1.0f) {
            angular_damping = std::clamp(angular_damping, 0.0f, 1.0f);
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Angular damping clamped to [0.0, 1.0]" << std::endl;
        }
        
        // 修正重力縮放
        if (gravity_scale < 0.0f) {
            gravity_scale = 0.0f;
            corrected = true;
            std::cerr << "PhysicsBodyComponent: Warning - Gravity scale corrected to 0.0f (negative values not allowed)" << std::endl;
        }
        
        return corrected;
    }
    
    /**
     * 設置物理體類型（安全版本，會觸發重新驗證）
     */
    void set_body_type_safe(PhysicsBodyType new_type) {
        PhysicsBodyType old_type = body_type;
        body_type = new_type;
        
        // 根據新類型調整相關屬性
        switch (body_type) {
            case PhysicsBodyType::STATIC:
                is_active = false;
                allow_sleeping = false;
                // 靜態物體不需要質量
                break;
            case PhysicsBodyType::KINEMATIC:
                is_kinematic = true;
                // 運動學物體需要有效密度
                if (material.density <= 0.0f) {
                    material.density = 1000.0f;
                    std::cerr << "PhysicsBodyComponent: Warning - Kinematic body density set to 1000.0f" << std::endl;
                }
                break;
            case PhysicsBodyType::TRIGGER:
                is_trigger = true;
                // 觸發器不需要質量
                break;
            case PhysicsBodyType::DYNAMIC:
                // 動態物體需要有效質量和密度
                if (mass <= 0.0f) {
                    mass = 1.0f;
                    std::cerr << "PhysicsBodyComponent: Warning - Dynamic body mass set to 1.0f" << std::endl;
                }
                if (material.density <= 0.0f) {
                    material.density = 1000.0f;
                    std::cerr << "PhysicsBodyComponent: Warning - Dynamic body density set to 1000.0f" << std::endl;
                }
                break;
        }
        
        if (old_type != new_type) {
            std::cout << "PhysicsBodyComponent: Body type changed from " 
                      << static_cast<int>(old_type) << " to " << static_cast<int>(new_type) << std::endl;
        }
    }
    
    /**
     * 創建物理體描述符（用於創建Jolt物理體）
     */
    PhysicsBodyDesc create_physics_body_desc(const Vec3& position, const Quat& rotation) const {
        PhysicsBodyDesc desc;
        desc.body_type = body_type;
        desc.shape = shape;
        desc.material = material;
        
        // 顯式使用 JPH 類型，避免與自定義類型衝突
        desc.position = JPH::RVec3(double(position.GetX()), double(position.GetY()), double(position.GetZ()));
        desc.rotation = JPH::Quat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
        desc.linear_velocity = JPH::Vec3(linear_velocity.GetX(), linear_velocity.GetY(), linear_velocity.GetZ());
        desc.angular_velocity = JPH::Vec3(angular_velocity.GetX(), angular_velocity.GetY(), angular_velocity.GetZ());
        desc.allow_sleeping = allow_sleeping;
        desc.user_data = user_data;
        return desc;
    }
    
    /**
     * 檢查是否為動態物體
     */
    bool is_dynamic() const {
        return body_type == PhysicsBodyType::DYNAMIC;
    }
    
    /**
     * 檢查是否為靜態物體
     */
    bool is_static() const {
        return body_type == PhysicsBodyType::STATIC;
    }
    
    /**
     * 檢查是否可以移動
     */
    bool can_move() const {
        return body_type == PhysicsBodyType::DYNAMIC || body_type == PhysicsBodyType::KINEMATIC;
    }
    
    /**
     * 獲取有效質量（靜態和運動學物體返回0）
     */
    float get_effective_mass() const {
        return is_dynamic() ? mass : 0.0f;
    }
};

} // namespace portal_core
