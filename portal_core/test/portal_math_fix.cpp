// Portal Math Bug Fix - 修復交集檢測邏輯
// 
// 問題：當前的 does_entity_intersect_portal 只檢查邊線是否與傳送門矩形相交
// 這會導致實體深入傳送門時被錯誤判斷為"不相交"
//
// 解決方案：改為檢查包圍盒是否與傳送門3D區域有任何重疊

#include "../lib/include/portal_math.h"

namespace Portal {
namespace Math {

bool PortalMath::does_entity_intersect_portal_improved(
    const Vector3& entity_bounds_min,
    const Vector3& entity_bounds_max,
    const Transform& entity_transform,
    const PortalPlane& portal_plane
) {
    // 獲取實體包圍盒的8個角點
    Vector3 corners[8] = {
        entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z)),
        entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z)),
        entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z)),
        entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z)),
        entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z)),
        entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z)),
        entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z)),
        entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z))
    };
    
    // 第一步：檢查是否有角點在傳送門平面兩側（基本相交測試）
    bool has_positive = false, has_negative = false;
    
    for (int i = 0; i < 8; ++i) {
        float distance = signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
        
        if (distance > EPSILON) {
            has_positive = true;
        } else if (distance < -EPSILON) {
            has_negative = true;
        }
    }
    
    // 如果所有頂點都在同一側，則不相交
    if (!has_positive || !has_negative) {
        return false;
    }
    
    // 第二步：檢查跨越平面的包圍盒是否與傳送門矩形區域有重疊
    // 將所有角點投影到傳送門平面上
    Vector3 projected_points[8];
    for (int i = 0; i < 8; ++i) {
        projected_points[i] = project_point_on_plane(corners[i], portal_plane.center, portal_plane.normal);
    }
    
    // 計算投影後點群的邊界框（在傳送門座標系中）
    float min_right = FLT_MAX, max_right = -FLT_MAX;
    float min_up = FLT_MAX, max_up = -FLT_MAX;
    
    for (int i = 0; i < 8; ++i) {
        Vector3 relative = projected_points[i] - portal_plane.center;
        float right_coord = relative.dot(portal_plane.right);
        float up_coord = relative.dot(portal_plane.up);
        
        min_right = std::min(min_right, right_coord);
        max_right = std::max(max_right, right_coord);
        min_up = std::min(min_up, up_coord);
        max_up = std::max(max_up, up_coord);
    }
    
    // 檢查投影邊界框是否與傳送門矩形重疊
    float portal_half_width = portal_plane.width * 0.5f;
    float portal_half_height = portal_plane.height * 0.5f;
    
    bool overlaps_width = (max_right >= -portal_half_width) && (min_right <= portal_half_width);
    bool overlaps_height = (max_up >= -portal_half_height) && (min_up <= portal_half_height);
    
    return overlaps_width && overlaps_height;
}

} // namespace Math
} // namespace Portal
