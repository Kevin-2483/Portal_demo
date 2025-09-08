#include "physics_event_adapter.h"
#include "../components/physics_body_component.h"
#include <iostream>
#include <cmath>

namespace portal_core {

PhysicsEventAdapter::PhysicsEventAdapter(EventManager& event_manager, 
                                       PhysicsWorldManager& physics_world, 
                                       entt::registry& registry)
    : event_manager_(event_manager)
    , physics_world_(physics_world)
    , registry_(registry) {
}

bool PhysicsEventAdapter::initialize() {
    if (initialized_) {
        return true;
    }

    if (!physics_world_.is_initialized()) {
        debug_log("PhysicsEventAdapter: PhysicsWorldManager not initialized");
        return false;
    }

    // 设置Jolt Physics回调 - 使用新的接触信息
    physics_world_.set_contact_added_callback([this](BodyID body1, BodyID body2, const Vec3& contact_point, const Vec3& contact_normal, float impulse_magnitude) {
        if (enabled_) {
            handle_contact_added_with_info(body1, body2, contact_point, contact_normal, impulse_magnitude);
        }
    });

    physics_world_.set_contact_removed_callback([this](BodyID body1, BodyID body2, const Vec3& contact_point, const Vec3& contact_normal, float impulse_magnitude) {
        if (enabled_) {
            handle_contact_removed(body1, body2);
        }
    });

    physics_world_.set_body_activated_callback([this](BodyID body_id, uint64 user_data) {
        if (enabled_) {
            handle_body_activated(body_id, user_data);
        }
    });

    physics_world_.set_body_deactivated_callback([this](BodyID body_id, uint64 user_data) {
        if (enabled_) {
            handle_body_deactivated(body_id, user_data);
        }
    });

    // 初始化BodyID到实体的映射
    update_body_entity_mapping();

    initialized_ = true;
    debug_log("PhysicsEventAdapter: Initialized successfully");
    return true;
}

void PhysicsEventAdapter::cleanup() {
    if (!initialized_) {
        return;
    }

    // 清理映射缓存
    body_to_entity_map_.clear();
    
    initialized_ = false;
    debug_log("PhysicsEventAdapter: Cleaned up");
}

void PhysicsEventAdapter::update(float delta_time) {
    if (!initialized_ || !enabled_) {
        return;
    }

    last_update_time_ = delta_time;

    // 更新BodyID到实体的映射缓存
    update_body_entity_mapping();

    // 处理懒加载查询
    process_pending_queries();

    // 处理平面相交检测（2D相交）
    process_plane_intersections(delta_time);

    // 处理区域监控
    process_area_monitoring(delta_time);

    // 处理持续碰撞
    process_persistent_contacts(delta_time);
}

// === Jolt Physics 回调处理 ===

void PhysicsEventAdapter::handle_contact_added_with_info(BodyID body1, BodyID body2, const Vec3& contact_point, const Vec3& contact_normal, float impulse_magnitude) {
    auto entity1 = body_id_to_entity(body1);
    auto entity2 = body_id_to_entity(body2);

    if (entity1 == entt::null || entity2 == entt::null) {
        return;
    }

    // 使用传入的接触信息创建ContactInfo
    ContactInfo contact_info;
    contact_info.point = {contact_point.GetX(), contact_point.GetY(), contact_point.GetZ()};
    contact_info.normal = {contact_normal.GetX(), contact_normal.GetY(), contact_normal.GetZ()};
    contact_info.impulse_magnitude = impulse_magnitude;

    // 检测相交类型（2D平面相交或3D空间相交）
    auto intersection_dimension = detect_intersection_dimension(contact_info.point, contact_info.normal);

    // 判断是否为传感器事件 - 使用安全方法检查
    bool is_sensor1 = is_body_sensor_safe(body1);
    bool is_sensor2 = is_body_sensor_safe(body2);

    if (is_sensor1 || is_sensor2) {
        // 触发器事件 - 立即处理
        entt::entity sensor_entity = is_sensor1 ? entity1 : entity2;
        entt::entity other_entity = is_sensor1 ? entity2 : entity1;
        
        dispatch_trigger_enter_event(sensor_entity, other_entity, contact_info);
        
        // 处理区域监控变化
        handle_area_monitoring_change(sensor_entity, other_entity, true);
    } else {
        // 普通碰撞事件 - 立即处理
        dispatch_collision_start_event(entity1, entity2, contact_info);
    }

    ++processed_collisions_count_;
}

void PhysicsEventAdapter::handle_contact_removed(BodyID body1, BodyID body2) {
    auto entity1 = body_id_to_entity(body1);
    auto entity2 = body_id_to_entity(body2);

    if (entity1 == entt::null || entity2 == entt::null) {
        return;
    }

    // 判断是否为传感器事件 - 使用安全方法检查
    bool is_sensor1 = is_body_sensor_safe(body1);
    bool is_sensor2 = is_body_sensor_safe(body2);

    if (is_sensor1 || is_sensor2) {
        // 触发器退出事件
        entt::entity sensor_entity = is_sensor1 ? entity1 : entity2;
        entt::entity other_entity = is_sensor1 ? entity2 : entity1;
        
        dispatch_trigger_exit_event(sensor_entity, other_entity);
        
        // 处理区域监控变化
        handle_area_monitoring_change(sensor_entity, other_entity, false);
    } else {
        // 普通碰撞结束事件
        dispatch_collision_end_event(entity1, entity2);
    }
}

void PhysicsEventAdapter::handle_body_activated(BodyID body_id, uint64 user_data) {
    auto entity = body_id_to_entity(body_id);
    if (entity == entt::null) {
        return;
    }

    auto position = physics_world_.get_body_position(body_id);
    auto dimension = PhysicsEventUtils::detect_dimension(Vec3(position.GetX(), position.GetY(), position.GetZ()));

    auto activation_event = BodyActivationEvent(entity, true, 
        Vec3(position.GetX(), position.GetY(), position.GetZ()), dimension);
    
    // 队列事件 - 批量处理
    event_manager_.enqueue(activation_event);
}

void PhysicsEventAdapter::handle_body_deactivated(BodyID body_id, uint64 user_data) {
    auto entity = body_id_to_entity(body_id);
    if (entity == entt::null) {
        return;
    }

    auto position = physics_world_.get_body_position(body_id);
    auto dimension = PhysicsEventUtils::detect_dimension(Vec3(position.GetX(), position.GetY(), position.GetZ()));

    auto activation_event = BodyActivationEvent(entity, false, 
        Vec3(position.GetX(), position.GetY(), position.GetZ()), dimension);
    
    // 队列事件 - 批量处理
    event_manager_.enqueue(activation_event);
}

// === 实体查找和映射 ===

entt::entity PhysicsEventAdapter::body_id_to_entity(BodyID body_id) {
    uint32_t id = body_id.GetIndexAndSequenceNumber();
    
    auto it = body_to_entity_map_.find(id);
    if (it != body_to_entity_map_.end()) {
        return it->second;
    }

    return entt::null;
}

void PhysicsEventAdapter::update_body_entity_mapping() {
    // 清空旧映射
    body_to_entity_map_.clear();

    // 重建映射 - 遍历所有有PhysicsBodyComponent的实体
    auto view = registry_.view<PhysicsBodyComponent>();
    for (auto entity : view) {
        auto& body_comp = view.get<PhysicsBodyComponent>(entity);
        if (body_comp.body_id.IsInvalid()) {
            continue;
        }
        
        uint32_t id = body_comp.body_id.GetIndexAndSequenceNumber();
        body_to_entity_map_[id] = entity;
    }
}

// === 碰撞和触发器检测 ===

PhysicsEventAdapter::ContactInfo PhysicsEventAdapter::get_contact_info(BodyID body1, BodyID body2) {
    // 注意：这个方法已经不推荐使用
    // 新的接触信息应该通过handle_contact_added_with_info方法从Jolt回调参数中获取
    // 这里只保留作为后备，实际使用中应该避免调用此方法
    
    ContactInfo info;
    info.point = Vec3(0, 0, 0);  // 默认接触点 
    info.normal = Vec3(0, 1, 0);  // 默认向上法线
    info.impulse_magnitude = 0.0f;  // 没有真实数据时使用0
    
    debug_log("Warning: get_contact_info called with default values. Use handle_contact_added_with_info instead.");
    
    return info;
}

bool PhysicsEventAdapter::is_sensor_body(BodyID body_id) {
    // 使用安全方法检查传感器
    return is_body_sensor_safe(body_id);
}

bool PhysicsEventAdapter::is_body_sensor_safe(BodyID body_id) {
    // 安全版本：通过实体组件来检查是否为传感器
    auto entity = body_id_to_entity(body_id);
    if (entity == entt::null) {
        return false;
    }
    
    // 检查实体是否有PhysicsBodyComponent并且标记为触发器
    if (registry_.all_of<PhysicsBodyComponent>(entity)) {
        auto& body_comp = registry_.get<PhysicsBodyComponent>(entity);
        // 检查物理体是否标记为触发器（传感器）
        return body_comp.is_trigger;
    }
    
    return false;
}

// === 事件分发辅助方法 ===

void PhysicsEventAdapter::dispatch_collision_start_event(entt::entity entity_a, entt::entity entity_b, 
                                                        const ContactInfo& contact) {
    auto dimension = detect_intersection_dimension(contact.point, contact.normal);
    
    auto collision_event = CollisionStartEvent(entity_a, entity_b, contact.point, contact.normal, 
                                             contact.impulse_magnitude, dimension);
    
    // 立即事件 - 同步处理
    event_manager_.publish_immediate(collision_event, EventMetadata{EventPriority::HIGH, 0.0f, true, "collision"});
}

void PhysicsEventAdapter::dispatch_collision_end_event(entt::entity entity_a, entt::entity entity_b) {
    auto collision_event = CollisionEndEvent(entity_a, entity_b);
    
    // 立即事件 - 同步处理  
    event_manager_.publish_immediate(collision_event, EventMetadata{EventPriority::NORMAL, 0.0f, true, "collision"});
}

void PhysicsEventAdapter::dispatch_trigger_enter_event(entt::entity sensor_entity, entt::entity other_entity, 
                                                      const ContactInfo& contact) {
    auto dimension = detect_intersection_dimension(contact.point, contact.normal);
    
    auto trigger_event = TriggerEnterEvent(sensor_entity, other_entity, contact.point, contact.normal, dimension);
    
    // 立即事件 - 同步处理
    event_manager_.publish_immediate(trigger_event, EventMetadata{EventPriority::HIGH, 0.0f, true, "trigger"});
}

void PhysicsEventAdapter::dispatch_trigger_exit_event(entt::entity sensor_entity, entt::entity other_entity) {
    auto trigger_event = TriggerExitEvent(sensor_entity, other_entity);
    
    // 立即事件 - 同步处理
    event_manager_.publish_immediate(trigger_event, EventMetadata{EventPriority::NORMAL, 0.0f, true, "trigger"});
}

// === 2D/3D 相交检测支持 ===

PhysicsEventDimension PhysicsEventAdapter::detect_intersection_dimension(const Vec3& contact_point, const Vec3& contact_normal) {
    // 检查是否为平面相交（2D类型）
    if (is_plane_intersection(contact_normal)) {
        return PhysicsEventDimension::DIMENSION_2D;  // 平面相交
    } else {
        return PhysicsEventDimension::DIMENSION_3D;  // 空间相交
    }
}

bool PhysicsEventAdapter::is_plane_intersection(const Vec3& contact_normal, float tolerance) {
    // 如果法线主要指向某个坐标轴，认为是平面相交
    float abs_x = std::abs(contact_normal.GetX());
    float abs_y = std::abs(contact_normal.GetY());
    float abs_z = std::abs(contact_normal.GetZ());
    
    // 检查是否有一个分量占主导地位（接近1.0）
    return (abs_x > (1.0f - tolerance) && abs_y < tolerance && abs_z < tolerance) ||
           (abs_y > (1.0f - tolerance) && abs_x < tolerance && abs_z < tolerance) ||
           (abs_z > (1.0f - tolerance) && abs_x < tolerance && abs_y < tolerance);
}

// === 懒加载查询处理 ===

void PhysicsEventAdapter::process_pending_queries() {
    auto pending_view = registry_.view<PendingQueryTag, PhysicsEventQueryComponent>();
    
    for (auto [entity, pending_tag, query_component] : pending_view.each()) {
        process_entity_queries(entity);
        
        // 移除待处理标记
        registry_.remove<PendingQueryTag>(entity);
        ++processed_queries_count_;
    }
}

void PhysicsEventAdapter::process_entity_queries(entt::entity entity) {
    auto& query_comp = registry_.get<PhysicsEventQueryComponent>(entity);
    
    if (!query_comp.has_pending_queries) {
        return;
    }

    // 执行射线查询
    if (!query_comp.raycast_queries.empty()) {
        execute_raycast_queries(entity, query_comp);
    }

    // 执行重叠查询
    if (!query_comp.overlap_queries.empty()) {
        execute_overlap_queries(entity, query_comp);
    }

    // 标记查询已处理
    query_comp.has_pending_queries = false;
}

void PhysicsEventAdapter::execute_raycast_queries(entt::entity entity, PhysicsEventQueryComponent& query_comp) {
    for (auto& raycast : query_comp.raycast_queries) {
        // 执行射线检测
        auto result = physics_world_.raycast(RVec3(raycast.origin.GetX(), raycast.origin.GetY(), raycast.origin.GetZ()),
                                           raycast.direction, raycast.max_distance);
        
        // 更新查询结果
        raycast.hit = result.hit;
        raycast.hit_point = result.hit_point;
        raycast.hit_normal = result.hit_normal;
        raycast.hit_distance = result.distance;
        raycast.hit_entity = body_id_to_entity(result.body_id);

        // 检测相交维度
        auto dimension = detect_intersection_dimension(raycast.hit_point, raycast.hit_normal);

        // 发送结果事件
        auto result_event = RaycastResultEvent(entity, raycast.hit, raycast.hit_point, raycast.hit_normal,
                                             raycast.hit_distance, raycast.hit_entity, dimension);
        
        // 队列事件 - 批量处理
        event_manager_.enqueue(result_event, EventMetadata{EventPriority::NORMAL, 0.0f, true, "raycast"});
    }
}

void PhysicsEventAdapter::execute_overlap_queries(entt::entity entity, PhysicsEventQueryComponent& query_comp) {
    for (auto& overlap : query_comp.overlap_queries) {
        std::vector<entt::entity> overlapping_entities;

        if (overlap.shape == PhysicsQueryComponent::OverlapQuery::SPHERE) {
            // 球体重叠查询
            auto bodies = physics_world_.overlap_sphere(
                RVec3(overlap.center.GetX(), overlap.center.GetY(), overlap.center.GetZ()), 
                overlap.size.GetX());  // 使用x分量作为半径
                
            for (auto body_id : bodies) {
                auto overlapped_entity = body_id_to_entity(body_id);
                if (overlapped_entity != entt::null && overlapped_entity != entity) {
                    overlapping_entities.push_back(overlapped_entity);
                }
            }
        } else if (overlap.shape == PhysicsQueryComponent::OverlapQuery::BOX) {
            // 盒体重叠查询
            auto bodies = physics_world_.overlap_box(
                RVec3(overlap.center.GetX(), overlap.center.GetY(), overlap.center.GetZ()), 
                overlap.size, overlap.rotation);
                
            for (auto body_id : bodies) {
                auto overlapped_entity = body_id_to_entity(body_id);
                if (overlapped_entity != entt::null && overlapped_entity != entity) {
                    overlapping_entities.push_back(overlapped_entity);
                }
            }
        }

        // 更新查询结果
        overlap.overlapping_entities = overlapping_entities;

        // 发送重叠查询结果事件
        auto result_event = OverlapQueryResultEvent(entity, overlap.center, overlap.size.GetX());
        result_event.overlapping_entities = std::move(overlapping_entities);
        
        // 队列事件 - 批量处理
        event_manager_.enqueue(result_event, EventMetadata{EventPriority::NORMAL, 0.0f, true, "overlap"});
    }
}

// === 平面相交检测处理（2D相交） ===

void PhysicsEventAdapter::process_plane_intersections(float delta_time) {
    auto plane_view = registry_.view<PlaneIntersectionComponent>();
    
    for (auto entity : plane_view) {
        auto& plane_comp = plane_view.get<PlaneIntersectionComponent>(entity);
        
        if (!plane_comp.active) {
            continue;
        }

        // 检查更新间隔
        plane_comp.last_check_time += delta_time;
        if (plane_comp.last_check_time < plane_comp.check_interval) {
            continue;
        }
        plane_comp.last_check_time = 0.0f;

        check_entity_plane_intersection(entity, plane_comp);
    }
}

void PhysicsEventAdapter::check_entity_plane_intersection(entt::entity entity, PlaneIntersectionComponent& plane_comp) {
    // 获取被监控实体的位置
    if (!registry_.all_of<PhysicsBodyComponent>(plane_comp.monitored_entity)) {
        return;
    }

    auto& body_comp = registry_.get<PhysicsBodyComponent>(plane_comp.monitored_entity);
    auto position = physics_world_.get_body_position(body_comp.body_id);
    Vec3 entity_pos(position.GetX(), position.GetY(), position.GetZ());

    // 检查当前是否在平面上方
    bool is_above_now = plane_comp.is_point_above(entity_pos);

    // 检查是否发生了平面穿越
    if (plane_comp.was_above != is_above_now && plane_comp.notify_on_cross) {
        // 发生了平面穿越
        auto intersection_event = CollisionStartEvent(entity, plane_comp.monitored_entity, 
                                                    entity_pos, plane_comp.plane_normal, 0.0f, 
                                                    PhysicsEventDimension::DIMENSION_2D);  // 明确标记为2D相交
        
        // 立即事件 - 平面穿越是重要事件
        event_manager_.publish_immediate(intersection_event, 
            EventMetadata{EventPriority::HIGH, 0.0f, true, "plane_intersection"});
    }

    // 更新状态
    plane_comp.was_above = is_above_now;
}

// === 区域监控处理 ===

void PhysicsEventAdapter::process_area_monitoring(float delta_time) {
    auto monitor_view = registry_.view<AreaMonitorComponent>();
    
    for (auto entity : monitor_view) {
        auto& monitor_comp = monitor_view.get<AreaMonitorComponent>(entity);
        
        if (!monitor_comp.active) {
            continue;
        }

        // 检查更新间隔
        monitor_comp.last_update_time += delta_time;
        if (monitor_comp.last_update_time < monitor_comp.update_interval) {
            continue;
        }
        monitor_comp.last_update_time = 0.0f;

        // 执行区域重叠查询
        auto bodies = physics_world_.overlap_sphere(
            RVec3(monitor_comp.center.GetX(), monitor_comp.center.GetY(), monitor_comp.center.GetZ()),
            monitor_comp.radius);

        // 更新区域内实体列表
        std::unordered_set<entt::entity> current_entities;
        for (auto body_id : bodies) {
            auto found_entity = body_id_to_entity(body_id);
            if (found_entity != entt::null && found_entity != entity) {
                current_entities.insert(found_entity);
            }
        }

        // 检查变化
        bool has_changes = (current_entities.size() != monitor_comp.entities_in_area.size());
        if (!has_changes) {
            for (auto ent : current_entities) {
                if (monitor_comp.entities_in_area.find(ent) == monitor_comp.entities_in_area.end()) {
                    has_changes = true;
                    break;
                }
            }
        }

        if (has_changes) {
            // 更新区域状态组件
            auto status_update = AreaStatusUpdateComponent(current_entities.size());
            status_update.previous_entity_count = monitor_comp.last_entity_count;
            status_update.entities_added = (current_entities.size() > monitor_comp.last_entity_count);
            status_update.entities_removed = (current_entities.size() < monitor_comp.last_entity_count);
            
            // 实体事件 - 持续状态管理
            event_manager_.add_component_event(entity, std::move(status_update));
            
            // 更新监控组件状态
            monitor_comp.entities_in_area = std::move(current_entities);
            monitor_comp.last_entity_count = monitor_comp.entities_in_area.size();
        }
    }
}

void PhysicsEventAdapter::handle_area_monitoring_change(entt::entity sensor_entity, entt::entity other_entity, bool entering) {
    // 检查传感器实体是否有区域监控组件
    if (registry_.all_of<AreaMonitorComponent>(sensor_entity)) {
        auto& monitor = registry_.get<AreaMonitorComponent>(sensor_entity);
        
        if (entering) {
            monitor.entities_in_area.insert(other_entity);
        } else {
            monitor.entities_in_area.erase(other_entity);
        }
        
        // 更新监控状态
        auto status_update = AreaStatusUpdateComponent(monitor.entities_in_area.size());
        status_update.entities_added = entering;
        status_update.entities_removed = !entering;
        
        event_manager_.add_component_event(sensor_entity, std::move(status_update));
    }
}

// === 持续碰撞处理 ===

void PhysicsEventAdapter::process_persistent_contacts(float delta_time) {
    auto contact_view = registry_.view<PersistentContactComponent>();
    
    for (auto entity : contact_view) {
        auto& contact_comp = contact_view.get<PersistentContactComponent>(entity);
        
        if (!contact_comp.active) {
            continue;
        }

        // 更新接触持续时间
        contact_comp.contact_duration += delta_time;

        // 检查是否达到阈值
        if (contact_comp.contact_duration >= contact_comp.duration_threshold && 
            contact_comp.notify_on_threshold) {
            
            // 发送持续接触事件
            auto persistent_event = CollisionStartEvent(entity, contact_comp.other_entity,
                                                      contact_comp.contact_point, contact_comp.contact_normal,
                                                      contact_comp.contact_force);
            persistent_event.collision_type = "persistent_contact";
            
            // 队列事件 - 持续接触不需要立即处理
            event_manager_.enqueue(persistent_event, 
                EventMetadata{EventPriority::LOW, 0.0f, true, "persistent_contact"});
            
            // 避免重复触发
            contact_comp.notify_on_threshold = false;
        }
    }
}

void PhysicsEventAdapter::debug_log(const std::string& message) {
    if (debug_mode_) {
        std::cout << "[PhysicsEventAdapter] " << message << std::endl;
    }
}

} // namespace portal_core
