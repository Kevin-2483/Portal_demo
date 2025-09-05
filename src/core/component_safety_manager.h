#pragma once

#include "components/physics_body_component.h"
#include "components/transform_component.h"
#include <iostream>
#include <string>

namespace portal_core {

/**
 * ECS 組件安全檢查和自動修正工具
 * 提供統一的組件驗證、修正和警告機制
 */
class ComponentSafetyManager {
public:
    /**
     * 驗證和修正 PhysicsBodyComponent
     * @param component 要檢查的組件
     * @param entity_id 實體ID（用於日誌）
     * @return 是否進行了修正
     */
    static bool validate_and_correct_physics_body(PhysicsBodyComponent& component, uint32_t entity_id = 0) {
        bool corrected = false;
        std::string entity_info = entity_id > 0 ? " (Entity " + std::to_string(entity_id) + ")" : "";
        
        // 檢查物理體類型相關的限制
        corrected |= validate_body_type_properties(component, entity_info);
        
        // 檢查材質屬性
        corrected |= validate_material_properties(component, entity_info);
        
        // 檢查形狀屬性
        corrected |= validate_shape_properties(component, entity_info);
        
        // 檢查運動屬性
        corrected |= validate_motion_properties(component, entity_info);
        
        return corrected;
    }
    
    /**
     * 驗證 TransformComponent
     * @param component 要檢查的組件
     * @param entity_id 實體ID（用於日誌）
     * @return 是否進行了修正
     */
    static bool validate_and_correct_transform(TransformComponent& component, uint32_t entity_id = 0) {
        bool corrected = false;
        std::string entity_info = entity_id > 0 ? " (Entity " + std::to_string(entity_id) + ")" : "";
        
        // 檢查縮放值
        if (component.scale.GetX() <= 0.0f || component.scale.GetY() <= 0.0f || component.scale.GetZ() <= 0.0f) {
            Vector3 old_scale = component.scale;
            component.scale = Vector3(
                component.scale.GetX() <= 0.0f ? 1.0f : component.scale.GetX(),
                component.scale.GetY() <= 0.0f ? 1.0f : component.scale.GetY(),
                component.scale.GetZ() <= 0.0f ? 1.0f : component.scale.GetZ()
            );
            
            log_warning("TransformComponent", "Invalid scale corrected from (" +
                       std::to_string(old_scale.GetX()) + ", " + std::to_string(old_scale.GetY()) + ", " + std::to_string(old_scale.GetZ()) + ") to (" +
                       std::to_string(component.scale.GetX()) + ", " + std::to_string(component.scale.GetY()) + ", " + std::to_string(component.scale.GetZ()) + ")" + entity_info);
            corrected = true;
        }
        
        // 檢查旋轉四元數是否歸一化
        float magnitude = std::sqrt(component.rotation.GetW() * component.rotation.GetW() + 
                                   component.rotation.GetX() * component.rotation.GetX() + 
                                   component.rotation.GetY() * component.rotation.GetY() + 
                                   component.rotation.GetZ() * component.rotation.GetZ());
        
        if (std::abs(magnitude - 1.0f) > 1e-6f) {
            if (magnitude > 1e-6f) {
                component.rotation = Quaternion(
                    component.rotation.GetW() / magnitude,
                    component.rotation.GetX() / magnitude,
                    component.rotation.GetY() / magnitude,
                    component.rotation.GetZ() / magnitude
                );
            } else {
                component.rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);  // 身份四元數
            }
            
            log_warning("TransformComponent", "Quaternion normalized" + entity_info);
            corrected = true;
        }
        
        return corrected;
    }
    
    /**
     * 檢查組件間的依賴關係
     */
    static bool validate_component_dependencies(entt::registry& registry, entt::entity entity) {
        bool valid = true;
        uint32_t entity_id = static_cast<uint32_t>(entity);
        
        // 檢查物理體必需的依賴
        if (registry.try_get<PhysicsBodyComponent>(entity)) {
            if (!registry.try_get<TransformComponent>(entity)) {
                log_error("ComponentDependency", "PhysicsBodyComponent requires TransformComponent (Entity " + std::to_string(entity_id) + ")");
                valid = false;
            }
        }
        
        return valid;
    }

private:
    /**
     * 驗證物理體類型相關屬性
     */
    static bool validate_body_type_properties(PhysicsBodyComponent& component, const std::string& entity_info) {
        bool corrected = false;
        
        // 根據物理體類型檢查質量相關屬性
        switch (component.body_type) {
            case PhysicsBodyType::DYNAMIC:
                if (component.mass <= 0.0f) {
                    component.mass = 1.0f;
                    log_warning("PhysicsBodyComponent", "Dynamic body mass corrected to 1.0f" + entity_info);
                    corrected = true;
                }
                if (component.material.density <= 0.0f) {
                    component.material.density = 1000.0f;
                    log_warning("PhysicsBodyComponent", "Dynamic body density corrected to 1000.0f" + entity_info);
                    corrected = true;
                }
                break;
                
            case PhysicsBodyType::KINEMATIC:
                if (component.material.density <= 0.0f) {
                    component.material.density = 1000.0f;
                    log_warning("PhysicsBodyComponent", "Kinematic body density corrected to 1000.0f" + entity_info);
                    corrected = true;
                }
                break;
                
            case PhysicsBodyType::STATIC:
            case PhysicsBodyType::TRIGGER:
                // 靜態物體和觸發器不需要質量，但我們不會自動修改用戶設置的值
                break;
        }
        
        return corrected;
    }
    
