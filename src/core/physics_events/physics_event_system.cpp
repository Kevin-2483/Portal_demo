#include "physics_event_system.h"
#include <iostream>

namespace portal_core {

PhysicsEventSystem::PhysicsEventSystem(EventManager& event_manager, 
                                     PhysicsWorldManager& physics_world, 
                                     entt::registry& registry)
    : event_manager_(event_manager)
    , physics_world_(physics_world)
    , registry_(registry) {
    
    // 创建子系统
    adapter_ = std::make_unique<PhysicsEventAdapter>(event_manager_, physics_world_, registry_);
    query_manager_ = std::make_unique<LazyPhysicsQueryManager>(event_manager_, physics_world_, registry_);
}

bool PhysicsEventSystem::initialize() {
    if (initialized_) {
        return true;
    }

    // 初始化事件适配器
    if (!adapter_->initialize()) {
        std::cerr << "PhysicsEventSystem: Failed to initialize event adapter" << std::endl;
        return false;
    }

    initialized_ = true;
    statistics_.system_initialized = true;
    
    if (debug_mode_) {
        std::cout << "PhysicsEventSystem: Initialized successfully" << std::endl;
    }
    
    return true;
}

void PhysicsEventSystem::cleanup() {
    if (!initialized_) {
        return;
    }

    adapter_->cleanup();
    
    initialized_ = false;
    statistics_.system_initialized = false;
    
    if (debug_mode_) {
        std::cout << "PhysicsEventSystem: Cleaned up" << std::endl;
    }
}

void PhysicsEventSystem::update(float delta_time) {
    if (!initialized_ || !enabled_) {
        return;
    }

    last_update_time_ = delta_time;
    statistics_.last_update_time = delta_time;

    // 更新事件适配器
    adapter_->update(delta_time);

    // 处理懒加载查询
    query_manager_->process_pending_queries(delta_time);

    // 更新统计信息
    update_statistics();
}

void PhysicsEventSystem::set_enabled(bool enabled) {
    enabled_ = enabled;
    statistics_.system_enabled = enabled;
    
    if (adapter_) {
        adapter_->set_enabled(enabled);
    }
    
    if (debug_mode_) {
        std::cout << "PhysicsEventSystem: " << (enabled ? "Enabled" : "Disabled") << std::endl;
    }
}

bool PhysicsEventSystem::is_enabled() const {
    return enabled_;
}

void PhysicsEventSystem::set_debug_mode(bool debug) {
    debug_mode_ = debug;
    
    if (adapter_) {
        adapter_->set_debug_mode(debug);
    }
    
    if (query_manager_) {
        query_manager_->set_debug_mode(debug);
    }
}

PhysicsEventSystem::SystemStatistics PhysicsEventSystem::get_statistics() const {
    update_statistics();
    return statistics_;
}

void PhysicsEventSystem::reset_statistics() {
    statistics_ = SystemStatistics{};
    statistics_.system_initialized = initialized_;
    statistics_.system_enabled = enabled_;
}

void PhysicsEventSystem::export_debug_info() const {
    auto stats = get_statistics();
    
    std::cout << "=== PhysicsEventSystem Debug Info ===" << std::endl;
    std::cout << "System Initialized: " << (stats.system_initialized ? "Yes" : "No") << std::endl;
    std::cout << "System Enabled: " << (stats.system_enabled ? "Yes" : "No") << std::endl;
    std::cout << "Processed Collisions: " << stats.processed_collisions << std::endl;
    std::cout << "Processed Queries: " << stats.processed_queries << std::endl;
    std::cout << "Active Area Monitors: " << stats.active_area_monitors << std::endl;
    std::cout << "Active Plane Intersections: " << stats.active_plane_intersections << std::endl;
    std::cout << "Last Update Time: " << stats.last_update_time << "s" << std::endl;
    
    // 获取查询管理器统计信息
    if (query_manager_) {
        auto query_stats = query_manager_->get_query_statistics();
        std::cout << "Pending Raycast Queries: " << query_stats.pending_raycast_queries << std::endl;
        std::cout << "Pending Overlap Queries: " << query_stats.pending_overlap_queries << std::endl;
    }
    
    std::cout << "=====================================" << std::endl;
}

void PhysicsEventSystem::update_statistics() const {
    // 获取查询管理器统计信息
    if (query_manager_) {
        auto query_stats = query_manager_->get_query_statistics();
        statistics_.active_area_monitors = query_stats.active_area_monitors;
        statistics_.active_plane_intersections = query_stats.active_plane_intersections;
        statistics_.processed_queries = query_stats.processed_queries_this_frame;
    }
}

} // namespace portal_core
