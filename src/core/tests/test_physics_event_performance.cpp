#include "core/physics_events/physics_event_system.h"
#include "core/physics_events/physics_events.h"
#include "core/event_manager.h"
#include "core/physics_world_manager.h"
#include "core/components/physics_body_component.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <vector>

using namespace portal_core;

/**
 * 物理事件系统性能测试
 * 测试大量物理实体和查询的性能表现
 */
class PhysicsEventPerformanceTest {
public:
    PhysicsEventPerformanceTest() : event_manager_(registry_) {}

    void run_performance_tests() {
        std::cout << "=== Physics Event System Performance Tests ===" << std::endl;
        
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return;
        }

        // 运行性能测试
        test_many_collisions();
        test_many_raycast_queries();
        test_many_area_monitors();
        test_lazy_loading_performance();

        cleanup_systems();
        std::cout << "✅ Performance tests completed" << std::endl;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventSystem> physics_event_system_;

    // 临时事件计数器类，用于性能测试
    struct EventCounter {
        int collision_events = 0;
        int raycast_events = 0;
        int overlap_events = 0;

        void handle_collision(const CollisionStartEvent&) { collision_events++; }
        void handle_raycast(const RaycastResultEvent&) { raycast_events++; }
        void handle_overlap(const OverlapQueryResultEvent&) { overlap_events++; }
    };

    bool initialize_systems() {
        physics_world_ = std::make_unique<PhysicsWorldManager>();
        if (!physics_world_->initialize()) {
            return false;
        }

        physics_event_system_ = std::make_unique<PhysicsEventSystem>(
            event_manager_, *physics_world_, registry_);
        
        if (!physics_event_system_->initialize()) {
            return false;
        }

        // 关闭调试模式以提高性能
        physics_event_system_->set_debug_mode(false);
        
        return true;
    }

    void cleanup_systems() {
        if (physics_event_system_) {
            physics_event_system_->cleanup();
        }
        if (physics_world_) {
            physics_world_->cleanup();
        }
    }

    void test_many_collisions() {
        std::cout << "\n⚡ Testing many collisions performance..." << std::endl;
        
        const int entity_count = 100;
        std::vector<entt::entity> entities;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建大量物理实体
        for (int i = 0; i < entity_count; ++i) {
            float x = (i % 10) * 2.0f;
            float z = (i / 10) * 2.0f;
            auto entity = create_test_entity(JPH::Vec3(x, 10 + i * 0.1f, z), PhysicsBodyType::DYNAMIC);
            entities.push_back(entity);
        }
        
        // 创建地面
        create_test_entity(JPH::Vec3(10, -1, 10), PhysicsBodyType::STATIC);
        
        auto creation_time = std::chrono::high_resolution_clock::now();
        auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(creation_time - start_time);
        
        std::cout << "📊 Created " << entity_count << " entities in " << creation_duration.count() << "ms" << std::endl;
        
        // 模拟物理更新
        EventCounter counter;
        auto collision_sink = event_manager_.subscribe<CollisionStartEvent>();
        auto connection = collision_sink.connect<&EventCounter::handle_collision>(counter);
        
        auto sim_start = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 60; ++frame) {  // 1秒的模拟
            physics_world_->update(1.0f / 60.0f);
            physics_event_system_->update(1.0f / 60.0f);
            event_manager_.process_queued_events(1.0f / 60.0f);
        }
        
        auto sim_end = std::chrono::high_resolution_clock::now();
        auto sim_duration = std::chrono::duration_cast<std::chrono::milliseconds>(sim_end - sim_start);
        
