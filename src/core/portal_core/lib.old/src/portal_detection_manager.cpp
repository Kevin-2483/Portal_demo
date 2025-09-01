#include "portal_detection_manager.h"
#include "../include/portal_core.h"
#include "../include/portal_math.h"

namespace PortalCore {

PortalDetectionManager::PortalDetectionManager(IPhysicsDataProvider* provider, 
                                              IPortalDetectionOverride* override)
    : data_provider_(provider), override_(override) {
    // 數據提供者是必須的
    if (!data_provider_) {
        throw std::invalid_argument("Physics data provider cannot be null");
    }
}

bool PortalDetectionManager::check_center_crossing(EntityId entity, const ::Portal::Portal& portal) {
    if (!is_initialized()) {
        return false;
    }
    
    // 優先使用引擎的重載實現
    bool crossed = false;
    if (override_ && override_->override_center_crossing_check(entity, portal, crossed)) {
        return crossed;  // 引擎已處理
    }
    
    // 使用Portal庫的預設實現
    return default_center_crossing_check(entity, portal);
}

BoundingBoxAnalysis PortalDetectionManager::analyze_entity_bounding_box(EntityId entity, const ::Portal::Portal& portal) {
    if (!is_initialized()) {
        BoundingBoxAnalysis analysis;
        analysis.front_vertices_count = 0;
        analysis.back_vertices_count = 0;
        return analysis;
    }
    
    BoundingBoxAnalysis analysis;
    
    // 優先使用引擎的重載實現
    if (override_ && override_->override_bounding_box_analysis(entity, portal, analysis)) {
        return analysis;  // 引擎已處理
    }
    
    // 使用Portal庫的預設實現
    return default_bounding_box_analysis(entity, portal);
}

std::vector<EntityId> PortalDetectionManager::get_intersecting_entities(const ::Portal::Portal& portal) {
    if (!is_initialized()) {
        return {};
    }
    
    std::vector<EntityId> entities;
    
    // 優先使用引擎的重載實現
    if (override_ && override_->override_intersection_query(portal, entities)) {
        return entities;  // 引擎已處理
    }
    
    // 使用Portal庫的預設實現（可能需要遍歷所有實體，效率較低）
    return default_intersection_query(portal);
}

float PortalDetectionManager::calculate_crossing_progress(EntityId entity, const ::Portal::Portal& portal) {
    if (!is_initialized()) {
        return 0.0f;
    }
    
    float progress = 0.0f;
    
    // 優先使用引擎的重載實現
    if (override_ && override_->override_crossing_progress_calculation(entity, portal, progress)) {
        return progress;  // 引擎已處理
    }
    
    // 使用Portal庫的預設實現
    return default_crossing_progress_calculation(entity, portal);
}

void PortalDetectionManager::set_data_provider(IPhysicsDataProvider* provider) {
    data_provider_ = provider;
}

void PortalDetectionManager::set_detection_override(IPortalDetectionOverride* override) {
    override_ = override;
}

bool PortalDetectionManager::is_initialized() const {
    return data_provider_ != nullptr;
}

// === Portal庫內建檢測方法實現 ===

bool PortalDetectionManager::default_center_crossing_check(EntityId entity, const ::Portal::Portal& portal) {
    // 獲取實體的質心位置
    Vector3 center = data_provider_->get_entity_center_of_mass(entity);
    
    // 使用Portal庫現有的數學函數計算穿越
    return Portal::Math::PortalMath::signed_distance_to_plane(
        center, portal.get_plane().center, portal.get_plane().normal) < 0.0f;
}

BoundingBoxAnalysis PortalDetectionManager::default_bounding_box_analysis(EntityId entity, const ::Portal::Portal& portal) {
    // 獲取實體變換和包圍盒
    Transform transform = data_provider_->get_entity_transform(entity);
    BoundingBox bbox = data_provider_->get_entity_bounding_box(entity);
    
    // 簡單的包圍盒分析實現
    BoundingBoxAnalysis analysis;
    analysis.front_vertices_count = 0;
    analysis.back_vertices_count = 0;
    
    // 檢查包圍盒的8個頂點
    Vector3 corners[8];
    // 計算世界坐標系的包圍盒頂點
    const Vector3& min = bbox.min;
    const Vector3& max = bbox.max;
    
    corners[0] = transform.transform_point(Vector3(min.x, min.y, min.z));
    corners[1] = transform.transform_point(Vector3(max.x, min.y, min.z));
    corners[2] = transform.transform_point(Vector3(min.x, max.y, min.z));
    corners[3] = transform.transform_point(Vector3(max.x, max.y, min.z));
    corners[4] = transform.transform_point(Vector3(min.x, min.y, max.z));
    corners[5] = transform.transform_point(Vector3(max.x, min.y, max.z));
    corners[6] = transform.transform_point(Vector3(min.x, max.y, max.z));
    corners[7] = transform.transform_point(Vector3(max.x, max.y, max.z));
    
    // 檢查每個頂點在傳送門平面的哪一側
    for (int i = 0; i < 8; i++) {
        float distance = Portal::Math::PortalMath::signed_distance_to_plane(
            corners[i], portal.get_plane().center, portal.get_plane().normal);
        
        if (distance > 0.0f) {
            analysis.front_vertices_count++;
        } else {
            analysis.back_vertices_count++;
        }
    }
    
    // 計算穿越比例
    analysis.crossing_ratio = (analysis.front_vertices_count > 0 && analysis.back_vertices_count > 0) ? 
                             (float)analysis.back_vertices_count / 8.0f : 0.0f;
    
    return analysis;
}

std::vector<EntityId> PortalDetectionManager::default_intersection_query(const ::Portal::Portal& portal) {
    std::vector<EntityId> result;
    
    // 獲取所有活動實體
    std::vector<EntityId> all_entities = data_provider_->get_all_active_entities();
    
    // 遍歷檢查每個實體是否與傳送門相交
    for (EntityId entity : all_entities) {
        BoundingBoxAnalysis analysis = default_bounding_box_analysis(entity, portal);
        
        // 如果實體的包圍盒與傳送門有相交，則加入結果
        if (analysis.front_vertices_count > 0 && analysis.back_vertices_count > 0) {
            result.push_back(entity);
        }
    }
    
    return result;
}

float PortalDetectionManager::default_crossing_progress_calculation(EntityId entity, const ::Portal::Portal& portal) {
    // 獲取實體的質心位置
    Vector3 center = data_provider_->get_entity_center_of_mass(entity);
    
    // 計算質心到傳送門平面的距離
    float distance = Portal::Math::PortalMath::signed_distance_to_plane(
        center, portal.get_plane().center, portal.get_plane().normal);
    
    // 簡單的進度計算：基於距離的線性映射
    // 這裡假設進度範圍為 [-1, 1] 映射到 [0, 1]
    float progress = 0.5f + distance * 0.5f;
    
    // 限制在 [0, 1] 範圍內
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    return progress;
}

} // namespace PortalCore
