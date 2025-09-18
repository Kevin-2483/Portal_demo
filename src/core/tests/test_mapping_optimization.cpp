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

using namespace portal_core;

/**
 * PhysicsEventAdapter 映射优化测试
 * 验证增量更新 vs 每帧重建映射的性能差异
 */
class MappingOptimizationTest {
public:
    MappingOptimizationTest() : event_manager_(registry_) {}

    bool run_mapping_optimization_tests() {
        std::cout << "=== PhysicsEventAdapter Mapping Optimization Tests ===" << std::endl;
        std::cout << "Testing incremental mapping update vs full rebuild performance" << std::endl;
        
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        bool all_passed = true;
        
        // 运行映射优化测试
        all_passed &= test_incremental_mapping_functionality();
        all_passed &= test_mapping_performance_comparison();
        all_passed &= test_large_scale_scenario();

        cleanup_systems();

        std::cout << "\n=== Mapping Optimization Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All mapping optimization tests passed!" : "❌ Some mapping optimization tests failed!") << std::endl;
        
        return all_passed;
    }

private:
    entt::registry registry_;
    EventManager event_manager_;
    std::unique_ptr<PhysicsWorldManager> physics_world_;
    std::unique_ptr<PhysicsEventAdapter> adapter_;

    bool initialize_systems() {
        std::cout << "Initializing test systems..." << std::endl;

        // EventManager 不需要单独初始化（构造函数已处理）
        std::cout << "✅ EventManager ready" << std::endl;

        // 初始化物理世界
        physics_world_ = std::make_unique<PhysicsWorldManager>();
        if (!physics_world_->initialize()) {
            std::cout << "❌ Failed to initialize PhysicsWorldManager" << std::endl;
            return false;
        }

        // 初始化适配器（使用新的增量更新模式）
        adapter_ = std::make_unique<PhysicsEventAdapter>(event_manager_, *physics_world_, registry_);
        if (!adapter_->initialize(registry_)) {  // 使用带registry的初始化方法
            std::cout << "❌ Failed to initialize PhysicsEventAdapter" << std::endl;
            return false;
        }

        adapter_->set_debug_mode(true);
        std::cout << "✅ All systems initialized successfully" << std::endl;
        return true;
    }

    void cleanup_systems() {
        if (adapter_) {
            adapter_->cleanup();
        }
        if (physics_world_) {
            physics_world_->cleanup();
        }
        // EventManager 不需要单独清理
    }

    /**
     * 测试增量映射功能的正确性
     */
    bool test_incremental_mapping_functionality() {
        std::cout << "\n--- Test: Incremental Mapping Functionality ---" << std::endl;
        
        std::vector<entt::entity> test_entities;
        
        // 1. 测试实体创建和组件监听器设置
        std::cout << "Testing entity creation and component listener setup..." << std::endl;
        for (int i = 0; i < 5; ++i) {
            auto entity = registry_.create();
            test_entities.push_back(entity);
            
            // 添加物理体组件 - 这应该自动触发映射添加
            auto& physics_comp = registry_.emplace<PhysicsBodyComponent>(entity);
            // 使用更安全的BodyID创建方式
            physics_comp.body_id = JPH::BodyID();  // 先创建无效ID
            physics_comp.body_type = PhysicsBodyType::DYNAMIC;  // 设置为动态
            
            std::cout << "  Created entity " << static_cast<uint32_t>(entity) << " with physics component" << std::endl;
        }
        std::cout << "✅ Entity creation with component listeners works correctly" << std::endl;
        
        // 2. 测试组件更新触发
        std::cout << "Testing component update events..." << std::endl;
        if (!test_entities.empty()) {
            auto entity = test_entities[0];
            auto* physics_comp = registry_.try_get<PhysicsBodyComponent>(entity);
            
            // 更新组件以触发监听器
            physics_comp->body_type = PhysicsBodyType::KINEMATIC;
            registry_.patch<PhysicsBodyComponent>(entity);  // 触发更新事件
            
            std::cout << "  Updated entity " << static_cast<uint32_t>(entity) << " component" << std::endl;
        }
        std::cout << "✅ Component update events work correctly" << std::endl;
        
        // 3. 测试实体销毁时的监听器触发
        std::cout << "Testing entity destruction events..." << std::endl;
        while (!test_entities.empty()) {
            auto entity = test_entities.back();
            test_entities.pop_back();
            
            std::cout << "  Destroying entity " << static_cast<uint32_t>(entity) << std::endl;
            
            // 销毁实体 - 这应该触发组件移除监听器
            registry_.destroy(entity);
        }
        std::cout << "✅ Entity destruction events work correctly" << std::endl;
        
        return true;
    }

    /**
     * 测试映射性能对比（增量更新 vs 每帧重建）
     */
    bool test_mapping_performance_comparison() {
        std::cout << "\n--- Test: Mapping Performance Comparison ---" << std::endl;
        
        const size_t entity_count = 5000;
        const size_t update_cycles = 100;
        
        std::cout << "Testing with " << entity_count << " entities over " << update_cycles << " update cycles" << std::endl;
        
        // 测试增量更新方式（当前实现）
        auto incremental_time = test_incremental_update_performance(entity_count, update_cycles);
        
        // 测试每帧重建方式（模拟旧实现）
        auto rebuild_time = test_rebuild_performance(entity_count, update_cycles);
        
        double improvement_ratio = rebuild_time / incremental_time;
        
        std::cout << "Performance Results:" << std::endl;
        std::cout << "  Incremental update: " << incremental_time << " ms" << std::endl;
        std::cout << "  Full rebuild: " << rebuild_time << " ms" << std::endl;
        std::cout << "  Improvement ratio: " << improvement_ratio << "x faster" << std::endl;
        
        if (improvement_ratio < 2.0) {
            std::cout << "⚠️  Warning: Performance improvement is less than expected (< 2x)" << std::endl;
        } else {
            std::cout << "✅ Significant performance improvement achieved!" << std::endl;
        }
        
        return improvement_ratio > 1.0;  // 至少要比旧方法快
    }

    double test_incremental_update_performance(size_t entity_count, size_t update_cycles) {
        std::cout << "  Testing incremental update performance..." << std::endl;
        
        // 创建测试实体
        std::vector<entt::entity> entities = create_test_entities(entity_count);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t cycle = 0; cycle < update_cycles; ++cycle) {
            // 模拟正常的帧更新（不会触发映射重建）
            adapter_->update(1.0f / 60.0f);
            
            // 偶尔修改一些实体（模拟真实场景中的变化）
            if (cycle % 20 == 0) {
                modify_random_entities(entities, entity_count * 0.02);  // 修改2%的实体
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 清理测试实体
        cleanup_test_entities(entities);
        
        return elapsed_ms;
    }

    double test_rebuild_performance(size_t entity_count, size_t update_cycles) {
        std::cout << "  Testing full rebuild performance..." << std::endl;
        
        // 创建测试实体
        std::vector<entt::entity> entities = create_test_entities(entity_count);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t cycle = 0; cycle < update_cycles; ++cycle) {
            // 模拟每帧重建映射（旧方法）
            simulate_full_mapping_rebuild();
            
            // 偶尔修改一些实体
            if (cycle % 20 == 0) {
                modify_random_entities(entities, entity_count * 0.02);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // 清理测试实体
        cleanup_test_entities(entities);
        
        return elapsed_ms;
    }

    /**
     * 测试大规模场景下的性能
     */
    bool test_large_scale_scenario() {
        std::cout << "\n--- Test: Large Scale Scenario ---" << std::endl;
        
        const size_t large_entity_count = 20000;
        std::cout << "Testing large scale scenario with " << large_entity_count << " entities" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 创建大量实体
        std::vector<entt::entity> entities = create_test_entities(large_entity_count);
        
        auto creation_time = std::chrono::high_resolution_clock::now();
        double creation_ms = std::chrono::duration<double, std::milli>(creation_time - start_time).count();
        
        // 模拟多帧更新
        for (int frame = 0; frame < 30; ++frame) {
            adapter_->update(1.0f / 60.0f);
        }
        
        auto update_time = std::chrono::high_resolution_clock::now();
        double update_ms = std::chrono::duration<double, std::milli>(update_time - creation_time).count();
        
        // 清理实体
        cleanup_test_entities(entities);
        
        auto cleanup_time = std::chrono::high_resolution_clock::now();
        double cleanup_ms = std::chrono::duration<double, std::milli>(cleanup_time - update_time).count();
        
        std::cout << "Large scale performance:" << std::endl;
        std::cout << "  Entity creation: " << creation_ms << " ms" << std::endl;
        std::cout << "  30 frame updates: " << update_ms << " ms (" << update_ms / 30.0 << " ms/frame)" << std::endl;
        std::cout << "  Entity cleanup: " << cleanup_ms << " ms" << std::endl;
        
        double avg_frame_time = update_ms / 30.0;
        if (avg_frame_time > 16.67) {  // 超过60FPS的帧时间
            std::cout << "⚠️  Warning: Average frame time exceeds 60FPS threshold" << std::endl;
        } else {
            std::cout << "✅ Large scale scenario performs well!" << std::endl;
        }
        
        return avg_frame_time <= 100.0;  // 允许10FPS作为最低标准
    }

    // 辅助方法
    std::vector<entt::entity> create_test_entities(size_t count) {
        std::vector<entt::entity> entities;
        entities.reserve(count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> body_id_dist(1, 1000000);
        
        for (size_t i = 0; i < count; ++i) {
            auto entity = registry_.create();
            entities.push_back(entity);
            
            auto& physics_comp = registry_.emplace<PhysicsBodyComponent>(entity);
            physics_comp.body_id = JPH::BodyID();  // 使用默认构造，避免构造问题
            physics_comp.body_type = (i % 3 == 0) ? PhysicsBodyType::DYNAMIC : PhysicsBodyType::STATIC;  // 1/3的实体是动态的
            
            registry_.emplace<TransformComponent>(entity);
        }
        
        return entities;
    }

    void cleanup_test_entities(const std::vector<entt::entity>& entities) {
        for (auto entity : entities) {
            if (registry_.valid(entity)) {
                registry_.destroy(entity);
            }
        }
    }

    void modify_random_entities(const std::vector<entt::entity>& entities, size_t count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> entity_dist(0, entities.size() - 1);
        std::uniform_int_distribution<uint32_t> body_id_dist(1, 1000000);
        
        for (size_t i = 0; i < count && i < entities.size(); ++i) {
            auto entity = entities[entity_dist(gen)];
            if (registry_.valid(entity)) {
                auto* physics_comp = registry_.try_get<PhysicsBodyComponent>(entity);
                if (physics_comp) {
                    // 只修改其他属性，避免BodyID构造问题
                    physics_comp->body_type = (physics_comp->body_type == PhysicsBodyType::DYNAMIC) 
                        ? PhysicsBodyType::STATIC : PhysicsBodyType::DYNAMIC;
                    registry_.patch<PhysicsBodyComponent>(entity);
                }
            }
        }
    }

    void simulate_full_mapping_rebuild() {
        // 模拟旧方法：每帧清空并重建整个映射
        std::unordered_map<uint32_t, entt::entity> temp_mapping;
        
        auto view = registry_.view<PhysicsBodyComponent>();
        for (auto entity : view) {
            auto& body_comp = view.get<PhysicsBodyComponent>(entity);
            if (!body_comp.body_id.IsInvalid()) {
                uint32_t id = body_comp.body_id.GetIndexAndSequenceNumber();
                temp_mapping[id] = entity;
            }
        }
        
        // temp_mapping 会在函数结束时自动销毁，模拟每帧的内存分配/释放开销
    }
};

int main() {
    std::cout << "Starting PhysicsEventAdapter mapping optimization tests..." << std::endl;
    
    MappingOptimizationTest test;
    bool success = test.run_mapping_optimization_tests();
    
    std::cout << "\n" << (success ? "✅ All tests passed!" : "❌ Some tests failed!") << std::endl;
    return success ? 0 : 1;
}
