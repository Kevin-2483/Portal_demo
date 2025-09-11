#include "core/event_pool_and_concurrency.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace portal_core;

// 简单的测试组件
struct TestComponent {
    int data[10];  // 稍大一些的对象来模拟真实情况
    TestComponent() { for(int& i : data) i = 0; }
    TestComponent(int val) { for(int& i : data) i = val; }
};

// 性能测试：大容量池的 shrink_to_fit 操作
void test_large_pool_shrink_performance() {
    std::cout << "\n=== Large Pool Shrink Performance Test ===" << std::endl;
    
    const size_t LARGE_POOL_SIZE = 10000;
    const size_t TARGET_SIZE = 1000;
    
    EventPool<TestComponent> pool(0, LARGE_POOL_SIZE + 5000);
    
    // 预分配大容量池
    auto start_reserve = std::chrono::high_resolution_clock::now();
    pool.reserve(LARGE_POOL_SIZE);
    auto end_reserve = std::chrono::high_resolution_clock::now();
    
    auto reserve_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_reserve - start_reserve);
    std::cout << "Reserve " << LARGE_POOL_SIZE << " objects: " << reserve_duration.count() << " microseconds" << std::endl;
    
    auto stats_before = pool.get_statistics();
    std::cout << "Before shrink - Available: " << stats_before.available_count 
              << ", Total: " << stats_before.total_objects << std::endl;
    
    // 测试 shrink_to_fit 性能
    auto start_shrink = std::chrono::high_resolution_clock::now();
    pool.shrink_to_fit(TARGET_SIZE);
    auto end_shrink = std::chrono::high_resolution_clock::now();
    
    auto shrink_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_shrink - start_shrink);
    std::cout << "Shrink from " << LARGE_POOL_SIZE << " to " << TARGET_SIZE 
              << " objects: " << shrink_duration.count() << " microseconds" << std::endl;
    
    auto stats_after = pool.get_statistics();
    std::cout << "After shrink - Available: " << stats_after.available_count 
              << ", Total: " << stats_after.total_objects << std::endl;
    
    // 验证正确性
    if (stats_after.available_count == TARGET_SIZE) {
        std::cout << "✅ Shrink operation completed correctly" << std::endl;
    } else {
        std::cout << "❌ Shrink operation failed!" << std::endl;
    }
}

// 性能测试：大量 release 操作
void test_mass_release_performance() {
    std::cout << "\n=== Mass Release Performance Test ===" << std::endl;
    
    const size_t POOL_SIZE = 5000;
    EventPool<TestComponent> pool(0, POOL_SIZE + 1000);
    pool.reserve(POOL_SIZE);
    
    // 获取大量对象
    std::vector<EventPool<TestComponent>::unique_obj_ptr> objects;
    objects.reserve(POOL_SIZE);
    
    auto start_acquire = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < POOL_SIZE; ++i) {
        auto obj = pool.acquire(static_cast<int>(i));
        if (obj) {
            objects.push_back(std::move(obj));
        }
    }
    auto end_acquire = std::chrono::high_resolution_clock::now();
    
    auto acquire_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_acquire - start_acquire);
    std::cout << "Acquire " << objects.size() << " objects: " << acquire_duration.count() << " microseconds" << std::endl;
    
    // 测试批量释放性能（通过智能指针自动释放）
    auto start_release = std::chrono::high_resolution_clock::now();
    objects.clear();  // 这会触发所有对象的 release
    auto end_release = std::chrono::high_resolution_clock::now();
    
    auto release_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_release - start_release);
    std::cout << "Release " << POOL_SIZE << " objects: " << release_duration.count() << " microseconds" << std::endl;
    
    auto final_stats = pool.get_statistics();
    std::cout << "Final stats - Available: " << final_stats.available_count 
              << ", Active: " << final_stats.active_count 
              << ", Reuse ratio: " << final_stats.reuse_ratio << std::endl;
}

// 对比测试：验证 O(1) vs O(N) 性能差异
void test_complexity_comparison() {
    std::cout << "\n=== Complexity Comparison Test ===" << std::endl;
    
    // 测试不同池大小下的性能表现
    std::vector<size_t> pool_sizes = {100, 500, 1000, 2000, 5000};
    
    for (size_t pool_size : pool_sizes) {
        EventPool<TestComponent> pool(0, pool_size + 1000);
        pool.reserve(pool_size);
        
        // 获取并立即释放对象，测试 release 性能
        const size_t OPERATIONS = 1000;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < OPERATIONS; ++i) {
            auto obj = pool.acquire(static_cast<int>(i));
            // obj 在作用域结束时自动释放
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_time = static_cast<double>(duration.count()) / OPERATIONS;
        
        std::cout << "Pool size: " << pool_size 
                  << ", " << OPERATIONS << " acquire+release operations: " 
                  << duration.count() << " μs (avg: " << avg_time << " μs/op)" << std::endl;
    }
    
    std::cout << "✅ 如果优化有效，平均时间应该与池大小无关（O(1) 性能）" << std::endl;
}

int main() {
    std::cout << "=== EventPool Performance Benchmark ===" << std::endl;
    
    test_large_pool_shrink_performance();
    test_mass_release_performance();
    test_complexity_comparison();
    
    std::cout << "\n=== Performance Test Completed ===" << std::endl;
    return 0;
}