    /**
     * 驗證材質屬性
     */
    static bool validate_material_properties(PhysicsBodyComponent& component, const std::string& entity_info) {
        bool corrected = false;
        
        // 修正摩擦係數
        if (component.material.friction < 0.0f) {
            float old_friction = component.material.friction;
            component.material.friction = 0.0f;
            log_warning("PhysicsBodyComponent", "Friction corrected from " + std::to_string(old_friction) + " to 0.0f" + entity_info);
            corrected = true;
        }
        
        // 修正彈性係數
        if (component.material.restitution < 0.0f || component.material.restitution > 1.0f) {
            float old_restitution = component.material.restitution;
            component.material.restitution = std::clamp(component.material.restitution, 0.0f, 1.0f);
            log_warning("PhysicsBodyComponent", "Restitution corrected from " + std::to_string(old_restitution) + 
                       " to " + std::to_string(component.material.restitution) + entity_info);
            corrected = true;
        }
        
        return corrected;
    }
    
    /**
     * 驗證形狀屬性
     */
    static bool validate_shape_properties(PhysicsBodyComponent& component, const std::string& entity_info) {
        bool corrected = false;
        
        switch (component.shape.type) {
            case PhysicsShapeType::BOX:
                if (component.shape.size.GetX() <= 0.0f || component.shape.size.GetY() <= 0.0f || component.shape.size.GetZ() <= 0.0f) {
                    Vec3 old_size = component.shape.size;
                    component.shape.size = Vec3(
                        std::max(component.shape.size.GetX(), 0.1f),
                        std::max(component.shape.size.GetY(), 0.1f),
                        std::max(component.shape.size.GetZ(), 0.1f)
                    );
                    log_warning("PhysicsBodyComponent", "Box size corrected from (" +
                               std::to_string(old_size.GetX()) + ", " + std::to_string(old_size.GetY()) + ", " + std::to_string(old_size.GetZ()) + ") to (" +
                               std::to_string(component.shape.size.GetX()) + ", " + std::to_string(component.shape.size.GetY()) + ", " + std::to_string(component.shape.size.GetZ()) + ")" + entity_info);
                    corrected = true;
                }
                break;
                
            case PhysicsShapeType::SPHERE:
                if (component.shape.radius <= 0.0f) {
                    float old_radius = component.shape.radius;
                    component.shape.radius = 0.5f;
                    log_warning("PhysicsBodyComponent", "Sphere radius corrected from " + std::to_string(old_radius) + " to 0.5f" + entity_info);
                    corrected = true;
                }
                break;
                
            case PhysicsShapeType::CAPSULE:
                if (component.shape.radius <= 0.0f || component.shape.height <= 0.0f) {
                    float old_radius = component.shape.radius;
                    float old_height = component.shape.height;
                    component.shape.radius = std::max(component.shape.radius, 0.5f);
                    component.shape.height = std::max(component.shape.height, 1.0f);
                    log_warning("PhysicsBodyComponent", "Capsule dimensions corrected from (" + 
                               std::to_string(old_radius) + ", " + std::to_string(old_height) + ") to (" +
                               std::to_string(component.shape.radius) + ", " + std::to_string(component.shape.height) + ")" + entity_info);
                    corrected = true;
                }
                break;
                
            default:
                break;
        }
        
        return corrected;
    }
    
    /**
     * 驗證運動屬性
     */
    static bool validate_motion_properties(PhysicsBodyComponent& component, const std::string& entity_info) {
        bool corrected = false;
        
        // 修正最大速度
        if (component.max_linear_velocity <= 0.0f) {
            float old_velocity = component.max_linear_velocity;
            component.max_linear_velocity = 500.0f;
            log_warning("PhysicsBodyComponent", "Max linear velocity corrected from " + std::to_string(old_velocity) + " to 500.0f" + entity_info);
            corrected = true;
        }
        
        if (component.max_angular_velocity <= 0.0f) {
            float old_velocity = component.max_angular_velocity;
            component.max_angular_velocity = 47.1f;
            log_warning("PhysicsBodyComponent", "Max angular velocity corrected from " + std::to_string(old_velocity) + " to 47.1f" + entity_info);
            corrected = true;
        }
        
        // 修正阻尼值
        if (component.linear_damping < 0.0f || component.linear_damping > 1.0f) {
            float old_damping = component.linear_damping;
            component.linear_damping = std::clamp(component.linear_damping, 0.0f, 1.0f);
            log_warning("PhysicsBodyComponent", "Linear damping corrected from " + std::to_string(old_damping) + 
                       " to " + std::to_string(component.linear_damping) + entity_info);
            corrected = true;
        }
        
        if (component.angular_damping < 0.0f || component.angular_damping > 1.0f) {
            float old_damping = component.angular_damping;
            component.angular_damping = std::clamp(component.angular_damping, 0.0f, 1.0f);
            log_warning("PhysicsBodyComponent", "Angular damping corrected from " + std::to_string(old_damping) + 
                       " to " + std::to_string(component.angular_damping) + entity_info);
            corrected = true;
        }
        
        // 修正重力縮放
        if (component.gravity_scale < 0.0f) {
            float old_scale = component.gravity_scale;
            component.gravity_scale = 0.0f;
            log_warning("PhysicsBodyComponent", "Gravity scale corrected from " + std::to_string(old_scale) + " to 0.0f (negative values not allowed)" + entity_info);
            corrected = true;
        }
        
        return corrected;
    }
    
    /**
     * 輸出警告日誌
     */
    static void log_warning(const std::string& component_name, const std::string& message) {
        std::cerr << "[ComponentSafety] Warning - " << component_name << ": " << message << std::endl;
    }
    
    /**
     * 輸出錯誤日誌
     */
    static void log_error(const std::string& component_name, const std::string& message) {
        std::cerr << "[ComponentSafety] Error - " << component_name << ": " << message << std::endl;
    }
};

} // namespace portal_core
