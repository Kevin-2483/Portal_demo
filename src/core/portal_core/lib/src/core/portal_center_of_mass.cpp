#include "../../include/core/portal_center_of_mass.h"
#include "../../include/math/portal_math.h"
#include <algorithm>
#include <iostream>

namespace Portal {

Vector3 CenterOfMassManager::get_entity_center_of_mass_world(
    EntityId entity_id, 
    const Transform& entity_transform) {
    
    // 檢查是否有自定義配置
    auto config_it = entity_configs_.find(entity_id);
    if (config_it == entity_configs_.end()) {
        // 沒有配置，使用預設的幾何中心
        return calculate_geometric_center(entity_id, entity_transform);
    }
    
    const CenterOfMassConfig& config = config_it->second;
    
    // 檢查緩存
    auto cache_it = cached_results_.find(entity_id);
    bool need_recalculate = true;
    
    if (cache_it != cached_results_.end()) {
        const CenterOfMassResult& cached = cache_it->second;
        
        // 如果不需要自動更新，使用緩存
        if (!config.auto_update_on_mesh_change) {
            need_recalculate = false;
        } else if (provider_ && !provider_->has_mesh_changed(entity_id)) {
            // 網格沒有改變，使用緩存
            need_recalculate = false;
        }
    }
    
    if (need_recalculate) {
        // 重新計算
        CenterOfMassResult result = calculate_center_of_mass_internal(
            entity_id, config, entity_transform);
        cached_results_[entity_id] = result;
        return result.world_position;
    } else {
        // 使用緩存，但需要基於新的變換更新世界位置
        const CenterOfMassResult& cached = cache_it->second;
        return entity_transform.transform_point(cached.local_position);
    }
}

Vector3 CenterOfMassManager::get_entity_center_of_mass_local(EntityId entity_id) {
    auto config_it = entity_configs_.find(entity_id);
    if (config_it == entity_configs_.end()) {
        // 沒有配置，返回幾何中心
        return Vector3(0, 0, 0); // 假設幾何中心在原點
    }
    
    const CenterOfMassConfig& config = config_it->second;
    Transform identity_transform; // 單位變換
    
    CenterOfMassResult result = calculate_center_of_mass_internal(
        entity_id, config, identity_transform);
    
    return result.local_position;
}

void CenterOfMassManager::update_auto_update_entities(float delta_time) {
    static float update_timer = 0.0f;
    update_timer += delta_time;
    
    for (auto& [entity_id, config] : entity_configs_) {
        if (config.auto_update_on_mesh_change && 
            update_timer >= config.update_frequency) {
            
            if (provider_ && provider_->has_mesh_changed(entity_id)) {
                // 網格已改變，清除緩存以強制重新計算
                cached_results_.erase(entity_id);
                std::cout << "Auto-updating center of mass for entity " << entity_id 
                          << " due to mesh change\n";
            }
        }
    }
    
    // 重置計時器
    if (update_timer >= 0.1f) { // 每0.1秒檢查一次
        update_timer = 0.0f;
    }
}

void CenterOfMassManager::force_recalculate(EntityId entity_id) {
    // 清除緩存
    cached_results_.erase(entity_id);
    std::cout << "Forced recalculation of center of mass for entity " << entity_id << "\n";
}

CenterOfMassResult CenterOfMassManager::calculate_center_of_mass_internal(
    EntityId entity_id, 
    const CenterOfMassConfig& config, 
    const Transform& entity_transform) {
    
    CenterOfMassResult result;
    
    // 獲取真實時間戳（從提供者或使用預設）
    if (provider_) {
        result.calculation_time = provider_->get_current_timestamp();
    } else {
        // 後備選項：使用簡單的計數器作為時間戳
        static uint64_t counter = 0;
        result.calculation_time = ++counter;
    }
    
    try {
        switch (config.type) {
            case CenterOfMassType::GEOMETRIC_CENTER: {
                result.local_position = Vector3(0, 0, 0); // 幾何中心通常在原點
                result.world_position = entity_transform.transform_point(result.local_position);
                result.is_valid = true;
                break;
            }
            
            case CenterOfMassType::CUSTOM_POINT: {
                result.local_position = config.custom_point;
                result.world_position = entity_transform.transform_point(result.local_position);
                result.is_valid = true;
                std::cout << "Using custom center of mass point (" 
                          << config.custom_point.x << ", " 
                          << config.custom_point.y << ", " 
                          << config.custom_point.z << ") for entity " << entity_id << "\n";
                break;
            }
            
            case CenterOfMassType::BONE_ATTACHMENT: {
                if (provider_) {
                    try {
                        Transform bone_transform = provider_->get_bone_transform(
                            entity_id, config.bone_attachment.bone_name);
                        
                        // 骨骼變換 + 偏移量
                        result.local_position = bone_transform.transform_point(
                            config.bone_attachment.offset);
                        result.world_position = entity_transform.transform_point(result.local_position);
                        result.is_valid = true;
                        
                        std::cout << "Using bone-attached center of mass (bone: " 
                                  << config.bone_attachment.bone_name << ") for entity " 
                                  << entity_id << "\n";
                    } catch (...) {
                        // 骨骼查找失敗，降級到幾何中心
                        std::cerr << "Failed to find bone '" << config.bone_attachment.bone_name 
                                  << "' for entity " << entity_id << ", falling back to geometric center\n";
                        result.local_position = Vector3(0, 0, 0);
                        result.world_position = entity_transform.transform_point(result.local_position);
                        result.is_valid = true;
                    }
                } else {
                    // 沒有提供者，降級到幾何中心
                    result.local_position = Vector3(0, 0, 0);
                    result.world_position = entity_transform.transform_point(result.local_position);
                    result.is_valid = true;
                }
                break;
            }
            
            case CenterOfMassType::WEIGHTED_AVERAGE: {
                result.local_position = calculate_weighted_average(
                    config.weighted_points, entity_transform);
                result.world_position = entity_transform.transform_point(result.local_position);
                result.is_valid = true;
                std::cout << "Using weighted average center of mass (" 
                          << config.weighted_points.size() << " points) for entity " 
                          << entity_id << "\n";
                break;
            }
            
            case CenterOfMassType::PHYSICS_CENTER: {
                if (provider_ && config.consider_physics_mass) {
                    try {
                        std::vector<WeightedPoint> mass_points = 
                            provider_->get_mass_distribution(entity_id);
                        
                        if (!mass_points.empty()) {
                            result.local_position = calculate_weighted_average(
                                mass_points, entity_transform);
                            result.world_position = entity_transform.transform_point(result.local_position);
                            result.is_valid = true;
                            
                            std::cout << "Using physics-based center of mass (" 
                                      << mass_points.size() << " mass points) for entity " 
                                      << entity_id << "\n";
                        } else {
                            // 沒有質量分布數據，降級到幾何中心
                            result.local_position = Vector3(0, 0, 0);
                            result.world_position = entity_transform.transform_point(result.local_position);
                            result.is_valid = true;
                        }
                    } catch (...) {
                        // 物理質量獲取失敗，降級到幾何中心
                        result.local_position = Vector3(0, 0, 0);
                        result.world_position = entity_transform.transform_point(result.local_position);
                        result.is_valid = true;
                    }
                } else {
                    // 降級到幾何中心
                    result.local_position = Vector3(0, 0, 0);
                    result.world_position = entity_transform.transform_point(result.local_position);
                    result.is_valid = true;
                }
                break;
            }
            
            case CenterOfMassType::DYNAMIC_CALCULATED: {
                // 動態計算：結合多種因素
                if (provider_) {
                    try {
                        std::vector<WeightedPoint> mass_points = 
                            provider_->get_mass_distribution(entity_id);
                        
                        if (!mass_points.empty()) {
                            // 使用物理質量分布
                            result.local_position = calculate_weighted_average(
                                mass_points, entity_transform);
                        } else if (!config.weighted_points.empty()) {
                            // 降級到配置的加權點
                            result.local_position = calculate_weighted_average(
                                config.weighted_points, entity_transform);
                        } else {
                            // 最終降級到幾何中心
                            result.local_position = Vector3(0, 0, 0);
                        }
                        
                        result.world_position = entity_transform.transform_point(result.local_position);
                        result.is_valid = true;
                        
                        std::cout << "Using dynamic center of mass calculation for entity " 
                                  << entity_id << "\n";
                    } catch (...) {
                        // 動態計算失敗，降級到幾何中心
                        result.local_position = Vector3(0, 0, 0);
                        result.world_position = entity_transform.transform_point(result.local_position);
                        result.is_valid = true;
                    }
                } else {
                    result.local_position = Vector3(0, 0, 0);
                    result.world_position = entity_transform.transform_point(result.local_position);
                    result.is_valid = true;
                }
                break;
            }
        }
    } catch (...) {
        // 任何計算錯誤都降級到幾何中心
        std::cerr << "Error calculating center of mass for entity " << entity_id 
                  << ", using geometric center\n";
        result.local_position = Vector3(0, 0, 0);
        result.world_position = entity_transform.transform_point(result.local_position);
        result.is_valid = true;
    }
    
    return result;
}

Vector3 CenterOfMassManager::calculate_geometric_center(
    EntityId entity_id, 
    const Transform& entity_transform) {
    // 預設的幾何中心實現：假設在本地原點
    Vector3 local_center(0, 0, 0);
    return entity_transform.transform_point(local_center);
}

Vector3 CenterOfMassManager::calculate_weighted_average(
    const std::vector<WeightedPoint>& points, 
    const Transform& entity_transform) {
    
    if (points.empty()) {
        return Vector3(0, 0, 0);
    }
    
    Vector3 weighted_sum(0, 0, 0);
    float total_weight = 0.0f;
    
    for (const auto& point : points) {
        weighted_sum = weighted_sum + point.position * point.weight;
        total_weight += point.weight;
    }
    
    if (total_weight > 0.0001f) {
        return weighted_sum * (1.0f / total_weight);
    } else {
        // 所有權重為零，返回第一個點的位置
        return points[0].position;
    }
}

} // namespace Portal
