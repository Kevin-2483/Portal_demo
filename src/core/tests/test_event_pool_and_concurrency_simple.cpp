#include "../event_pool_and_concurrency.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>

using namespace portal_core;

// 简单的测试宏
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << message << " at line " << __LINE__ << std::endl; \
        return false; \
    } else { \
        std::cout << "PASSED: " << message << std::endl; \
    }

#define TEST_ASSERT_EQ(expected, actual, message) \
    if ((expected) != (actual)) { \
        std::cerr << "FAILED: " << message << " (expected: " << expected << ", actual: " << actual << ") at line " << __LINE__ << std::endl; \
        return false; \
    } else { \
        std::cout << "PASSED: " << message << std::endl; \
    }

// 测试事件结构
struct TestEvent {
    int id;
    float value;
    
    TestEvent() : id(0), value(0.0f) {}
    TestEvent(int i, float v) : id(i), value(v) {}
    
    bool operator==(const TestEvent& other) const {
        return id == other.id && value == other.value;
    }
};

// 测试组件结构
struct TestComponent {
    int data;
    TestComponent() : data(0) {}
    TestComponent(int d) : data(d) {}
};

// 测试 EventPool 基本功能
bool test_event_pool_basic() {
    std::cout << "\n=== Testing EventPool Basic Operations ===" << std::endl;
    
    EventPool<TestComponent> pool;
    
    // 测试获取对象
    auto obj1 = pool.acquire();
    TEST_ASSERT(obj1 != nullptr, "First object acquisition");
    
    auto obj2 = pool.acquire();
    TEST_ASSERT(obj2 != nullptr, "Second object acquisition");
    
    // 测试统计信息
    auto stats = pool.get_statistics();
    TEST_ASSERT_EQ(2, stats.created_count, "Created count should be 2");
    TEST_ASSERT_EQ(0, stats.reused_count, "Reused count should be 0");
    
    return true;
}

// 测试 EventPool 重用机制
bool test_event_pool_reuse() {
    std::cout << "\n=== Testing EventPool Reuse Mechanism ===" << std::endl;
    
    EventPool<TestComponent> pool;
    
    // 第一次获取
    {
        auto obj = pool.acquire();
        obj->data = 42;
    } // obj 在这里被释放回池中
    
    // 第二次获取应该重用对象
    auto obj2 = pool.acquire();
    auto stats = pool.get_statistics();
    
    TEST_ASSERT_EQ(1, stats.reused_count, "Should have 1 reused object");
    TEST_ASSERT(stats.reuse_ratio > 0.0f, "Reuse ratio should be greater than 0");
    
    return true;
}

// 测试 EventPool 预分配
bool test_event_pool_reserve() {
    std::cout << "\n=== Testing EventPool Reserve ===" << std::endl;
    
    EventPool<TestComponent> pool;
    
    pool.reserve(10);
    auto stats = pool.get_statistics();
    
    TEST_ASSERT_EQ(10, stats.available_count, "Should have 10 available objects");
    TEST_ASSERT_EQ(10, stats.created_count, "Should have created 10 objects");
    
    return true;
}

// 测试 EventPoolManager
bool test_event_pool_manager() {
    std::cout << "\n=== Testing EventPoolManager ===" << std::endl;
    
    auto& manager = EventPoolManager::get_instance();
    
    // 获取不同类型的池
    auto& pool1 = manager.get_pool<TestComponent>();
    auto& pool2 = manager.get_pool<TestEvent>();
    
    // 确保是不同的池实例
    TEST_ASSERT(&pool1 != reinterpret_cast<void*>(&pool2), "Different pools should have different addresses");
    
    // 测试预热功能（不会抛出异常）
    manager.warmup_pools();
    manager.cleanup_expired_pools();
    
    return true;
}

// 测试 LockFreeEventQueue 基本功能
bool test_lockfree_queue_basic() {
    std::cout << "\n=== Testing LockFreeEventQueue Basic Operations ===" << std::endl;
    
    LockFreeEventQueue<TestEvent> queue(16);
    
    TEST_ASSERT(queue.empty(), "Queue should be empty initially");
    TEST_ASSERT_EQ(0, queue.size(), "Queue size should be 0");
    
    // 入队测试
    TestEvent event1(1, 1.5f);
    TEST_ASSERT(queue.enqueue(event1), "Should be able to enqueue event");
    TEST_ASSERT(!queue.empty(), "Queue should not be empty after enqueue");
    TEST_ASSERT_EQ(1, queue.size(), "Queue size should be 1");
    
    // 出队测试
    TestEvent result;
    TEST_ASSERT(queue.dequeue(result), "Should be able to dequeue event");
    TEST_ASSERT(result == event1, "Dequeued event should match original");
    TEST_ASSERT(queue.empty(), "Queue should be empty after dequeue");
    
    return true;
}

// 测试 LockFreeEventQueue 容量限制
bool test_lockfree_queue_capacity() {
    std::cout << "\n=== Testing LockFreeEventQueue Capacity ===" << std::endl;
    
    LockFreeEventQueue<TestEvent> queue(3); // 小容量便于测试
    
    // 填满队列 (实际可用容量是 capacity-1)
    for (int i = 0; i < 2; ++i) {
        TestEvent event(i, i * 1.0f);
        TEST_ASSERT(queue.enqueue(event), "Should be able to enqueue within capacity");
    }
    
    // 再入队应该失败
    TestEvent overflow_event(999, 999.0f);
    TEST_ASSERT(!queue.enqueue(overflow_event), "Should fail to enqueue when queue is full");
    
    return true;
}

