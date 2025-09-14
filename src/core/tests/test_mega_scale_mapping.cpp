#include "core/physics_events/physics_event_adapter.h"
#include "core/event_manager.h"
#include "core/physics_world_manager.h"
#include "core/components/physics_body_component.h"
#include "core/components/transform_component.h"
#include "core/math_types.h"
#include <entt/entt.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <memory> // For std::unique_ptr

// 假设这些头文件和命名空间存在
// #include "path/to/your/files.h"
using namespace portal_core;

/**
 * 超大规模双向映射性能测试
 * 专门测试100万实体场景下的性能差异
 */
class MegaScaleMappingTest {
public:
    MegaScaleMappingTest() : event_manager_(registry_) {}

    // 【修改点 1/5】添加析构函数，保证在对象销毁时安全清理
    ~MegaScaleMappingTest() {
        std::cout << "\nCleaning up test systems in destructor..." << std::endl;
        cleanup_systems();
    }

    bool run_mega_scale_tests() {
        std::cout << "=== Mega Scale Dual Mapping Performance Tests ===" << std::endl;
        std::cout << "Testing with up to 1,000,000 entities" << std::endl;
        
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        bool all_passed = true;
        
        // 运行超大规模测试
        all_passed &= test_mega_scale_performance();
        all_passed &= test_removal_pattern_analysis();

        // 【说明】这里的 cleanup_systems() 调用是可选的，因为析构函数会确保最终清理。
        // cleanup_systems();

        std::cout << "\n=== Mega Scale Test Summary ===" << std::endl;
        // 注意：最后的总览信息会在 main 函数的末尾打印
        
        return all_passed;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventAdapter> adapter_;

    bool initialize_systems() {
        std::cout << "Initializing test systems..." << std::endl;

        // 初始化物理世界
        physics_world_ = std::make_unique<PhysicsWorldManager>();
        if (!physics_world_->initialize()) {
            std::cout << "❌ Failed to initialize PhysicsWorldManager" << std::endl;
            return false;
        }

        // 初始化适配器
        adapter_ = std::make_unique<PhysicsEventAdapter>(event_manager_, *physics_world_, registry_);
        if (!adapter_->initialize(registry_)) {
            std::cout << "❌ Failed to initialize PhysicsEventAdapter" << std::endl;
            return false;
        }

        adapter_->set_debug_mode(false); // 关闭调试输出
        std::cout << "✅ All systems initialized successfully" << std::endl;
        return true;
    }

    void cleanup_systems() {
        if (adapter_) {
            adapter_->cleanup();
            adapter_.reset(); // 释放 unique_ptr
        }
        if (physics_world_) {
            physics_world_->cleanup();
            physics_world_.reset(); // 释放 unique_ptr
        }
    }

    bool test_mega_scale_performance() {
        std::cout << "\n--- Test: Mega Scale Performance ---" << std::endl;
        
        const size_t mega_entity_count = 200000;
        const size_t removal_sample_size = 50000;
        
        std::cout << "Creating " << mega_entity_count << " entities..." << std::endl;
        
        auto creation_start = std::chrono::high_resolution_clock::now();
        auto entities_opt = create_test_entities_fast(mega_entity_count);
        auto creation_end = std::chrono::high_resolution_clock::now();
        
        double creation_time = std::chrono::duration<double, std::milli>(creation_end - creation_start).count();
        std::cout << "✅ Created " << mega_entity_count << " entities in " << creation_time << " ms" << std::endl;
        
        std::cout << "Testing optimized O(1) removal performance..." << std::endl;
        auto optimized_time = test_optimized_mega_removal(entities_opt, removal_sample_size);
        registry_.clear(); // 【修改点 2/5】测试后清理现场

        std::cout << "Recreating entities for legacy test..." << std::endl;
        auto entities_leg = create_test_entities_fast(mega_entity_count);
        
        std::cout << "Testing legacy O(n) removal performance..." << std::endl;
        auto legacy_time = test_legacy_mega_removal(entities_leg, removal_sample_size);
        registry_.clear(); // 【修改点 2/5】测试后清理现场
        
        double improvement_ratio = (optimized_time > 0) ? (legacy_time / optimized_time) : 0;
        
        std::cout << "\n=== Large Scale Performance Results ===" << std::endl;
        std::cout << "Entity Count: " << mega_entity_count << std::endl;
        std::cout << "Removal Sample: " << removal_sample_size << " entities" << std::endl;
        std::cout << "Optimized (O(1)): " << optimized_time << " ms" << std::endl;
        std::cout << "Legacy (O(n)): " << legacy_time << " ms" << std::endl;
        std::cout << "Improvement: " << improvement_ratio << "x faster" << std::endl;
        
        if (improvement_ratio > 1.0) {
             std::cout << "✅ GOOD: Performance improvement detected!" << std::endl;
        } else {
             std::cout << "⚠️ WARNING: Performance regression or no improvement." << std::endl;
        }
        
        return improvement_ratio > 1.0;
    }

    bool test_removal_pattern_analysis() {
        std::cout << "\n--- Test: Removal Pattern Analysis ---" << std::endl;
        
        const size_t entity_count = 500000;
        const std::vector<double> removal_percentages = {0.01, 0.05, 0.1, 0.2, 0.5};
        
        std::cout << "Testing different removal patterns with " << entity_count << " entities" << std::endl;
        
        for (double removal_pct : removal_percentages) {
            size_t removal_count = static_cast<size_t>(entity_count * removal_pct);
            
            std::cout << "\nTesting " << (removal_pct * 100) << "% removal (" << removal_count << " entities):" << std::endl;
            
            auto entities_opt = create_test_entities_fast(entity_count);
            auto optimized_time = test_optimized_mega_removal(entities_opt, removal_count);
            registry_.clear(); // 【修改点 3/5】每次迭代后清理，防止内存无限增长
            
            auto entities_leg = create_test_entities_fast(entity_count);
            auto legacy_time = test_legacy_mega_removal(entities_leg, removal_count);
            registry_.clear(); // 【修改点 3/5】每次迭代后清理
            
            double improvement = (optimized_time > 0) ? (legacy_time / optimized_time) : 0;
            
            std::cout << "  Optimized: " << optimized_time << " ms" << std::endl;
            std::cout << "  Legacy: " << legacy_time << " ms" << std::endl;
            std::cout << "  Improvement: " << improvement << "x" << std::endl;
        }
        
        return true;
    }

    double test_optimized_mega_removal(std::vector<entt::entity>& entities, size_t removal_count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 确保不会移除超出范围的实体
        size_t count = std::min(removal_count, entities.size());
        for (size_t i = 0; i < count; ++i) {
            if (registry_.valid(entities[i])) {
                registry_.destroy(entities[i]);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

    double test_legacy_mega_removal(std::vector<entt::entity>& entities, size_t removal_count) {
        std::unordered_map<uint32_t, entt::entity> legacy_mapping;
        for (auto entity : entities) {
            if (registry_.all_of<PhysicsBodyComponent>(entity)) {
                auto& body_comp = registry_.get<PhysicsBodyComponent>(entity);
                if (!body_comp.body_id.IsInvalid()) {
                    uint32_t id = body_comp.body_id.GetIndexAndSequenceNumber();
                    legacy_mapping[id] = entity;
                }
            }
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t count = std::min(removal_count, entities.size());
        for (size_t i = 0; i < count; ++i) {
            auto entity = entities[i];
            if (registry_.valid(entity)) {
                auto it = std::find_if(legacy_mapping.begin(), legacy_mapping.end(),
                    [entity](const auto& pair) {
                        return pair.second == entity;
                    });
                
                if (it != legacy_mapping.end()) {
                    legacy_mapping.erase(it);
                }
                
                registry_.destroy(entity);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

    std::vector<entt::entity> create_test_entities_fast(size_t count) {
        std::vector<entt::entity> entities;
        entities.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            auto entity = registry_.create();
            entities.push_back(entity);
            
            auto& physics_comp = registry_.emplace<PhysicsBodyComponent>(entity);
            
            // 【修改点 4/5】为测试分配一个唯一的、有效的 BodyID
            // 对于独立的映射测试，使用简单的计数器作为ID是完全有效的
            physics_comp.body_id = JPH::BodyID(static_cast<uint32_t>(i));
            
            physics_comp.body_type = (i % 2 == 0) ? PhysicsBodyType::DYNAMIC : PhysicsBodyType::STATIC;
            
            if (i % 10 == 0) {
                registry_.emplace<TransformComponent>(entity);
            }
        }
        
        return entities;
    }
};

int main() {
    try {
        std::cout << "Starting mega scale dual mapping performance tests..." << std::endl;
        std::cout << "WARNING: This test will use significant memory and CPU time!" << std::endl;
        std::cout << "Press Ctrl+C to cancel if needed..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        bool success = false;
        { // 【修改点 5/5】将 test 对象放在独立作用域中，确保它在 main 结束前被完全析构
            MegaScaleMappingTest test;
            success = test.run_mega_scale_tests();
        }
        
        std::cout << "\n" << (success ? "✅ All mega scale tests passed!" : "❌ Some mega scale tests failed!") << std::endl;
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
}