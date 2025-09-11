#include "core/physics_events/physics_event_system.h"
#include "core/physics_events/physics_events.h"
#include "core/event_manager.h"
#include "core/physics_world_manager.h"
#include "core/components/physics_body_component.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace portal_core;

/**
 * 物理事件系统测试
 * 测试2D/3D相交检测、懒加载查询等功能
 */
class PhysicsEventSystemTest {
public:
    PhysicsEventSystemTest() : event_manager_(registry_) {}

    bool run_all_tests() {
        std::cout << "=== Portal Demo Physics Event System Tests ===" << std::endl;
        
        bool all_passed = true;
        
        // 初始化系统
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        // 运行各种测试
        all_passed &= test_collision_events();
        all_passed &= test_trigger_events();
        all_passed &= test_raycast_queries();
        all_passed &= test_area_monitoring();
        all_passed &= test_2d_plane_intersection();
        all_passed &= test_3d_space_intersection();
        all_passed &= test_lazy_loading();
        all_passed &= test_water_surface_detection();
        all_passed &= test_ground_detection();

        // 清理
        cleanup_systems();

        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All tests passed!" : "❌ Some tests failed!") << std::endl;
        
        return all_passed;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventSystem> physics_event_system_;

    // 测试结果收集
    struct TestResults {
        int collision_start_events = 0;
        int collision_end_events = 0;
        int trigger_enter_events = 0;
        int trigger_exit_events = 0;
        int raycast_result_events = 0;
        int overlap_result_events = 0;
        bool water_surface_detected = false;
        bool ground_detected = false;
        bool plane_intersection_detected = false;
    } results_;

    // 事件处理成员函数
    void handle_collision_start(const CollisionStartEvent& event) {
        results_.collision_start_events++;
        std::cout << "📬 Collision start event received (entities: " 
                  << static_cast<uint32_t>(event.entity_a) << " <-> " 
                  << static_cast<uint32_t>(event.entity_b) << ")" << std::endl;
        
        // 检查是否为平面相交（2D）
        if (event.dimension == PhysicsEventDimension::DIMENSION_2D) {
            results_.plane_intersection_detected = true;
            std::cout << "🔍 2D Plane intersection detected!" << std::endl;
        }
    }

    void handle_collision_end(const CollisionEndEvent& event) {
        results_.collision_end_events++;
        std::cout << "📭 Collision end event received" << std::endl;
    }

    void handle_trigger_enter(const TriggerEnterEvent& event) {
        results_.trigger_enter_events++;
        std::cout << "🚪 Trigger enter event received (sensor: " 
                  << static_cast<uint32_t>(event.sensor_entity) << ", entity: " 
                  << static_cast<uint32_t>(event.other_entity) << ")" << std::endl;
    }

    void handle_trigger_exit(const TriggerExitEvent& event) {
        results_.trigger_exit_events++;
        std::cout << "🚪 Trigger exit event received" << std::endl;
    }

    void handle_raycast_result(const RaycastResultEvent& event) {
        results_.raycast_result_events++;
        std::cout << "🎯 Raycast result received (hit: " << (event.hit ? "Yes" : "No") << ")" << std::endl;
        
        if (event.hit && event.hit_distance < 1.0f) {  // 检测到地面
            results_.ground_detected = true;
            std::cout << "🌍 Ground detection successful!" << std::endl;
        }
    }

    void handle_overlap_result(const OverlapQueryResultEvent& event) {
        results_.overlap_result_events++;
        std::cout << "🔍 Overlap result received (objects found: " << event.overlapping_entities.size() << ")" << std::endl;
        
        if (!event.overlapping_entities.empty()) {
            results_.water_surface_detected = true;
            std::cout << "🌊 Water surface interaction detected!" << std::endl;
        }
    }

    bool initialize_systems() {
        std::cout << "\n🔧 Initializing systems..." << std::endl;

        // 初始化物理世界
        physics_world_ = std::make_unique<PhysicsWorldManager>();
        if (!physics_world_->initialize()) {
            std::cout << "❌ Failed to initialize PhysicsWorldManager" << std::endl;
            return false;
        }
        std::cout << "✅ PhysicsWorldManager initialized" << std::endl;

        // 初始化物理事件系统
        physics_event_system_ = std::make_unique<PhysicsEventSystem>(
            event_manager_, *physics_world_, registry_);
        
        if (!physics_event_system_->initialize()) {
            std::cout << "❌ Failed to initialize PhysicsEventSystem" << std::endl;
            return false;
        }
        physics_event_system_->set_debug_mode(true);
        std::cout << "✅ PhysicsEventSystem initialized" << std::endl;

        // 设置事件回调
        setup_event_callbacks();
        
        return true;
    }

    void setup_event_callbacks() {
        // 订阅碰撞事件
        auto collision_start_sink = physics_event_system_->get_collision_start_sink();
        collision_start_sink.connect<&PhysicsEventSystemTest::handle_collision_start>(*this);

        auto collision_end_sink = physics_event_system_->get_collision_end_sink();
        collision_end_sink.connect<&PhysicsEventSystemTest::handle_collision_end>(*this);

        // 订阅触发器事件
        auto trigger_enter_sink = physics_event_system_->get_trigger_enter_sink();
        trigger_enter_sink.connect<&PhysicsEventSystemTest::handle_trigger_enter>(*this);

        auto trigger_exit_sink = physics_event_system_->get_trigger_exit_sink();
        trigger_exit_sink.connect<&PhysicsEventSystemTest::handle_trigger_exit>(*this);

        // 订阅射线检测结果
        auto raycast_result_sink = physics_event_system_->get_raycast_result_sink();
        raycast_result_sink.connect<&PhysicsEventSystemTest::handle_raycast_result>(*this);

        // 订阅重叠查询结果
        auto overlap_result_sink = physics_event_system_->get_overlap_result_sink();
        overlap_result_sink.connect<&PhysicsEventSystemTest::handle_overlap_result>(*this);
    }

    void cleanup_systems() {
        std::cout << "\n🧹 Cleaning up systems..." << std::endl;
        
        if (physics_event_system_) {
            physics_event_system_->cleanup();
        }
        
        if (physics_world_) {
            physics_world_->cleanup();
        }
        
        std::cout << "✅ Systems cleaned up" << std::endl;
    }

    bool test_collision_events() {
        std::cout << "\n🧪 Testing collision events..." << std::endl;

        // 创建一个更大的地面，确保碰撞能够发生
        auto ground = create_ground_entity(JPH::Vec3(0, -0.5, 0), JPH::Vec3(10, 0.5, 10));  // 大地面
        auto falling_ball = create_test_entity(JPH::Vec3(0, 5, 0), PhysicsBodyType::DYNAMIC);  // 掉落的球

        // 添加调试信息
        auto& ground_body = registry_.get<PhysicsBodyComponent>(ground);
        auto& ball_body = registry_.get<PhysicsBodyComponent>(falling_ball);
        std::cout << "Created ground: " << static_cast<uint32_t>(ground) 
                  << " (BodyID: " << ground_body.body_id.GetIndexAndSequenceNumber() << ") at (0, -0.5, 0)" << std::endl;
        std::cout << "Created falling ball: " << static_cast<uint32_t>(falling_ball) 
                  << " (BodyID: " << ball_body.body_id.GetIndexAndSequenceNumber() << ") at (0, 5, 0)" << std::endl;

        // 让球有一定的初始速度向下
        physics_world_->set_body_linear_velocity(ball_body.body_id, JPH::Vec3(0, -5, 0));

        // 模拟更多帧物理更新，确保球落到地面发生碰撞
        simulate_physics_frames(200);  // 增加到200帧，确保球有足够时间落到地面

        bool passed = results_.collision_start_events > 0;
        std::cout << (passed ? "✅" : "❌") << " Collision events test: " 
                  << results_.collision_start_events << " events received" << std::endl;
        
        return passed;
    }

    bool test_trigger_events() {
        std::cout << "\n🧪 Testing trigger events..." << std::endl;

        // 创建触发器
        auto trigger = create_trigger_entity(JPH::Vec3(10, 0, 0), 2.0f);  // 半径2米的触发器
        auto moving_entity = create_test_entity(JPH::Vec3(6, 0, 0), PhysicsBodyType::DYNAMIC);  // 移动物体，从更远处开始

        // 检查触发器组件是否正确设置
        auto& trigger_comp = registry_.get<PhysicsBodyComponent>(trigger);
        auto& moving_comp = registry_.get<PhysicsBodyComponent>(moving_entity);
        
        std::cout << "Created trigger: " << static_cast<uint32_t>(trigger) 
                  << " (BodyID: " << trigger_comp.body_id.GetIndexAndSequenceNumber() << ")"
                  << " at (10, 0, 0), is_trigger=" << trigger_comp.is_trigger << std::endl;
        std::cout << "Created moving entity: " << static_cast<uint32_t>(moving_entity) 
                  << " (BodyID: " << moving_comp.body_id.GetIndexAndSequenceNumber() << ")"
                  << " at (6, 0, 0), is_trigger=" << moving_comp.is_trigger << std::endl;

        // 给移动物体添加更大的速度，确保它能进入触发器
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(moving_entity).body_id, 
            JPH::Vec3(5, 0, 0));  // 更快地向右移动

        // 模拟物理更新，给足够时间让实体进入触发器
        simulate_physics_frames(300);

        bool passed = results_.trigger_enter_events > 0;
        std::cout << (passed ? "✅" : "❌") << " Trigger events test: " 
                  << results_.trigger_enter_events << " enter events received" << std::endl;
        
        return passed;
    }