// 测试 LockFreeEventQueue 批量操作
bool test_lockfree_queue_batch() {
    std::cout << "\n=== Testing LockFreeEventQueue Batch Operations ===" << std::endl;
    
    LockFreeEventQueue<TestEvent> queue(16);
    
    // 入队多个事件
    for (int i = 0; i < 5; ++i) {
        TestEvent event(i, i * 2.0f);
        TEST_ASSERT(queue.enqueue(event), "Should be able to enqueue multiple events");
    }
    
    // 批量出队
    std::vector<TestEvent> events;
    size_t dequeued = queue.dequeue_batch(events, 3);
    
    TEST_ASSERT_EQ(3, dequeued, "Should dequeue exactly 3 events");
    TEST_ASSERT_EQ(3, events.size(), "Events vector should contain 3 events");
    
    // 验证顺序
    for (size_t i = 0; i < events.size(); ++i) {
        TEST_ASSERT_EQ(static_cast<int>(i), events[i].id, "Event ID should match order");
        TEST_ASSERT_EQ(i * 2.0f, events[i].value, "Event value should match expected");
    }
    
    return true;
}

// 测试 ConcurrentEventDispatcher
bool test_concurrent_dispatcher() {
    std::cout << "\n=== Testing ConcurrentEventDispatcher ===" << std::endl;
    
    ConcurrentEventDispatcher dispatcher;
    
    // 入队事件
    TestEvent event1(1, 1.0f);
    TestEvent event2(2, 2.0f);
    
    TEST_ASSERT(dispatcher.enqueue_concurrent(event1), "Should be able to enqueue first event");
    TEST_ASSERT(dispatcher.enqueue_concurrent(event2), "Should be able to enqueue second event");
    
    // 处理事件
    std::vector<TestEvent> processed_events;
    dispatcher.process_events<TestEvent>([&](const TestEvent& event) {
        processed_events.push_back(event);
    });
    
    TEST_ASSERT_EQ(2, processed_events.size(), "Should process exactly 2 events");
    TEST_ASSERT(processed_events[0] == event1, "First processed event should match");
    TEST_ASSERT(processed_events[1] == event2, "Second processed event should match");
    
    // 检查统计信息
    auto stats = dispatcher.get_statistics();
    TEST_ASSERT_EQ(2, stats.total_processed, "Total processed should be 2");
    
    return true;
}

// 多线程测试 - LockFreeEventQueue
bool test_lockfree_queue_multithreaded() {
    std::cout << "\n=== Testing LockFreeEventQueue Multi-threaded ===" << std::endl;
    
    LockFreeEventQueue<TestEvent> queue(1024);
    const int num_producers = 4;
    const int events_per_producer = 100;
    
    std::vector<std::thread> producers;
    std::atomic<int> total_produced(0);
    
    // 启动生产者线程
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < events_per_producer; ++i) {
                TestEvent event(p * events_per_producer + i, i * 1.0f);
                while (!queue.enqueue(event)) {
                    // 重试直到成功
                    std::this_thread::yield();
                }
                total_produced.fetch_add(1);
            }
        });
    }
    
    // 消费者线程
    std::vector<TestEvent> consumed_events;
    std::atomic<bool> should_stop(false);
    
    std::thread consumer([&]() {
        TestEvent event;
        while (!should_stop.load() || !queue.empty()) {
            if (queue.dequeue(event)) {
                consumed_events.push_back(event);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });
    
    // 等待生产者完成
    for (auto& producer : producers) {
        producer.join();
    }
    
    // 等待一段时间确保消费完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    should_stop.store(true);
    consumer.join();
    
    // 验证结果
    TEST_ASSERT_EQ(num_producers * events_per_producer, total_produced.load(), "All events should be produced");
    TEST_ASSERT_EQ(num_producers * events_per_producer, consumed_events.size(), "All events should be consumed");
    
    return true;
}

// 性能基准测试
bool test_performance_benchmark() {
    std::cout << "\n=== Performance Benchmark ===" << std::endl;
    
    const int iterations = 10000;
    
    // 测试 EventPool 性能
    {
        EventPool<TestComponent> pool;
        pool.reserve(100);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto obj = pool.acquire();
            obj->data = i;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "EventPool performance: " << iterations << " operations in " 
                  << duration.count() << " microseconds" << std::endl;
        
        auto stats = pool.get_statistics();
        std::cout << "Reuse ratio: " << stats.reuse_ratio << std::endl;
    }
    
    // 测试 LockFreeEventQueue 性能
    {
        LockFreeEventQueue<TestEvent> queue(16384);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 入队性能
        for (int i = 0; i < iterations; ++i) {
            TestEvent event(i, i * 1.0f);
            queue.enqueue(event);
        }
        
        // 出队性能
        TestEvent event;
        for (int i = 0; i < iterations; ++i) {
            queue.dequeue(event);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "LockFreeEventQueue performance: " << (iterations * 2) << " operations in " 
                  << duration.count() << " microseconds" << std::endl;
    }
    
    return true;
}

// 主测试函数
int main() {
    std::cout << "=== Event Pool and Concurrency Tests ===" << std::endl;
    
    bool all_passed = true;
    
    // 运行所有测试
    all_passed &= test_event_pool_basic();
    all_passed &= test_event_pool_reuse();
    all_passed &= test_event_pool_reserve();
    all_passed &= test_event_pool_manager();
    all_passed &= test_lockfree_queue_basic();
    all_passed &= test_lockfree_queue_capacity();
    all_passed &= test_lockfree_queue_batch();
    all_passed &= test_concurrent_dispatcher();
    all_passed &= test_lockfree_queue_multithreaded();
    all_passed &= test_performance_benchmark();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    if (all_passed) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        return 1;
    }
}
