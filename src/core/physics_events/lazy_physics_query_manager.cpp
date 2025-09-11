#include "lazy_physics_query_manager.h"
#include "../components/physics_body_component.h"
#include <iostream>

namespace portal_core {

LazyPhysicsQueryManager::LazyPhysicsQueryManager(EventManager& event_manager, 
                                               PhysicsWorldManager& physics_world, 
                                               entt::registry& registry)
    : event_manager_(event_manager)
    , physics_world_(physics_world)
    , registry_(registry) {
}

// === 懒加载射线检测 ===

void LazyPhysicsQueryManager::request_raycast(entt::entity requester, const Vec3& origin, const Vec3& direction, 
                                             float max_distance, uint32_t layer_mask) {
    // 确保实体有必要的组件
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    // 添加射线查询
    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_raycast(origin, direction, max_distance, layer_mask);

    debug_log("Requested raycast for entity " + std::to_string(static_cast<uint32_t>(requester)) + 
              " from (" + std::to_string(origin.GetX()) + ", " + std::to_string(origin.GetY()) + ", " + std::to_string(origin.GetZ()) + ")");
}

void LazyPhysicsQueryManager::request_multiple_raycasts(entt::entity requester, 
                                                       const std::vector<std::tuple<Vec3, Vec3, float>>& raycast_params) {
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    
    for (const auto& [origin, direction, max_distance] : raycast_params) {
        query_comp.add_raycast(origin, direction, max_distance);
    }

    debug_log("Requested " + std::to_string(raycast_params.size()) + " raycasts for entity " + 
              std::to_string(static_cast<uint32_t>(requester)));
}

// === 懒加载区域监控 ===

void LazyPhysicsQueryManager::request_area_monitoring(entt::entity requester, const Vec3& center, float radius, 
                                                     uint32_t layer_mask, float update_interval) {
    // 按需创建区域监控组件
    if (!registry_.all_of<AreaMonitorComponent>(requester)) {
        auto dimension = detect_query_dimension(center);
        registry_.emplace<AreaMonitorComponent>(requester, center, radius, dimension);
    }

    // 确保有查询组件
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    // 添加球体重叠查询
    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_sphere_overlap(center, radius, layer_mask);

    // 配置区域监控
    auto& monitor_comp = registry_.get<AreaMonitorComponent>(requester);
    monitor_comp.center = center;
    monitor_comp.radius = radius;
    monitor_comp.layer_mask = layer_mask;
    monitor_comp.update_interval = update_interval;
    monitor_comp.active = true;

    debug_log("Requested area monitoring for entity " + std::to_string(static_cast<uint32_t>(requester)) + 
              " at (" + std::to_string(center.GetX()) + ", " + std::to_string(center.GetY()) + ", " + std::to_string(center.GetZ()) + 
              ") with radius " + std::to_string(radius));
}

void LazyPhysicsQueryManager::request_box_area_monitoring(entt::entity requester, const Vector3& center, const Vector3& half_extents,
                                                        const Quaternion& rotation, uint32_t layer_mask) {
    // 按需创建包含检测组件（用于矩形区域）
    if (!registry_.all_of<ContainmentComponent>(requester)) {
        auto dimension = detect_query_dimension(center);
        Vec3 min_bounds = center - half_extents;
        Vec3 max_bounds = center + half_extents;
        registry_.emplace<ContainmentComponent>(requester, min_bounds, max_bounds, dimension);
    }

    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    // 添加盒体重叠查询
    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_box_overlap(center, half_extents, rotation, layer_mask);

    debug_log("Requested box area monitoring for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

// === 懒加载平面相交检测（2D相交） ===

void LazyPhysicsQueryManager::request_plane_intersection_monitoring(entt::entity requester, entt::entity target_entity,
                                                                   const Vec3& plane_normal, float plane_distance,
                                                                   float check_interval) {
    // 按需创建平面相交组件
    if (!registry_.all_of<PlaneIntersectionComponent>(requester)) {
        registry_.emplace<PlaneIntersectionComponent>(requester, plane_normal, plane_distance, target_entity, 
                                                    PhysicsEventDimension::DIMENSION_2D);  // 明确标记为2D相交
    }

    // 配置平面相交检测
    auto& plane_comp = registry_.get<PlaneIntersectionComponent>(requester);
    plane_comp.plane_normal = plane_normal;
    plane_comp.plane_distance = plane_distance;
    plane_comp.monitored_entity = target_entity;
    plane_comp.check_interval = check_interval;
    plane_comp.active = true;
    plane_comp.dimension = PhysicsEventDimension::DIMENSION_2D;  // 平面相交是2D类型

    debug_log("Requested plane intersection monitoring for entity " + std::to_string(static_cast<uint32_t>(requester)) + 
              " targeting entity " + std::to_string(static_cast<uint32_t>(target_entity)));
}

void LazyPhysicsQueryManager::request_water_surface_detection(entt::entity requester, entt::entity target_entity, 
                                                            float water_level) {
    // 水面检测是特殊的水平平面相交
    Vec3 up_normal(0, 1, 0);  // 向上的法线
    request_plane_intersection_monitoring(requester, target_entity, up_normal, water_level, 0.016f);  // 每帧检测

    debug_log("Requested water surface detection for entity " + std::to_string(static_cast<uint32_t>(requester)) + 
              " at water level " + std::to_string(water_level));
}

void LazyPhysicsQueryManager::request_ground_detection(entt::entity requester, entt::entity target_entity) {
    // 地面检测是向下的射线检测
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    // 获取目标实体位置作为射线起点
    if (registry_.all_of<portal_core::PhysicsBodyComponent>(target_entity)) {
        auto& body_comp = registry_.get<portal_core::PhysicsBodyComponent>(target_entity);
        auto position = physics_world_.get_body_position(body_comp.body_id);
        
        Vec3 origin(position.GetX(), position.GetY(), position.GetZ());
        Vec3 down_direction(0, -1, 0);  // 向下的方向
        
        request_raycast(requester, origin, down_direction, 10.0f);  // 检测10米内的地面
    }

    debug_log("Requested ground detection for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

// === 懒加载距离查询 ===

void LazyPhysicsQueryManager::request_nearest_entity_query(entt::entity requester, const Vec3& center, 
                                                          float max_distance, uint32_t layer_mask) {
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_distance_query(center, max_distance, layer_mask);

    debug_log("Requested nearest entity query for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

void LazyPhysicsQueryManager::request_distance_range_query(entt::entity requester, const Vec3& center, 
                                                          float min_distance, float max_distance, uint32_t layer_mask) {
    // 使用球体重叠查询来实现距离范围查询
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    
    // 添加两个球体查询：一个最大范围，一个最小范围
    query_comp.add_sphere_overlap(center, max_distance, layer_mask);
    if (min_distance > 0.0f) {
        query_comp.add_sphere_overlap(center, min_distance, layer_mask);
    }

    debug_log("Requested distance range query for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

// === 懒加载形状查询 ===

void LazyPhysicsQueryManager::request_sphere_overlap_query(entt::entity requester, const Vec3& center, float radius,
                                                          uint32_t layer_mask) {
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_sphere_overlap(center, radius, layer_mask);

    debug_log("Requested sphere overlap query for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

void LazyPhysicsQueryManager::request_box_overlap_query(entt::entity requester, const Vector3& center, const Vector3& half_extents,
                                                       const Quaternion& rotation, uint32_t layer_mask) {
    ensure_query_component(requester);
    ensure_pending_query_tag(requester);

    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(requester);
    query_comp.add_box_overlap(center, half_extents, rotation, layer_mask);

    debug_log("Requested box overlap query for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

// === 高级查询功能 ===

void LazyPhysicsQueryManager::request_persistent_contact_monitoring(entt::entity requester, entt::entity other_entity,
                                                                   float duration_threshold, float force_threshold) {
    // 按需创建持续接触组件
    if (!registry_.all_of<PersistentContactComponent>(requester)) {
        auto dimension = PhysicsEventDimension::DIMENSION_3D;  // 持续接触通常是3D相交
        registry_.emplace<PersistentContactComponent>(requester, other_entity, Vec3::sZero(), Vec3::sZero(), dimension);
    }

    auto& contact_comp = registry_.get<PersistentContactComponent>(requester);
    contact_comp.other_entity = other_entity;
    contact_comp.duration_threshold = duration_threshold;
    contact_comp.force_threshold = force_threshold;
    contact_comp.active = true;

    debug_log("Requested persistent contact monitoring for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

void LazyPhysicsQueryManager::request_containment_detection(entt::entity requester, const Vec3& bounds_min, const Vec3& bounds_max,
                                                           float check_interval) {
    // 按需创建包含检测组件
    if (!registry_.all_of<ContainmentComponent>(requester)) {
        auto dimension = detect_query_dimension((bounds_min + bounds_max) * 0.5f);
        registry_.emplace<ContainmentComponent>(requester, bounds_min, bounds_max, dimension);
    }

    auto& containment_comp = registry_.get<ContainmentComponent>(requester);
    containment_comp.bounds_min = bounds_min;
    containment_comp.bounds_max = bounds_max;
    containment_comp.check_interval = check_interval;
    containment_comp.active = true;

    debug_log("Requested containment detection for entity " + std::to_string(static_cast<uint32_t>(requester)));
}

// === 查询管理 ===

void LazyPhysicsQueryManager::process_pending_queries(float delta_time) {
    update_statistics();
    
    // 这个方法由PhysicsEventAdapter调用，实际处理逻辑在适配器中
    // 这里主要更新统计信息
}

void LazyPhysicsQueryManager::cancel_entity_queries(entt::entity entity) {
    if (registry_.all_of<PhysicsEventQueryComponent>(entity)) {
        auto& query_comp = registry_.get<PhysicsEventQueryComponent>(entity);
        query_comp.clear_queries();
    }

    // 移除相关组件
    registry_.remove<PendingQueryTag>(entity);
    
    debug_log("Cancelled all queries for entity " + std::to_string(static_cast<uint32_t>(entity)));
}

void LazyPhysicsQueryManager::cancel_raycast_queries(entt::entity entity) {
    if (registry_.all_of<PhysicsEventQueryComponent>(entity)) {
        auto& query_comp = registry_.get<PhysicsEventQueryComponent>(entity);
        query_comp.raycast_queries.clear();
    }
}

void LazyPhysicsQueryManager::cancel_area_monitoring(entt::entity entity) {
    registry_.remove<AreaMonitorComponent>(entity);
    
    if (registry_.all_of<PhysicsEventQueryComponent>(entity)) {
        auto& query_comp = registry_.get<PhysicsEventQueryComponent>(entity);
        query_comp.overlap_queries.clear();
    }
}

void LazyPhysicsQueryManager::cancel_plane_intersection_monitoring(entt::entity entity) {
    registry_.remove<PlaneIntersectionComponent>(entity);
}

// === 配置和统计 ===

LazyPhysicsQueryManager::QueryStatistics LazyPhysicsQueryManager::get_query_statistics() const {
    update_statistics();
    return statistics_;
}

// === 内部辅助方法 ===

void LazyPhysicsQueryManager::ensure_query_component(entt::entity entity) {
    if (!registry_.all_of<PhysicsEventQueryComponent>(entity)) {
        registry_.emplace<PhysicsEventQueryComponent>(entity);
    }
}

void LazyPhysicsQueryManager::ensure_pending_query_tag(entt::entity entity, int priority) {
    if (!registry_.all_of<PendingQueryTag>(entity)) {
        registry_.emplace<PendingQueryTag>(entity, priority);
    }
}

PhysicsEventDimension LazyPhysicsQueryManager::detect_query_dimension(const Vec3& position, const Vec3& direction) {
    // 如果方向主要是沿某个轴的，可能是2D查询
    if (direction != Vec3::sZero()) {
        float abs_x = std::abs(direction.GetX());
        float abs_y = std::abs(direction.GetY());
        float abs_z = std::abs(direction.GetZ());
        
        // 如果方向主要沿单一轴，认为是2D查询
        if ((abs_y > 0.9f && abs_x < 0.1f && abs_z < 0.1f) ||  // 主要沿Y轴
            (abs_x > 0.9f && abs_y < 0.1f && abs_z < 0.1f) ||  // 主要沿X轴  
            (abs_z > 0.9f && abs_x < 0.1f && abs_y < 0.1f)) {  // 主要沿Z轴
            return PhysicsEventDimension::DIMENSION_2D;
        }
    }
    
    // 如果Z坐标接近0，可能是2D查询
    if (std::abs(position.GetZ()) < 0.001f) {
        return PhysicsEventDimension::DIMENSION_2D;
    }
    
    return PhysicsEventDimension::DIMENSION_3D;
}

void LazyPhysicsQueryManager::debug_log(const std::string& message) const {
    if (debug_mode_) {
        std::cout << "[LazyPhysicsQueryManager] " << message << std::endl;
    }
}

void LazyPhysicsQueryManager::update_statistics() const {
    // 统计待处理的查询
    statistics_.pending_raycast_queries = 0;
    statistics_.pending_overlap_queries = 0;
    statistics_.active_area_monitors = 0;
    statistics_.active_plane_intersections = 0;

    auto query_view = registry_.view<PhysicsEventQueryComponent>();
    for (auto entity : query_view) {
        auto& query_comp = query_view.get<PhysicsEventQueryComponent>(entity);
        statistics_.pending_raycast_queries += query_comp.raycast_queries.size();
        statistics_.pending_overlap_queries += query_comp.overlap_queries.size();
    }

    auto monitor_view = registry_.view<AreaMonitorComponent>();
    for (auto entity : monitor_view) {
        auto& monitor_comp = monitor_view.get<AreaMonitorComponent>(entity);
        if (monitor_comp.active) {
            ++statistics_.active_area_monitors;
        }
    }

    auto plane_view = registry_.view<PlaneIntersectionComponent>();
    for (auto entity : plane_view) {
        auto& plane_comp = plane_view.get<PlaneIntersectionComponent>(entity);
        if (plane_comp.active) {
            ++statistics_.active_plane_intersections;
        }
    }
}

} // namespace portal_core