    bool test_raycast_queries() {
        std::cout << "\n🧪 Testing raycast queries..." << std::endl;

        // 创建射线查询实体
        auto raycast_entity = registry_.create();
        
        // 向下射线，应该击中地面
        JPH::Vec3 origin(0, 10, 0);
        JPH::Vec3 direction(0, -1, 0);  // 向下
        
        physics_event_system_->request_raycast(raycast_entity, origin, direction, 20.0f);

        // 处理查询
        simulate_physics_frames(3);

        bool passed = results_.raycast_result_events > 0;
        std::cout << (passed ? "✅" : "❌") << " Raycast queries test: " 
                  << results_.raycast_result_events << " results received" << std::endl;
        
        return passed;
    }

    bool test_area_monitoring() {
        std::cout << "\n🧪 Testing area monitoring..." << std::endl;

        // 创建区域监控
        auto monitor_entity = registry_.create();
        JPH::Vec3 monitor_center(20, 0, 0);
        float monitor_radius = 3.0f;
        
        physics_event_system_->request_area_monitoring(monitor_entity, monitor_center, monitor_radius);

        // 创建一个实体进入监控区域
        auto test_entity = create_test_entity(JPH::Vec3(18, 0, 0), PhysicsBodyType::DYNAMIC);
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(test_entity).body_id, 
            JPH::Vec3(1, 0, 0));  // 向右移动进入区域

