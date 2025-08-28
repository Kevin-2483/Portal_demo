#include <iostream>
#include <memory>
#define PORTAL_INCLUDE_EXAMPLES
#include "lib/include/portal.h"

using namespace Portal;

void debug_intersection_details(
    EntityId entity_id,
    const Example::ExamplePhysicsQuery* physics_query,
    const ::Portal::Portal* portal
) {
    Vector3 bounds_min, bounds_max;
    physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);
    Transform entity_transform = physics_query->get_entity_transform(entity_id);
    PortalPlane portal_plane = portal->get_plane();
    
    std::cout << "=== 詳細交集分析 ===\n";
    std::cout << "傳送門中心: (" << portal_plane.center.x << ", " << portal_plane.center.y << ", " << portal_plane.center.z << ")\n";
    std::cout << "傳送門尺寸: " << portal_plane.width << "x" << portal_plane.height << "\n";
    std::cout << "實體位置: (" << entity_transform.position.x << ", " << entity_transform.position.y << ", " << entity_transform.position.z << ")\n";
    
    // 手動執行分析
    Vector3 corners[8] = {
        entity_transform.transform_point(Vector3(bounds_min.x, bounds_min.y, bounds_min.z)),
        entity_transform.transform_point(Vector3(bounds_max.x, bounds_min.y, bounds_min.z)),
        entity_transform.transform_point(Vector3(bounds_min.x, bounds_max.y, bounds_min.z)),
        entity_transform.transform_point(Vector3(bounds_max.x, bounds_max.y, bounds_min.z)),
        entity_transform.transform_point(Vector3(bounds_min.x, bounds_min.y, bounds_max.z)),
        entity_transform.transform_point(Vector3(bounds_max.x, bounds_min.y, bounds_max.z)),
        entity_transform.transform_point(Vector3(bounds_min.x, bounds_max.y, bounds_max.z)),
        entity_transform.transform_point(Vector3(bounds_max.x, bounds_max.y, bounds_max.z))
    };
    
    std::cout << "8個角點的世界坐標:\n";
    for (int i = 0; i < 8; ++i) {
        std::cout << "  [" << i << "]: (" << corners[i].x << ", " << corners[i].y << ", " << corners[i].z << ")";
        float distance = Math::PortalMath::signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
        std::cout << " 距離平面: " << distance << "\n";
    }
    
    // 檢查有多少頂點在平面兩側
    int positive = 0, negative = 0, on_plane = 0;
    for (int i = 0; i < 8; ++i) {
        float distance = Math::PortalMath::signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
        if (distance > 0.001f) positive++;
        else if (distance < -0.001f) negative++;
        else on_plane++;
    }
    
    std::cout << "頂點分佈: 正面=" << positive << ", 背面=" << negative << ", 平面上=" << on_plane << "\n";
    
    if (positive > 0 && negative > 0) {
        std::cout << "包圍盒跨越平面，檢查投影重疊...\n";
        
        // 計算投影邊界
        float min_right = 1e6f, max_right = -1e6f;
        float min_up = 1e6f, max_up = -1e6f;
        
        for (int i = 0; i < 8; ++i) {
            // 手動計算投影
            Vector3 to_point = corners[i] - portal_plane.center;
            float distance = to_point.dot(portal_plane.normal);
            Vector3 projected = corners[i] - portal_plane.normal * distance;
            
            Vector3 relative = projected - portal_plane.center;
            float right_coord = relative.dot(portal_plane.right);
            float up_coord = relative.dot(portal_plane.up);
            
            min_right = std::min(min_right, right_coord);
            max_right = std::max(max_right, right_coord);
            min_up = std::min(min_up, up_coord);
            max_up = std::max(max_up, up_coord);
        }
        
        float portal_half_width = portal_plane.width * 0.5f;
        float portal_half_height = portal_plane.height * 0.5f;
        
        std::cout << "投影邊界: 水平[" << min_right << ", " << max_right << "], 垂直[" << min_up << ", " << max_up << "]\n";
        std::cout << "傳送門邊界: 水平[" << -portal_half_width << ", " << portal_half_width << "], 垂直[" << -portal_half_height << ", " << portal_half_height << "]\n";
        
        bool overlaps_width = (max_right >= -portal_half_width) && (min_right <= portal_half_width);
        bool overlaps_height = (max_up >= -portal_half_height) && (min_up <= portal_half_height);
        
        std::cout << "重疊檢查: 寬度=" << (overlaps_width ? "是" : "否") << ", 高度=" << (overlaps_height ? "是" : "否") << "\n";
    }
    
    std::cout << "========================\n\n";
}

int main() {
    try {
        std::cout << "=== 交集檢測深度調試 ===\n\n";
        
        // 創建系統
        auto physics_query = std::make_unique<Example::ExamplePhysicsQuery>();
        auto physics_manipulator = std::make_unique<Example::ExamplePhysicsManipulator>(physics_query.get());
        auto render_query = std::make_unique<Example::ExampleRenderQuery>();
        auto render_manipulator = std::make_unique<Example::ExampleRenderManipulator>();
        auto event_handler = std::make_unique<Example::ExampleEventHandler>();

        HostInterfaces interfaces;
        interfaces.physics_query = physics_query.get();
        interfaces.physics_manipulator = physics_manipulator.get();
        interfaces.render_query = render_query.get();
        interfaces.render_manipulator = render_manipulator.get();
        interfaces.event_handler = event_handler.get();

        auto portal_manager = std::make_unique<PortalManager>(interfaces);
        portal_manager->initialize();

        // 設置傳送門
        PortalPlane plane1;
        plane1.center = Vector3(-3, 0, 0);
        plane1.normal = Vector3(1, 0, 0);
        plane1.up = Vector3(0, 1, 0);
        plane1.right = Vector3(0, 0, 1);
        plane1.width = 2.0f;
        plane1.height = 3.0f;

        PortalId portal1_id = portal_manager->create_portal(plane1);
        const ::Portal::Portal* portal = portal_manager->get_portal(portal1_id);

        // 設置實體
        EntityId entity_id = 1001;
        Vector3 bounds_min(-0.5f, -1.0f, -0.5f);
        Vector3 bounds_max(0.5f, 1.0f, 0.5f);

        // 測試三個關鍵位置
        float test_positions[] = {-3.0f, -2.5f, -2.0f};
        for (int i = 0; i < 3; ++i) {
            std::cout << "測試位置 X=" << test_positions[i] << "\n";
            
            Transform transform;
            transform.position = Vector3(test_positions[i], 0, 0);
            physics_query->add_test_entity(entity_id, transform, bounds_min, bounds_max);
            
            debug_intersection_details(entity_id, physics_query.get(), portal);
            
            bool result = Math::PortalMath::does_entity_intersect_portal(
                bounds_min, bounds_max, transform, portal->get_plane()
            );
            std::cout << "最終結果: " << (result ? "相交" : "不相交") << "\n\n";
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "錯誤: " << e.what() << std::endl;
        return 1;
    }
}
