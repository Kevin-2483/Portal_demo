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

    // 测试结果统计
    struct IntersectionResults {
        int plane_2d_intersections = 0;       // 平面相交（2D）
        int spatial_3d_intersections = 0;     // 空间相交（3D）
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
                    results_.ground_intersections++;  // 地面
                }
            } else if (std::abs(event.contact_normal.GetX()) > 0.9f || std::abs(event.contact_normal.GetZ()) > 0.9f) {
                results_.wall_intersections++;  // 墙面
            }
        } else if (event.dimension == PhysicsEventDimension::DIMENSION_3D) {
            results_.spatial_3d_intersections++;
            std::cout << "🔍 3D Spatial intersection detected at (" 
                      << event.contact_point.GetX() << ", " 
                      << event.contact_point.GetY() << ", " 
                      << event.contact_point.GetZ() << ")" << std::endl;
        }
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
        std::cout << "\n🌊 Testing water plane intersection (2D)..." << std::endl;
        
        // 创建水面检测场景
        auto monitor_entity = registry_.create();
        auto swimmer_entity = create_test_entity(JPH::Vec3(0, 2, 0), PhysicsBodyType::DYNAMIC);  // 在水面上方
        
        // 设置水面为Y=0平面
        float water_level = 0.0f;
        physics_event_system_->request_water_surface_detection(monitor_entity, swimmer_entity, water_level);
        
        std::cout << "🏊 Entity diving into water from Y=2 to Y=-1..." << std::endl;
        
        // 模拟跳水动作
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(swimmer_entity).body_id, 
            JPH::Vec3(0, -2, 0));  // 向下跳入水中
        
        // 模拟足够长的时间让实体穿越水面
        simulate_frames(20);
        
        bool passed = results_.water_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Water plane intersection: " 
                  << results_.water_intersections << " water entries detected" << std::endl;
        
        return passed;
    }

    bool test_ground_plane_intersection() {
        std::cout << "\n🌍 Testing ground plane intersection (2D)..." << std::endl;
        
        // 创建地面场景
        auto ground_plane = create_plane_entity(JPH::Vec3(10, -1, 0), JPH::Vec3(0, 1, 0), 20.0f);  // 水平地面
        auto falling_entity = create_test_entity(JPH::Vec3(10, 5, 0), PhysicsBodyType::DYNAMIC);  // 掉落物体
        
        std::cout << "📦 Entity falling onto ground plane at Y=-1..." << std::endl;
        
        // 模拟掉落
        simulate_frames(15);
        
        bool passed = results_.ground_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Ground plane intersection: " 
                  << results_.ground_intersections << " ground hits detected" << std::endl;
        
        return passed;
    }

    bool test_wall_plane_intersection() {
        std::cout << "\n🧱 Testing wall plane intersection (2D)..." << std::endl;
        
        // 创建垂直墙面
        auto wall_plane = create_plane_entity(JPH::Vec3(20, 0, 0), JPH::Vec3(1, 0, 0), 10.0f);  // 垂直墙面，法线向X方向
        auto moving_entity = create_test_entity(JPH::Vec3(18, 0, 0), PhysicsBodyType::DYNAMIC);  // 向墙移动的物体
        
        std::cout << "🏃 Entity moving into wall plane at X=20..." << std::endl;
        
        // 向墙面移动
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(moving_entity).body_id, 
            JPH::Vec3(3, 0, 0));  // 向右移动撞墙
        
        simulate_frames(10);
        
        bool passed = results_.wall_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Wall plane intersection: " 
                  << results_.wall_intersections << " wall hits detected" << std::endl;
        
        return passed;
    }

    bool test_spatial_3d_intersection() {
        std::cout << "\n🔮 Testing spatial 3D intersection..." << std::endl;
        
        // 创建两个在3D空间中相撞的球体
        auto ball1 = create_test_entity(JPH::Vec3(30, 0, 0), PhysicsBodyType::DYNAMIC);
        auto ball2 = create_test_entity(JPH::Vec3(33, 1, 0.5), PhysicsBodyType::DYNAMIC);  // 不同高度和深度
        
        std::cout << "⚽ Two balls colliding in 3D space..." << std::endl;
        
        // 让它们相向运动（不是沿坐标轴的运动）
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(ball1).body_id, 
            JPH::Vec3(2, 0.5, 0.2));  // 斜向运动
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(ball2).body_id, 
            JPH::Vec3(-1.5, -0.3, -0.1));  // 斜向运动
        
        simulate_frames(15);
        
        bool passed = results_.spatial_3d_intersections > 0;
        std::cout << (passed ? "✅" : "❌") << " Spatial 3D intersection: " 
                  << results_.spatial_3d_intersections << " 3D collisions detected" << std::endl;
        
        return passed;
    }

    bool test_mixed_intersection_scenarios() {
        std::cout << "\n🎭 Testing mixed intersection scenarios..." << std::endl;
        
        // 创建复杂场景：同时有2D平面相交和3D空间相交
        
        // 场景1：球滚下斜坡（3D）然后落入水中（2D）
        auto rolling_ball = create_test_entity(JPH::Vec3(40, 3, 0), PhysicsBodyType::DYNAMIC);
        auto slope = create_plane_entity(JPH::Vec3(42, 1, 0), JPH::Vec3(-0.707f, 0.707f, 0), 5.0f);  // 45度斜坡
        
        // 球向斜坡滚动
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(rolling_ball).body_id, 
            JPH::Vec3(1, -0.5, 0));
        
        // 设置水面检测
        auto water_monitor = registry_.create();
        physics_event_system_->request_water_surface_detection(water_monitor, rolling_ball, 0.0f);
        
        std::cout << "🎾 Ball rolling down slope then into water..." << std::endl;
        
        simulate_frames(25);
        
        // 验证既有3D相交（球撞斜坡）又有2D相交（球入水）
        bool has_3d = results_.spatial_3d_intersections > 1;  // 之前测试已有一些
        bool has_2d = results_.plane_2d_intersections > 1;    // 之前测试已有一些
        
        bool passed = has_3d && has_2d;
        std::cout << (passed ? "✅" : "❌") << " Mixed intersections: " 
                  << "3D=" << results_.spatial_3d_intersections 
                  << ", 2D=" << results_.plane_2d_intersections << std::endl;
        
        return passed;
    }

    bool test_intersection_dimension_detection() {
        std::cout << "\n🔍 Testing intersection dimension detection accuracy..." << std::endl;
        
        // 测试边缘情况：接近但不完全沿坐标轴的碰撞
        
        // 情况1：几乎水平的碰撞（应该检测为2D）
        auto entity1 = create_test_entity(JPH::Vec3(50, 0, 0), PhysicsBodyType::DYNAMIC);
        auto entity2 = create_test_entity(JPH::Vec3(52, 0.1, 0), PhysicsBodyType::DYNAMIC);  // 微小Y偏移
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity1).body_id, 
            JPH::Vec3(1, 0, 0));  // 主要沿X轴
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity2).body_id, 
            JPH::Vec3(-1, 0, 0));  // 主要沿X轴
        
        int initial_2d = results_.plane_2d_intersections;
        int initial_3d = results_.spatial_3d_intersections;
        
        simulate_frames(10);
        
        // 情况2：明显的3D碰撞
        auto entity3 = create_test_entity(JPH::Vec3(60, 0, 0), PhysicsBodyType::DYNAMIC);
        auto entity4 = create_test_entity(JPH::Vec3(61, 1, 1), PhysicsBodyType::DYNAMIC);
        
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity3).body_id, 
            JPH::Vec3(0.5, 0.5, 0.5));  // 3D方向
        physics_world_->set_body_linear_velocity(
            registry_.get<PhysicsBodyComponent>(entity4).body_id, 
            JPH::Vec3(-0.5, -0.5, -0.5));  // 3D方向
        
        simulate_frames(10);
        
        int final_2d = results_.plane_2d_intersections;
        int final_3d = results_.spatial_3d_intersections;
        
        // 验证检测的准确性
        bool detected_2d = (final_2d > initial_2d);
        bool detected_3d = (final_3d > initial_3d);
        
        bool passed = detected_2d && detected_3d;
        std::cout << (passed ? "✅" : "❌") << " Dimension detection accuracy: " 
                  << "2D detected=" << (detected_2d ? "Yes" : "No")
                  << ", 3D detected=" << (detected_3d ? "Yes" : "No") << std::endl;
        
        return passed;
    }

    entt::entity create_test_entity(const JPH::Vec3& position, PhysicsBodyType body_type) {
        auto entity = registry_.create();
        
        PhysicsBodyDesc desc;
        desc.body_type = body_type;
        desc.shape = PhysicsShapeDesc::sphere(0.5f);
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        
        auto body_id = physics_world_->create_body(desc);
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, body_type, desc.shape);
        physics_component.body_id = body_id;
        
        return entity;
    }

    entt::entity create_plane_entity(const JPH::Vec3& position, const JPH::Vec3& normal, float size) {
        auto entity = registry_.create();
        
        PhysicsBodyDesc desc;
        desc.body_type = PhysicsBodyType::STATIC;
        
        // 根据法线方向创建合适的盒子形状作为平面
        JPH::Vec3 box_size;
        if (std::abs(normal.GetY()) > 0.9f) {
            // 水平平面
            box_size = JPH::Vec3(size, 0.1f, size);
        } else if (std::abs(normal.GetX()) > 0.9f) {
            // 垂直平面（YZ平面）
            box_size = JPH::Vec3(0.1f, size, size);
        } else {
            // 其他平面
            box_size = JPH::Vec3(size, size, 0.1f);
        }
        
        desc.shape = PhysicsShapeDesc::box(box_size);
        desc.position = RVec3(position.GetX(), position.GetY(), position.GetZ());
        
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
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