        // 处理监控
        simulate_physics_frames(10);

        bool passed = results_.overlap_result_events > 0;
        std::cout << (passed ? "✅" : "❌") << " Area monitoring test: " 
                  << results_.overlap_result_events << " overlap results received" << std::endl;
        
        return passed;
    }

    bool test_2d_plane_intersection() {
        std::cout << "\n🧪 Testing 2D plane intersection..." << std::endl;

        // 创建平面相交监控
        auto monitor_entity = registry_.create();
        auto target_entity = create_test_entity(JPH::Vec3(30, 5, 0), PhysicsBodyType::DYNAMIC);  // 在平面上方
        
        JPH::Vec3 plane_normal(0, 1, 0);  // 水平平面，法线向上
        float plane_distance = 0.0f;  // Y=0平面
        
        physics_event_system_->request_plane_intersection(monitor_entity, target_entity, 
                                                        plane_normal, plane_distance);

        // 让物体下落穿越平面
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(target_entity).body_id, 
            JPH::Vec3(0, -2, 0));  // 向下移动

        // 处理平面相交检测
        simulate_physics_frames(15);

        bool passed = results_.plane_intersection_detected;
        std::cout << (passed ? "✅" : "❌") << " 2D plane intersection test: " 
                  << (passed ? "Detected" : "Not detected") << std::endl;
        
        return passed;
    }

    bool test_3d_space_intersection() {
        std::cout << "\n🧪 Testing 3D space intersection..." << std::endl;

        // 创建两个在空间中相交的物体
        auto entity1 = create_test_entity(JPH::Vec3(40, 0, 0), PhysicsBodyType::DYNAMIC);
        auto entity2 = create_test_entity(JPH::Vec3(42, 0, 0), PhysicsBodyType::DYNAMIC);

        // 让它们相向运动
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity1).body_id, 
            JPH::Vec3(1, 0, 0));
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity2).body_id, 
            JPH::Vec3(-1, 0, 0));

        // 处理3D相交
        simulate_physics_frames(10);

        // 检查是否有3D相交事件
        bool passed = results_.collision_start_events > 2;  // 之前的测试已经有一些碰撞事件
        std::cout << (passed ? "✅" : "❌") << " 3D space intersection test: " 
                  << (passed ? "Detected" : "Not detected") << std::endl;
        
        return passed;
    }

    bool test_lazy_loading() {
        std::cout << "\n🧪 Testing lazy loading..." << std::endl;

        // 创建实体但不立即添加组件
        auto entity = registry_.create();

        // 检查没有查询组件
        bool no_query_component_initially = !registry_.all_of<PhysicsEventQueryComponent>(entity);

        // 请求查询 - 应该懒加载创建组件
        physics_event_system_->request_raycast(entity, JPH::Vec3(50, 0, 0), JPH::Vec3(0, -1, 0));

        // 检查是否懒加载创建了组件
        bool query_component_created = registry_.all_of<PhysicsEventQueryComponent>(entity);
        bool pending_tag_created = registry_.all_of<PendingQueryTag>(entity);

        bool passed = no_query_component_initially && query_component_created && pending_tag_created;
        std::cout << (passed ? "✅" : "❌") << " Lazy loading test: " 
                  << (passed ? "Components created on demand" : "Failed to create components") << std::endl;
        
        return passed;
    }

    bool test_water_surface_detection() {
        std::cout << "\n🧪 Testing water surface detection..." << std::endl;

        // 创建水面检测
        auto monitor_entity = registry_.create();
        auto swimmer_entity = create_test_entity(JPH::Vec3(60, 2, 0), PhysicsBodyType::DYNAMIC);  // 在水面上方
        
        float water_level = 0.0f;
        physics_event_system_->request_water_surface_detection(monitor_entity, swimmer_entity, water_level);

        // 让实体"跳入"水中
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(swimmer_entity).body_id, 
            JPH::Vec3(0, -3, 0));  // 快速下落

        // 处理水面检测
        simulate_physics_frames(10);

        // 水面检测应该触发平面相交事件
        bool passed = results_.plane_intersection_detected;
        std::cout << (passed ? "✅" : "❌") << " Water surface detection test: " 
                  << (passed ? "Water entry detected" : "No water entry detected") << std::endl;
        
        return passed;
    }

    bool test_ground_detection() {
        std::cout << "\n🧪 Testing ground detection..." << std::endl;

        // 创建地面检测
        auto detector_entity = registry_.create();
        auto falling_entity = create_test_entity(JPH::Vec3(70, 5, 0), PhysicsBodyType::DYNAMIC);  // 在空中
        
        physics_event_system_->request_ground_detection(detector_entity, falling_entity);

        // 处理地面检测
        simulate_physics_frames(5);

        bool passed = results_.ground_detected;
        std::cout << (passed ? "✅" : "❌") << " Ground detection test: " 
                  << (passed ? "Ground detected" : "No ground detected") << std::endl;
        
        return passed;
    }

    entt::entity create_test_entity(const JPH::Vec3& position, PhysicsBodyType body_type) {
        auto entity = registry_.create();
        
        // 创建物理体
        PhysicsBodyDesc desc;
        desc.body_type = body_type;
        desc.shape = PhysicsShapeDesc::sphere(0.5f);  // 半径0.5米的球
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        
        auto body_id = physics_world_->create_body(desc);
        
        // 添加物理体组件
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, body_type, desc.shape);
        physics_component.body_id = body_id;
        
        // 手动触发组件更新事件，让监听器能捕获到正确的BodyID
        registry_.patch<PhysicsBodyComponent>(entity);
        
        return entity;
    }

    entt::entity create_trigger_entity(const JPH::Vec3& position, float radius) {
        auto entity = registry_.create();
        
        // 创建触发器
        PhysicsBodyDesc desc;
        desc.body_type = PhysicsBodyType::TRIGGER;
        desc.shape = PhysicsShapeDesc::sphere(radius);
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        
        auto body_id = physics_world_->create_body(desc);
        
        // 添加物理体组件
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, desc.body_type, desc.shape);
        physics_component.body_id = body_id;
        physics_component.is_trigger = true;  // 确保触发器标志被设置
        
        // 手动触发组件更新事件，让监听器能捕获到正确的BodyID
        registry_.patch<PhysicsBodyComponent>(entity);
        
        std::cout << "Created trigger entity " << static_cast<uint32_t>(entity) 
                  << " (BodyID: " << body_id.GetIndexAndSequenceNumber() 
                  << ") with is_trigger=" << physics_component.is_trigger << std::endl;
        
        return entity;
    }

    entt::entity create_ground_entity(const JPH::Vec3& position, const JPH::Vec3& half_extents) {
        auto entity = registry_.create();
        
        // 创建盒状地面
        PhysicsBodyDesc desc;
        desc.body_type = PhysicsBodyType::STATIC;
        desc.shape = PhysicsShapeDesc::box(half_extents);  // 使用盒子形状
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        
        auto body_id = physics_world_->create_body(desc);
        
        // 添加物理体组件
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, desc.body_type, desc.shape);
        physics_component.body_id = body_id;
        
        // 手动触发组件更新事件，让监听器能捕获到正确的BodyID
        registry_.patch<PhysicsBodyComponent>(entity);
        
        return entity;
    }

    void print_physics_bodies_status() {
        // 打印所有有PhysicsBodyComponent的实体的状态
        auto view = registry_.view<PhysicsBodyComponent>();
        for (auto entity : view) {
            auto& body_comp = view.get<PhysicsBodyComponent>(entity);
            if (!body_comp.body_id.IsInvalid()) {
                auto position = physics_world_->get_body_position(body_comp.body_id);
                std::cout << "        Entity " << static_cast<uint32_t>(entity) 
                          << " (BodyID: " << body_comp.body_id.GetIndexAndSequenceNumber() << ") "
                          << "at (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << ")" << std::endl;
            }
        }
    }

    void simulate_physics_frames(int frame_count) {
        std::cout << "  🔄 Starting physics simulation for " << frame_count << " frames..." << std::endl;
        
        for (int i = 0; i < frame_count; ++i) {
            float delta_time = 1.0f / 60.0f;  // 60 FPS

            // 每二十帧打印一次（或第一帧和最后一帧）
            bool should_print = ((i + 1) % 40 == 0) || (i == 0) || (i == frame_count - 1);
            
            if (should_print) {
                std::cout << "    Frame " << (i + 1) << "/" << frame_count << ":" << std::endl;
                
                // 检查一些物理体的位置（如果存在的话）
                print_physics_bodies_status();
            }
            
            // 更新物理世界
            physics_world_->update(delta_time);
            if (should_print) {
                std::cout << "      ✅ Physics world updated" << std::endl;
            }
            
            // 更新物理事件系统
            physics_event_system_->update(delta_time);
            if (should_print) {
                std::cout << "      ✅ Physics event system updated" << std::endl;
            }
            
            // 处理事件队列
            event_manager_.process_queued_events(delta_time);
            if (should_print) {
                std::cout << "      ✅ Event queue processed" << std::endl;
                
                // 检查累积的事件计数
                std::cout << "      📊 Events so far - Collisions: " << results_.collision_start_events 
                          << ", Triggers: " << results_.trigger_enter_events 
                          << ", Raycasts: " << results_.raycast_result_events << std::endl;
            }
            
            // 短暂暂停
            std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        }
        
        std::cout << "  ✅ Physics simulation completed" << std::endl;
    }
};

int main() {
    std::cout << "Portal Demo Physics Event System Test" << std::endl;
    std::cout << "Testing 2D/3D intersection detection and lazy loading..." << std::endl;
    
    PhysicsEventSystemTest test;
    bool success = test.run_all_tests();
    
    return success ? 0 : 1;
}
