#include "../physics_events/physics_event_system.h"
#include "../physics_events/physics_events.h"
#include "../event_manager.h"
#include "../physics_world_manager.h"
#include "../components/physics_body_component.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace portal_core;

/**
 * 简化的懒加载机制测试
 * 重点测试组件的按需创建和基本生命周期管理
 */
class SimpleLazyLoadingTest
{
public:
    SimpleLazyLoadingTest() : event_manager_(registry_) {}

    bool run_lazy_loading_tests()
    {
        std::cout << "=== Simplified Lazy Loading Tests ===" << std::endl;
        std::cout << "Testing basic component creation and lifecycle" << std::endl;

        if (!initialize_systems())
        {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        bool all_passed = true;

        // 运行简化的懒加载测试
        all_passed &= test_basic_component_creation();
        all_passed &= test_component_lifecycle();
        all_passed &= test_memory_efficiency();

        cleanup_systems();

        std::cout << "\n=== Lazy Loading Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All lazy loading tests passed!" : "❌ Some lazy loading tests failed!") << std::endl;

        return all_passed;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventSystem> physics_event_system_;

    bool initialize_systems()
    {
        physics_world_ = std::make_unique<PhysicsWorldManager>();
        if (!physics_world_->initialize())
        {
            return false;
        }

        physics_event_system_ = std::make_unique<PhysicsEventSystem>(
            event_manager_, *physics_world_, registry_);

        if (!physics_event_system_->initialize())
        {
            return false;
        }

        physics_event_system_->set_debug_mode(true);

        return true;
    }

    void cleanup_systems()
    {
        if (physics_event_system_)
        {
            physics_event_system_->cleanup();
        }
        if (physics_world_)
        {
            physics_world_->cleanup();
        }
    }

    bool test_basic_component_creation()
    {
        std::cout << "\n🎯 Testing basic component creation..." << std::endl;

        // 创建一些实体
        std::vector<entt::entity> entities;
        for (int i = 0; i < 5; ++i)
        {
            auto entity = create_test_entity(JPH::Vec3(i * 2, 0, 0), PhysicsBodyType::DYNAMIC);
            entities.push_back(entity);
        }

        // 验证初始状态 - 应该没有特殊的查询组件
        auto all_entities = registry_.view<entt::entity>();
        size_t initial_entities = all_entities.size();
        std::cout << "📊 Created entities: " << initial_entities << std::endl;

        // 现在请求区域监控，应该触发组件创建
        std::cout << "� Requesting area monitoring (tests component creation)..." << std::endl;
        for (auto entity : entities)
        {
            auto monitor = registry_.create();
            // 获取实体的物理体组件和位置
            if (auto *physics_comp = registry_.try_get<PhysicsBodyComponent>(entity))
            {
                auto position = physics_world_->get_body_position(physics_comp->body_id);
                physics_event_system_->request_area_monitoring(monitor, JPH::Vec3(position.GetX(), position.GetY(), position.GetZ()), 2.0f);
            }
        }

        simulate_frames(3);

        // 验证监控组件已创建
        int monitor_components = 0;
        auto monitor_view = registry_.view<AreaMonitorComponent>();
        for (auto [entity, monitor] : monitor_view.each())
        {
            monitor_components++;
        }

        bool passed = monitor_components > 0;
        std::cout << (passed ? "✅" : "❌") << " Component creation: "
                  << monitor_components << " monitor components created" << std::endl;

        return passed;
    }

    bool test_component_lifecycle()
    {
        std::cout << "\n♻️ Testing component lifecycle..." << std::endl;

        // 创建临时实体
        auto temp_entity = create_test_entity(JPH::Vec3(20, 0, 0), PhysicsBodyType::DYNAMIC);

        std::cout << "� Creating and destroying entity with components..." << std::endl;

        // 创建监控组件
        auto monitor = registry_.create();
        physics_event_system_->request_area_monitoring(monitor, JPH::Vec3(1, 0, 0), 1.0f);

        simulate_frames(2);

        // 验证组件存在
        bool monitor_exists = registry_.any_of<AreaMonitorComponent>(monitor);

        std::cout << "📋 Component creation: " << (monitor_exists ? "✅" : "❌") << std::endl;

        // 销毁实体
        registry_.destroy(temp_entity);
        simulate_frames(2);

        // 验证系统仍然稳定（不会因为引用无效实体而崩溃）
        bool system_stable = true;
        try
        {
            simulate_frames(3);
        }
        catch (...)
        {
            system_stable = false;
        }

        bool passed = monitor_exists && system_stable;
        std::cout << (passed ? "✅" : "❌") << " Lifecycle management: Component created and system remains stable" << std::endl;

        return passed;
    }

    bool test_memory_efficiency()
    {
        std::cout << "\n💾 Testing memory efficiency..." << std::endl;

        const int entity_count = 50;
        std::vector<entt::entity> entities;

        std::cout << "🏭 Creating " << entity_count << " entities..." << std::endl;

        for (int i = 0; i < entity_count; ++i)
        {
            auto entity = create_test_entity(JPH::Vec3(i % 10, 0, i / 10), PhysicsBodyType::DYNAMIC);
            entities.push_back(entity);
        }

        int initial_components = count_all_components();

        // 只为少数实体请求监控
        std::cout << "🎯 Requesting monitoring for only 20% of entities..." << std::endl;

        for (int i = 0; i < entity_count / 5; ++i)
        {
            auto monitor = registry_.create();
            physics_event_system_->request_area_monitoring(monitor, JPH::Vec3(i * 2, 0, 0), 1.0f);
        }

        simulate_frames(5);

        int final_components = count_all_components();

        // 验证内存效率：只有请求的实体才有额外组件
        int monitor_components = 0;
        auto monitor_view = registry_.view<AreaMonitorComponent>();
        for (auto [entity, monitor] : monitor_view.each())
        {
            monitor_components++;
        }

        bool efficient = (monitor_components <= entity_count / 3); // 应该远少于总实体数

        std::cout << "📊 Memory usage:" << std::endl;
        std::cout << "  Initial components: " << initial_components << std::endl;
        std::cout << "  Final components: " << final_components << std::endl;
        std::cout << "  Monitor components: " << monitor_components << std::endl;
        std::cout << "  Memory efficiency: " << (efficient ? "Good" : "Poor") << std::endl;

        bool passed = efficient && (monitor_components > 0);
        std::cout << (passed ? "✅" : "❌") << " Memory efficiency: Only needed components allocated" << std::endl;

        return passed;
    }

    entt::entity create_test_entity(const JPH::Vec3 &position, PhysicsBodyType body_type)
    {
        auto entity = registry_.create();

        PhysicsBodyDesc desc;
        desc.body_type = body_type;
        desc.shape = PhysicsShapeDesc::sphere(0.5f);
        desc.position = JPH::RVec3(position.GetX(), position.GetY(), position.GetZ());

        auto body_id = physics_world_->create_body(desc);
        
        // 正确创建PhysicsBodyComponent
        auto& physics_component = registry_.emplace<PhysicsBodyComponent>(entity, body_type, desc.shape);
        physics_component.body_id = body_id;

        return entity;
    }

    int count_all_components()
    {
        // 统计所有类型的组件
        int count = 0;

        // 基础组件
        registry_.view<PhysicsBodyComponent>().each([&count](auto)
                                                    { count++; });

        // 监控组件
        registry_.view<AreaMonitorComponent>().each([&count](auto)
                                                    { count++; });

        // 其他事件组件
        registry_.view<PlaneIntersectionComponent>().each([&count](auto)
                                                          { count++; });

        return count;
    }

    void simulate_frames(int frame_count)
    {
        for (int i = 0; i < frame_count; ++i)
        {
            float delta_time = 1.0f / 60.0f;

            physics_world_->update(delta_time);
            physics_event_system_->update(delta_time);
            event_manager_.process_queued_events(delta_time);

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
};

int main()
{
    std::cout << "Portal Demo Simplified Lazy Loading Test" << std::endl;
    std::cout << "Testing basic component creation and lifecycle management" << std::endl;

    SimpleLazyLoadingTest test;
    bool success = test.run_lazy_loading_tests();

    std::cout << "\n"
              << (success ? "🎉 All tests passed! Basic lazy loading is working."
                          : "⚠️  Some tests failed. Please check the implementation.")
              << std::endl;

    return success ? 0 : 1;
}
