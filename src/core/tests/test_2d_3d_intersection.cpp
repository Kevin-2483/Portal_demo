#include "core/physics_events/physics_event_system.h"
#include "core/physics_events/physics_events.h"
#include "core/event_manager.h"
#include "core/physics_world_manager.h"
#include "core/components/physics_body_component.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace portal_core;

/**
 * 2D/3D相交检测专门测试
 * 重点测试平面相交（2D）和空间相交（3D）的区别和正确性
 */
class IntersectionTypeTest {
public:
    IntersectionTypeTest() : event_manager_(registry_) {}

    bool run_intersection_tests() {
        std::cout << "=== 2D/3D Intersection Detection Tests ===" << std::endl;
        std::cout << "Testing plane intersection (2D) vs spatial intersection (3D)" << std::endl;
        
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        bool all_passed = true;
        
        // 运行专门的相交测试
        all_passed &= test_water_plane_intersection();
        all_passed &= test_ground_plane_intersection();
        all_passed &= test_wall_plane_intersection();
        all_passed &= test_spatial_3d_intersection();
        all_passed &= test_mixed_intersection_scenarios();
        all_passed &= test_intersection_dimension_detection();

        cleanup_systems();

        std::cout << "\n=== Intersection Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All intersection tests passed!" : "❌ Some intersection tests failed!") << std::endl;
        
        return all_passed;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventSystem> physics_event_system_;

    // 测试中创建的所有实体ID，用于清理
    std::vector<entt::entity> test_entities_;

    // 测试结果统计
    struct IntersectionResults {
        int plane_2d_intersections = 0;      // 平面相交（2D）
        int spatial_3d_intersections = 0;    // 空间相交（3D）
        int water_intersections = 0;
        int ground_intersections = 0;
        int wall_intersections = 0;
        bool correct_dimension_detection = true;
    } results_;

    // 事件处理成员函数
    void handle_collision_start(const CollisionStartEvent& event) {
        if (event.dimension == PhysicsEventDimension::DIMENSION_2D) {
            results_.plane_2d_intersections++;
            std::cout << "🔍 2D Plane intersection detected at (" 
                      << event.contact_point.GetX() << ", " 
                      << event.contact_point.GetY() << ", " 
                      << event.contact_point.GetZ() << ")" << std::endl;
            
            // 根据法线方向判断平面类型
            if (std::abs(event.contact_normal.GetY()) > 0.9f) {
                if (event.contact_point.GetY() > -0.1f && event.contact_point.GetY() < 0.1f) {
                    results_.water_intersections++;  // 水面（Y=0附近）
                } else {
                    results_.ground_intersections++; // 地面
                }
            } else if (std::abs(event.contact_normal.GetX()) > 0.9f || std::abs(event.contact_normal.GetZ()) > 0.9f) {
                results_.wall_intersections++;   // 墙面
            }
        } else if (event.dimension == PhysicsEventDimension::DIMENSION_3D) {
            results_.spatial_3d_intersections++;
            std::cout << "🔍 3D Spatial intersection detected at (" 
                      << event.contact_point.GetX() << ", " 
                      << event.contact_point.GetY() << ", " 
                      << event.contact_point.GetZ() << ")" << std::endl;
        }
    }

    // 在每个测试用例开始前重置状态
    void reset_test_state() {
        // 清理上一个测试创建的所有实体和物理体
        for (auto entity : test_entities_) {
            if (registry_.valid(entity)) {
                auto* body_comp = registry_.try_get<PhysicsBodyComponent>(entity);
                if (body_comp && !body_comp->body_id.IsInvalid()) {
                    physics_world_->destroy_body(body_comp->body_id);
                }
                registry_.destroy(entity);
            }
        }
        test_entities_.clear();

        // 清空结果计数器
        results_ = {};

        // 清理事件队列，防止旧事件干扰
        event_manager_.process_queued_events(0.0f);
    }

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

        physics_event_system_->set_debug_mode(true);
        setup_intersection_callbacks();
        
