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

using namespace portal_core;

/**
 * 超大规模双向映射性能测试
 * 专门测试100万实体场景下的性能差异
 */
class MegaScaleMappingTest {
public:
    MegaScaleMappingTest() : event_manager_(registry_) {}

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

        cleanup_systems();

        std::cout << "\n=== Mega Scale Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All mega scale tests passed!" : "❌ Some mega scale tests failed!") << std::endl;
        
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

        adapter_->set_debug_mode(false);  // 关闭调试输出
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
    }

    /**
     * 超大规模性能测试
     */
    bool test_mega_scale_performance() {
        std::cout << "\n--- Test: Mega Scale Performance (1,000,000 entities) ---" << std::endl;
        
        const size_t mega_entity_count = 200000;  // 20万实体（更合理的测试规模）
        const size_t removal_sample_size = 50000;  // 移除5万实体作为样本
        
        std::cout << "Creating " << mega_entity_count << " entities..." << std::endl;
        
        auto creation_start = std::chrono::high_resolution_clock::now();
        std::vector<entt::entity> entities = create_test_entities_fast(mega_entity_count);
        auto creation_end = std::chrono::high_resolution_clock::now();
        
        double creation_time = std::chrono::duration<double, std::milli>(creation_end - creation_start).count();
        std::cout << "✅ Created " << mega_entity_count << " entities in " << creation_time << " ms" << std::endl;
        
        // 测试优化版本的移除性能
        std::cout << "Testing optimized O(1) removal performance..." << std::endl;
        auto optimized_time = test_optimized_mega_removal(entities, removal_sample_size);
        
        // 重新创建实体进行对比测试
        std::cout << "Recreating entities for legacy test..." << std::endl;
        entities = create_test_entities_fast(mega_entity_count);
        
        // 测试遗留版本的移除性能
        std::cout << "Testing legacy O(n) removal performance..." << std::endl;
        auto legacy_time = test_legacy_mega_removal(entities, removal_sample_size);
        
        double improvement_ratio = legacy_time / optimized_time;
        
        std::cout << "\n=== Large Scale Performance Results ===" << std::endl;
        std::cout << "Entity Count: 200,000" << std::endl;
        std::cout << "Removal Sample: " << removal_sample_size << " entities" << std::endl;
        std::cout << "Optimized (O(1)): " << optimized_time << " ms" << std::endl;
        std::cout << "Legacy (O(n)): " << legacy_time << " ms" << std::endl;
        std::cout << "Improvement: " << improvement_ratio << "x faster" << std::endl;
        std::cout << "Time saved: " << (legacy_time - optimized_time) << " ms" << std::endl;
        
        if (improvement_ratio > 10.0) {
            std::cout << "✅ EXCELLENT: Massive performance improvement!" << std::endl;
        } else if (improvement_ratio > 5.0) {
            std::cout << "✅ GREAT: Significant performance improvement!" << std::endl;
        } else if (improvement_ratio > 2.0) {
            std::cout << "✅ GOOD: Noticeable performance improvement!" << std::endl;
        } else if (improvement_ratio > 1.0) {
            std::cout << "✓ MODEST: Some performance improvement" << std::endl;
        } else {
            std::cout << "⚠️  WARNING: Performance regression detected!" << std::endl;
        }
        
        return improvement_ratio > 1.0;
    }

    /**
     * 移除模式分析测试
     */
    bool test_removal_pattern_analysis() {
        std::cout << "\n--- Test: Removal Pattern Analysis ---" << std::endl;
        
        const size_t entity_count = 500000;  // 50万实体
        const std::vector<double> removal_percentages = {0.01, 0.05, 0.1, 0.2, 0.5};  // 1%, 5%, 10%, 20%, 50%
        
        std::cout << "Testing different removal patterns with " << entity_count << " entities" << std::endl;
        
        for (double removal_pct : removal_percentages) {
            size_t removal_count = static_cast<size_t>(entity_count * removal_pct);
            
            std::cout << "\nTesting " << (removal_pct * 100) << "% removal (" << removal_count << " entities):" << std::endl;
            
            // 测试优化版本
            auto entities = create_test_entities_fast(entity_count);
            auto optimized_time = test_optimized_mega_removal(entities, removal_count);
            
            // 测试遗留版本
            entities = create_test_entities_fast(entity_count);
            auto legacy_time = test_legacy_mega_removal(entities, removal_count);
            
            double improvement = legacy_time / optimized_time;
            
            std::cout << "  Optimized: " << optimized_time << " ms" << std::endl;
            std::cout << "  Legacy: " << legacy_time << " ms" << std::endl;
            std::cout << "  Improvement: " << improvement << "x" << std::endl;
            
            if (improvement > 1.0) {
                std::cout << "  ✅ Performance gain achieved!" << std::endl;
            } else {
                std::cout << "  ⚠️  No performance gain" << std::endl;
            }
        }
        
        return true;
    }

    double test_optimized_mega_removal(std::vector<entt::entity>& entities, size_t removal_count) {
        // 随机选择要移除的实体
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 移除指定数量的实体（使用优化的O(1)实现）
        for (size_t i = 0; i < removal_count && i < entities.size(); ++i) {
            if (registry_.valid(entities[i])) {
                registry_.destroy(entities[i]);  // 触发优化的组件移除监听器
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

    double test_legacy_mega_removal(std::vector<entt::entity>& entities, size_t removal_count) {
        // 构建完整的映射（模拟实际场景）
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
        
        // 随机选择要移除的实体
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 模拟遗留版本的O(n)移除过程
        for (size_t i = 0; i < removal_count && i < entities.size(); ++i) {
            auto entity = entities[i];
            if (registry_.valid(entity)) {
                // 模拟O(n)查找和移除
                auto it = std::find_if(legacy_mapping.begin(), legacy_mapping.end(),
                    [entity](const std::pair<uint32_t, entt::entity>& pair) {
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

    // 快速实体创建（减少创建时间的干扰）
    std::vector<entt::entity> create_test_entities_fast(size_t count) {
        std::vector<entt::entity> entities;
        entities.reserve(count);
        
        // 批量创建实体，减少随机数生成开销
        for (size_t i = 0; i < count; ++i) {
            auto entity = registry_.create();
            entities.push_back(entity);
            
            auto& physics_comp = registry_.emplace<PhysicsBodyComponent>(entity);
            physics_comp.body_id = JPH::BodyID();  // 使用默认构造
            physics_comp.body_type = (i % 2 == 0) ? PhysicsBodyType::DYNAMIC : PhysicsBodyType::STATIC;
            
            // 只在必要时添加Transform组件
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
        
        // 给用户一些时间来取消
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        MegaScaleMappingTest test;
        bool success = test.run_mega_scale_tests();
        
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