        std::cout << "📊 Simulated 60 frames in " << sim_duration.count() << "ms" << std::endl;
        std::cout << "📊 Generated " << counter.collision_events << " collision events" << std::endl;
        std::cout << "📊 Average frame time: " << sim_duration.count() / 60.0f << "ms" << std::endl;
    }

    void test_many_raycast_queries() {
        std::cout << "\n⚡ Testing many raycast queries performance..." << std::endl;
        
        const int query_count = 200;
        std::vector<entt::entity> query_entities;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建大量射线查询
        for (int i = 0; i < query_count; ++i) {
            auto entity = registry_.create();
            query_entities.push_back(entity);
            
            JPH::Vec3 origin(i * 0.5f, 5, 0);
            JPH::Vec3 direction(0, -1, 0);
            physics_event_system_->request_raycast(entity, origin, direction, 10.0f);
        }
        
        auto query_creation_time = std::chrono::high_resolution_clock::now();
        auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(query_creation_time - start_time);
        
        std::cout << "📊 Created " << query_count << " raycast queries in " << creation_duration.count() << "ms" << std::endl;
        
        // 处理查询
        EventCounter raycast_counter;
        auto raycast_sink = event_manager_.subscribe<RaycastResultEvent>();
        auto raycast_connection = raycast_sink.connect<&EventCounter::handle_raycast>(raycast_counter);
        
        auto process_start = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 10; ++frame) {
            physics_event_system_->update(1.0f / 60.0f);
            event_manager_.process_queued_events(1.0f / 60.0f);
        }
        
        auto process_end = std::chrono::high_resolution_clock::now();
        auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(process_end - process_start);
        
        std::cout << "📊 Processed queries in " << process_duration.count() << "ms" << std::endl;
        std::cout << "📊 Generated " << raycast_counter.raycast_events << " raycast result events" << std::endl;
    }

    void test_many_area_monitors() {
        std::cout << "\n⚡ Testing many area monitors performance..." << std::endl;
        
        const int monitor_count = 50;
        std::vector<entt::entity> monitors;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建大量区域监控
        for (int i = 0; i < monitor_count; ++i) {
            auto entity = registry_.create();
            monitors.push_back(entity);
            
            JPH::Vec3 center(i * 3.0f, 0, 0);
            physics_event_system_->request_area_monitoring(entity, center, 2.0f);
        }
        
        auto creation_time = std::chrono::high_resolution_clock::now();
        auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(creation_time - start_time);
        
        std::cout << "📊 Created " << monitor_count << " area monitors in " << creation_duration.count() << "ms" << std::endl;
        
        // 创建一些移动实体
        for (int i = 0; i < 20; ++i) {
            auto entity = create_test_entity(JPH::Vec3(i * 1.5f, 0, 0), PhysicsBodyType::DYNAMIC);
            physics_world_->set_body_linear_velocity(
                registry_.get<PhysicsBodyComponent>(entity).body_id, 
                JPH::Vec3(1, 0, 0));
        }
        
        // 处理监控
        EventCounter overlap_counter;
        auto overlap_sink = event_manager_.subscribe<OverlapQueryResultEvent>();
        auto overlap_connection = overlap_sink.connect<&EventCounter::handle_overlap>(overlap_counter);
        
        auto process_start = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 30; ++frame) {
            physics_world_->update(1.0f / 60.0f);
            physics_event_system_->update(1.0f / 60.0f);
            event_manager_.process_queued_events(1.0f / 60.0f);
        }
        
        auto process_end = std::chrono::high_resolution_clock::now();
        auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(process_end - process_start);
        
        std::cout << "📊 Processed area monitoring in " << process_duration.count() << "ms" << std::endl;
        std::cout << "📊 Generated " << overlap_counter.overlap_events << " overlap events" << std::endl;
    }

    void test_lazy_loading_performance() {
        std::cout << "\n⚡ Testing lazy loading performance..." << std::endl;
        
        const int entity_count = 1000;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建大量实体，但不立即添加物理组件
        std::vector<entt::entity> entities;
        for (int i = 0; i < entity_count; ++i) {
            entities.push_back(registry_.create());
        }
        
        auto creation_time = std::chrono::high_resolution_clock::now();
        auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(creation_time - start_time);
        
        std::cout << "📊 Created " << entity_count << " entities in " << creation_duration.count() << "ms" << std::endl;
        
        // 懒加载请求查询
        auto lazy_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < entity_count; ++i) {
            if (i % 4 == 0) {  // 25% 请求射线查询
                physics_event_system_->request_raycast(entities[i], JPH::Vec3(i, 0, 0), JPH::Vec3(0, -1, 0));
            } else if (i % 4 == 1) {  // 25% 请求区域监控
                physics_event_system_->request_area_monitoring(entities[i], JPH::Vec3(i, 0, 0), 1.0f);
            }
            // 其他50%保持无组件状态
        }
        
        auto lazy_end = std::chrono::high_resolution_clock::now();
        auto lazy_duration = std::chrono::duration_cast<std::chrono::milliseconds>(lazy_end - lazy_start);
        
        std::cout << "📊 Lazy loaded components in " << lazy_duration.count() << "ms" << std::endl;
        
        // 统计实际创建的组件数量
        int query_components = 0;
        int pending_tags = 0;
        int area_monitors = 0;
        
        auto query_view = registry_.view<PhysicsEventQueryComponent>();
        query_components = query_view.size();
        
        auto pending_view = registry_.view<PendingQueryTag>();
        pending_tags = pending_view.size();
        
        auto monitor_view = registry_.view<AreaMonitorComponent>();
        area_monitors = monitor_view.size();
        
        std::cout << "📊 Created components: Query=" << query_components 
                  << ", PendingTags=" << pending_tags 
                  << ", AreaMonitors=" << area_monitors << std::endl;
        
        // 验证懒加载效率
        float component_ratio = static_cast<float>(query_components + area_monitors) / entity_count;
        std::cout << "📊 Component creation ratio: " << component_ratio * 100 << "%" << std::endl;
        std::cout << "📊 Memory efficiency: " << (component_ratio < 0.6f ? "Good" : "Needs optimization") << std::endl;
    }

    entt::entity create_test_entity(const JPH::Vec3& position, PhysicsBodyType body_type) {
        auto entity = registry_.create();
        
        PhysicsBodyDesc desc;
        desc.body_type = body_type;
        desc.shape = (body_type == PhysicsBodyType::STATIC) ? 
                     PhysicsShapeDesc::box(JPH::Vec3(100, 1, 100)) :    // 大地面
                     PhysicsShapeDesc::sphere(0.5f);                // 小球
        desc.position = JPH::RVec3(position.GetX(), position.GetY(), position.GetZ());
        
        auto body_id = physics_world_->create_body(desc);
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, body_type, desc.shape);
        physics_component.body_id = body_id;
        
        return entity;
    }
};

int main() {
    std::cout << "Portal Demo Physics Event System Performance Test" << std::endl;
    
    PhysicsEventPerformanceTest test;
    test.run_performance_tests();
    
    return 0;
}
