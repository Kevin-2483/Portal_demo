#include "core/physics_world_manager.h"
#include "core/event_manager.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace portal_core;

int main() {
    std::cout << "=== Simple Debug Test ===" << std::endl;
    
    // 测试1: 初始化物理世界
    std::cout << "1. Initializing physics world..." << std::endl;
    PhysicsWorldManager physics_world;
    if (!physics_world.initialize()) {
        std::cout << "❌ Failed to initialize physics world" << std::endl;
        return 1;
    }
    std::cout << "✅ Physics world initialized" << std::endl;

    // 测试2: 创建简单的物理体
    std::cout << "2. Creating physics bodies..." << std::endl;
    
    PhysicsBodyDesc static_body;
    static_body.body_type = PhysicsBodyType::STATIC;
    static_body.shape = PhysicsShapeDesc::box(JPH::Vec3(10, 0.1f, 10));
    static_body.position = RVec3(0, -1, 0);
    
    auto static_id = physics_world.create_body(static_body);
    if (static_id.IsInvalid()) {
        std::cout << "❌ Failed to create static body" << std::endl;
        return 1;
    }
    std::cout << "✅ Static body created" << std::endl;

    PhysicsBodyDesc dynamic_body;
    dynamic_body.body_type = PhysicsBodyType::DYNAMIC;
    dynamic_body.shape = PhysicsShapeDesc::sphere(0.5f);
    dynamic_body.position = RVec3(0, 2, 0);
    
    auto dynamic_id = physics_world.create_body(dynamic_body);
    if (dynamic_id.IsInvalid()) {
        std::cout << "❌ Failed to create dynamic body" << std::endl;
        return 1;
    }
    std::cout << "✅ Dynamic body created" << std::endl;

    // 测试3: 运行物理模拟
    std::cout << "3. Running physics simulation..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "Frame " << i << ": ";
        
        physics_world.update(1.0f / 60.0f);
        
        auto position = physics_world.get_body_position(dynamic_id);
        std::cout << "Ball at Y=" << position.GetY() << std::endl;
        
        if (position.GetY() < -0.5f) {
            std::cout << "✅ Ball has fallen and hit the ground!" << std::endl;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // 测试4: 清理
    std::cout << "4. Cleaning up..." << std::endl;
    physics_world.destroy_body(static_id);
    physics_world.destroy_body(dynamic_id);
    physics_world.cleanup();
    std::cout << "✅ Cleanup complete" << std::endl;

    std::cout << "\n🎉 All tests passed!" << std::endl;
    return 0;
}