        return true;
    }

    void setup_intersection_callbacks() {
        // 监听碰撞事件并检查维度类型
        auto collision_sink = physics_event_system_->get_collision_start_sink();
        collision_sink.connect<&IntersectionTypeTest::handle_collision_start>(*this);
    }

    void cleanup_systems() {
        if (physics_event_system_) {
            physics_event_system_->cleanup();
        }
        if (physics_world_) {
            physics_world_->cleanup();
        }
    }

    bool test_water_plane_intersection() {
        reset_test_state();
        std::cout << "\n🌊 Testing water plane intersection (2D)..." << std::endl;
        
        auto monitor_entity = create_test_entity(JPH::Vec3(0,0,0), PhysicsBodyType::STATIC, false); // Monitor doesn't need a body
        auto swimmer_entity = create_test_entity(JPH::Vec3(0, 2, 0), PhysicsBodyType::DYNAMIC);   // 在水面上方
        
        float water_level = 0.0f;
        physics_event_system_->request_water_surface_detection(monitor_entity, swimmer_entity, water_level);
        
        std::cout << "🏊 Entity diving into water from Y=2 to Y=-1..." << std::endl;
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(swimmer_entity).body_id, 
            JPH::Vec3(0, -2, 0));  // 向下跳入水中
        
        // 修正：增加模拟帧数以确保穿越
        simulate_frames(100);
        
        bool passed = results_.water_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Water plane intersection: " 
                  << results_.water_intersections << " water entries detected" << std::endl;
        
        return passed;
    }

    bool test_ground_plane_intersection() {
        reset_test_state();
        std::cout << "\n🌍 Testing ground plane intersection (2D)..." << std::endl;
        
        auto ground_plane = create_plane_entity(JPH::Vec3(10, -1, 0), JPH::Vec3(0, 1, 0), 20.0f);
        auto falling_entity = create_test_entity(JPH::Vec3(10, 5, 0), PhysicsBodyType::DYNAMIC);
        
        std::cout << "📦 Entity falling onto ground plane at Y=-1..." << std::endl;
        
        // 修正：增加模拟帧数以确保落地
        simulate_frames(100);
        
        bool passed = results_.ground_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Ground plane intersection: " 
                  << results_.ground_intersections << " ground hits detected" << std::endl;
        
        return passed;
    }

    bool test_wall_plane_intersection() {
        reset_test_state();
        std::cout << "\n🧱 Testing wall plane intersection (2D)..." << std::endl;
        
        auto wall_plane = create_plane_entity(JPH::Vec3(20, 0, 0), JPH::Vec3(1, 0, 0), 10.0f);
        auto moving_entity = create_test_entity(JPH::Vec3(18, 0, 0), PhysicsBodyType::DYNAMIC);
        
        std::cout << "🏃 Entity moving into wall plane at X=20..." << std::endl;
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(moving_entity).body_id, 
            JPH::Vec3(3, 0, 0));  // 向右移动撞墙
        
        // 修正：增加模拟帧数以确保撞墙
        simulate_frames(60);
        
        bool passed = results_.wall_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Wall plane intersection: " 
                  << results_.wall_intersections << " wall hits detected" << std::endl;
        
        return passed;
    }

    bool test_spatial_3d_intersection() {
        reset_test_state();
        std::cout << "\n🔮 Testing spatial 3D intersection..." << std::endl;
        
        auto ball1 = create_test_entity(JPH::Vec3(30, 0, 0), PhysicsBodyType::DYNAMIC);
        auto ball2 = create_test_entity(JPH::Vec3(33, 1, 0.5), PhysicsBodyType::DYNAMIC);
        
        std::cout << "⚽ Two balls colliding in 3D space..." << std::endl;
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(ball1).body_id, 
            JPH::Vec3(2, 0.5, 0.2));  // 斜向运动
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(ball2).body_id, 
            JPH::Vec3(-1.5, -0.3, -0.1)); // 斜向运动
        
        // 修正：增加模拟帧数以确保碰撞
        simulate_frames(80);
        
        bool passed = results_.spatial_3d_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Spatial 3D intersection: " 
                  << results_.spatial_3d_intersections << " 3D collisions detected" << std::endl;
        
        return passed;
    }

    bool test_mixed_intersection_scenarios() {
        reset_test_state();
        std::cout << "\n🎭 Testing mixed intersection scenarios..." << std::endl;
        
        auto rolling_ball = create_test_entity(JPH::Vec3(40, 3, 0), PhysicsBodyType::DYNAMIC);
        auto slope = create_plane_entity(JPH::Vec3(42, 1, 0), JPH::Vec3(-0.707f, 0.707f, 0), 5.0f);
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(rolling_ball).body_id, 
            JPH::Vec3(1, -0.5, 0));
        
        auto water_monitor = create_test_entity(JPH::Vec3(0,0,0), PhysicsBodyType::STATIC, false);
        physics_event_system_->request_water_surface_detection(water_monitor, rolling_ball, 0.0f);
        
        std::cout << "🎾 Ball rolling down slope then into water..." << std::endl;
        
        // 修正：增加模拟帧数以确保完成整个过程
        simulate_frames(150);
        
        bool has_3d = results_.spatial_3d_intersections > 0;
        bool has_2d = results_.plane_2d_intersections > 0;
        
        bool passed = has_3d && has_2d;
        std::cout << (passed ? "✅" : "❌") << " Mixed intersections: " 
                  << "3D=" << results_.spatial_3d_intersections 
                  << ", 2D=" << results_.plane_2d_intersections << std::endl;
        
        return passed;
    }

    bool test_intersection_dimension_detection() {
        reset_test_state();
        std::cout << "\n🔍 Testing intersection dimension detection accuracy..." << std::endl;
        
        // 情况1：几乎水平的碰撞（应该检测为2D）
        auto entity1 = create_test_entity(JPH::Vec3(50, 0, 0), PhysicsBodyType::DYNAMIC);
        auto entity2 = create_test_entity(JPH::Vec3(52, 0, 0), PhysicsBodyType::DYNAMIC);
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity1).body_id, JPH::Vec3(2, 0, 0));
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity2).body_id, JPH::Vec3(-2, 0, 0));
        
        int initial_2d = results_.plane_2d_intersections;
        int initial_3d = results_.spatial_3d_intersections;
        
        simulate_frames(40);
        
        // 情况2：明显的3D碰撞
        auto entity3 = create_test_entity(JPH::Vec3(60, 0, 0), PhysicsBodyType::DYNAMIC);
        auto entity4 = create_test_entity(JPH::Vec3(61, 1, 1), PhysicsBodyType::DYNAMIC);
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity3).body_id, JPH::Vec3(0.5, 0.5, 0.5));
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity4).body_id, JPH::Vec3(-0.5, -0.5, -0.5));
        
        simulate_frames(80);
        
        int final_2d = results_.plane_2d_intersections;
        int final_3d = results_.spatial_3d_intersections;
        
        bool detected_2d = (final_2d > initial_2d);
        bool detected_3d = (final_3d > initial_3d);
        
        bool passed = detected_2d && detected_3d;
        std::cout << (passed ? "✅" : "❌") << " Dimension detection accuracy: " 
                  << "2D detected=" << (detected_2d ? "Yes" : "No")
                  << ", 3D detected=" << (detected_3d ? "Yes" : "No") << std::endl;
        
        return passed;
    }

    entt::entity create_test_entity(const JPH::Vec3& position, PhysicsBodyType body_type, bool create_body = true) {
        auto entity = registry_.create();
        test_entities_.push_back(entity);

        if (create_body) {
            PhysicsBodyDesc desc;
            desc.body_type = body_type;
            desc.shape = PhysicsShapeDesc::sphere(0.5f);
            desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
            
            auto body_id = physics_world_->create_body(desc);
            auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, body_type, desc.shape);
            physics_component.body_id = body_id;
        }
        
        return entity;
    }

    entt::entity create_plane_entity(const JPH::Vec3& position, const JPH::Vec3& normal, float size) {
        auto entity = registry_.create();
        test_entities_.push_back(entity);
        
        PhysicsBodyDesc desc;
        desc.body_type = PhysicsBodyType::STATIC;
        
        JPH::Vec3 box_size(size, 0.1f, size);
        JPH::Quat rotation = JPH::Quat::sIdentity();

        // Use a thin box and rotate it to represent a plane
        if (std::abs(normal.GetY()) < 0.9f) { // If not a floor/ceiling
            rotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), normal);
        } else if (normal.GetY() < 0) { // Ceiling
            rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), 3.14159265358979323846f);
        }

        desc.shape = PhysicsShapeDesc::box(box_size);
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        desc.rotation = Quat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
        
        auto body_id = physics_world_->create_body(desc);
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, desc.body_type, desc.shape);
        physics_component.body_id = body_id;
        
        return entity;
    }

    void simulate_frames(int frame_count) {
        for (int i = 0; i < frame_count; ++i) {
            float delta_time = 1.0f / 60.0f;
            
            physics_world_->update(delta_time);
            physics_event_system_->update(delta_time);
            event_manager_.process_queued_events(delta_time);
            
            // In a non-test environment, you might not sleep. For testing, this is fine.
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

int main() {
    std::cout << "Portal Demo 2D/3D Intersection Detection Test" << std::endl;
    std::cout << "Testing the difference between plane intersection (2D) and spatial intersection (3D)" << std::endl;
    
    IntersectionTypeTest test;
    bool success = test.run_intersection_tests();
    
    std::cout << "\n" << (success ? "🎉 All tests passed! The system correctly distinguishes between 2D and 3D intersections." 
                                   : "⚠️  Some tests failed. Please check the intersection detection logic.") << std::endl;
    
    return success ? 0 : 1;
}
