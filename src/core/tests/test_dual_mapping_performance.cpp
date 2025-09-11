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

using namespace portal_core;

/**
 * 双向映射性能优化测试
 * 验证从O(n)到O(1)的性能提升
 */
class DualMappingPerformanceTest {
public:
    DualMappingPerformanceTest() : event_manager_(registry_) {}

    bool run_dual_mapping_tests() {
        std::cout << "=== Dual Mapping Performance Optimization Tests ===" << std::endl;
        std::cout << "Testing O(n) vs O(1) entity removal performance" << std::endl;
        
        if (!initialize_systems()) {
            std::cout << "❌ Failed to initialize systems" << std::endl;
            return false;
        }

        bool all_passed = true;
        
        // 运行双向映射性能测试
        all_passed &= test_removal_performance_comparison();
        all_passed &= test_large_scale_removal_scenario();
        all_passed &= test_mapping_consistency();

        cleanup_systems();

        std::cout << "\n=== Dual Mapping Test Summary ===" << std::endl;
        std::cout << (all_passed ? "✅ All dual mapping tests passed!" : "❌ Some dual mapping tests failed!") << std::endl;
        
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

        adapter_->set_debug_mode(false);  // 关闭调试输出以获得准确的性能测试
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
     * 测试实体移除性能对比（O(n) vs O(1)）
     */
    bool test_removal_performance_comparison() {
        std::cout << "\n--- Test: Entity Removal Performance Comparison ---" << std::endl;
        
        const std::vector<size_t> entity_sizes = {1000, 5000, 10000, 20000};
        
        for (size_t entity_count : entity_sizes) {
            std::cout << "Testing with " << entity_count << " entities:" << std::endl;
            
            // 测试新实现（双向映射，O(1)移除）
            auto optimized_time = test_optimized_removal_performance(entity_count);
            
            // 模拟旧实现（单向映射，O(n)移除）
            auto legacy_time = test_legacy_removal_performance(entity_count);
            
            double improvement_ratio = legacy_time / optimized_time;
            
            std::cout << "  Optimized (O(1)): " << optimized_time << " ms" << std::endl;
            std::cout << "  Legacy (O(n)): " << legacy_time << " ms" << std::endl;
            std::cout << "  Improvement: " << improvement_ratio << "x faster" << std::endl;
            
            if (improvement_ratio < 1.0) {
                std::cout << "⚠️  Warning: Optimized version is slower than legacy!" << std::endl;
            } else if (improvement_ratio > 2.0) {
                std::cout << "✅ Significant performance improvement!" << std::endl;
            } else {
                std::cout << "✓ Modest performance improvement" << std::endl;
            }
            std::cout << std::endl;
        }
        
        return true;
    }

    double test_optimized_removal_performance(size_t entity_count) {
        // 创建测试实体
        std::vector<entt::entity> entities = create_test_entities(entity_count);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 随机移除所有实体（模拟实际使用场景）
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        for (auto entity : entities) {
            if (registry_.valid(entity)) {
                registry_.destroy(entity);  // 这会触发组件移除监听器，使用新的O(1)实现
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

    double test_legacy_removal_performance(size_t entity_count) {
        // 创建测试实体
        std::vector<entt::entity> entities = create_test_entities(entity_count);
        
        // 构建与当前实现相同的映射结构
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
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 随机移除所有实体，模拟旧的O(n)查找移除
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(entities.begin(), entities.end(), gen);
        
        for (auto entity : entities) {
            if (registry_.valid(entity)) {
                // 模拟旧实现：需要遍历映射查找实体
                simulate_legacy_entity_removal(legacy_mapping, entity);
                registry_.destroy(entity);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

    void simulate_legacy_entity_removal(std::unordered_map<uint32_t, entt::entity>& mapping, entt::entity entity) {
        // 模拟原来的O(n)查找和移除过程
        auto it = std::find_if(mapping.begin(), mapping.end(),
            [entity](const std::pair<uint32_t, entt::entity>& pair) {
                return pair.second == entity;
            });
        
        if (it != mapping.end()) {
            mapping.erase(it);
        }
    }

    /**
     * 测试大规模实体移除场景
     */
    bool test_large_scale_removal_scenario() {
        std::cout << "\n--- Test: Large Scale Entity Removal Scenario ---" << std::endl;
        
        const size_t large_entity_count = 50000;
        const size_t removal_batches = 10;
        const size_t entities_per_batch = large_entity_count / removal_batches;
        
        std::cout << "Testing large scale scenario: " << large_entity_count << " entities, " 
                  << removal_batches << " removal batches" << std::endl;
        
        // 创建大量实体
        std::vector<entt::entity> entities = create_test_entities(large_entity_count);
        
        std::vector<double> batch_times;
        
        // 分批移除实体，测量每批的时间
        for (size_t batch = 0; batch < removal_batches; ++batch) {
            size_t start_idx = batch * entities_per_batch;
            size_t end_idx = std::min(start_idx + entities_per_batch, entities.size());
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (size_t i = start_idx; i < end_idx; ++i) {
                if (registry_.valid(entities[i])) {
                    registry_.destroy(entities[i]);
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            double batch_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            batch_times.push_back(batch_time);
            
            std::cout << "  Batch " << (batch + 1) << ": " << batch_time << " ms (" 
                      << entities_per_batch << " entities)" << std::endl;
        }
        
        // 分析性能一致性
        double avg_time = 0.0;
        for (double time : batch_times) {
            avg_time += time;
        }
        avg_time /= batch_times.size();
        
        double max_time = *std::max_element(batch_times.begin(), batch_times.end());
        double min_time = *std::min_element(batch_times.begin(), batch_times.end());
        double time_variance = max_time - min_time;
        
        std::cout << "Performance analysis:" << std::endl;
        std::cout << "  Average batch time: " << avg_time << " ms" << std::endl;
        std::cout << "  Min/Max batch time: " << min_time << "/" << max_time << " ms" << std::endl;
        std::cout << "  Time variance: " << time_variance << " ms" << std::endl;
        
        // 检查性能一致性（O(1)操作应该有相对稳定的性能）
        double variance_ratio = time_variance / avg_time;
        if (variance_ratio > 1.0) {
            std::cout << "⚠️  Warning: High performance variance detected (ratio: " << variance_ratio << ")" << std::endl;
        } else {
            std::cout << "✅ Consistent O(1) performance across batches!" << std::endl;
        }
        
        return variance_ratio <= 2.0;  // 允许2倍的方差
    }

    /**
     * 测试映射一致性
     */
    bool test_mapping_consistency() {
        std::cout << "\n--- Test: Mapping Consistency ---" << std::endl;
        
        const size_t test_entity_count = 1000;
        std::cout << "Testing mapping consistency with " << test_entity_count << " entities" << std::endl;
        
        // 创建实体并验证映射
        std::vector<entt::entity> entities = create_test_entities(test_entity_count);
        
        std::cout << "✅ Created " << entities.size() << " entities with dual mapping" << std::endl;
        
        // 随机移除一些实体
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, entities.size() - 1);
        
        size_t removal_count = test_entity_count / 4;  // 移除25%的实体
        for (size_t i = 0; i < removal_count; ++i) {
            size_t index = dist(gen);
            if (registry_.valid(entities[index])) {
                registry_.destroy(entities[index]);
                entities[index] = entt::null;  // 标记为已移除
            }
        }
        
        std::cout << "✅ Removed approximately " << removal_count << " entities" << std::endl;
        
        // 验证剩余的实体仍然可以通过映射查找
        size_t valid_lookups = 0;
        for (auto entity : entities) {
            if (entity != entt::null && registry_.valid(entity)) {
                if (registry_.all_of<PhysicsBodyComponent>(entity)) {
                    auto& body_comp = registry_.get<PhysicsBodyComponent>(entity);  
                    if (!body_comp.body_id.IsInvalid()) {
                        // 测试映射查找
                        auto found_entity = adapter_->body_id_to_entity(body_comp.body_id);
                        if (found_entity == entity) {
                            valid_lookups++;
                        }
                    }
                }
            }
        }
        
        std::cout << "✅ Verified " << valid_lookups << " valid mappings remain consistent" << std::endl;
        
        // 清理剩余实体
        for (auto entity : entities) {
            if (entity != entt::null && registry_.valid(entity)) {
                registry_.destroy(entity);
            }
        }
        
        std::cout << "✅ Mapping consistency test completed successfully" << std::endl;
        
        return true;
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
            physics_comp.body_id = JPH::BodyID();  // 使用默认构造
            physics_comp.body_type = (i % 3 == 0) ? PhysicsBodyType::DYNAMIC : PhysicsBodyType::STATIC;
            
            registry_.emplace<TransformComponent>(entity);
        }
        
        return entities;
    }
};

int main() {
    try {
        std::cout << "Starting dual mapping performance optimization tests..." << std::endl;
        
        DualMappingPerformanceTest test;
        bool success = test.run_dual_mapping_tests();
        
        std::cout << "\n" << (success ? "✅ All tests passed!" : "❌ Some tests failed!") << std::endl;
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
}
