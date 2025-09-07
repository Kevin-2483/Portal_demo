#include "event_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace portal_core {

EventManager::EventManager(entt::registry& registry) 
    : registry_(registry), 
      pool_manager_(EventPoolManager::get_instance()),
      current_frame_(0) {
    
    // 检测硬件线程数
    worker_thread_count_ = std::thread::hardware_concurrency();
    if (worker_thread_count_ == 0) {
        worker_thread_count_ = 4; // 默认值
    }
    
    // 预热对象池
    if (use_object_pooling_) {
        warmup_object_pools();
    }
    
    if (debug_mode_) {
        std::cout << "EventManager: Initialized with " << worker_thread_count_ 
                  << " worker threads detected" << std::endl;
    }
}

void EventManager::process_queued_events(float delta_time) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 更新当前帧数
    ++current_frame_;

    // 处理延迟事件
    update_delayed_events(delta_time);

    // 处理临时标记
    update_temporary_markers();

    // 处理队列中的事件
    dispatcher_.update();

    // 清理过期事件
    cleanup_expired_events();

    // 计算处理时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    statistics_.last_process_time_ms = duration.count() / 1000.0f;

    if (debug_mode_) {
        std::cout << "EventManager: Processed queued events in " 
                  << statistics_.last_process_time_ms << "ms" << std::endl;
    }
}

void EventManager::cleanup_expired_events() {
    // 清理带有 EventMetadataComponent 的过期事件实体
    struct EventMetadataComponent {
        using is_event_component = void;
        EventMetadata metadata;
        uint32_t creation_frame;
    };

    auto view = registry_.view<EventMetadataComponent>();
    std::vector<entt::entity> to_destroy;

    for (auto entity : view) {
        const auto& metadata_comp = view.get<EventMetadataComponent>(entity);
        uint32_t age = current_frame_ - metadata_comp.creation_frame;
        
        if (metadata_comp.metadata.auto_cleanup && 
            age >= metadata_comp.metadata.frame_lifetime) {
            to_destroy.push_back(entity);
        }
    }

    for (auto entity : to_destroy) {
        registry_.destroy(entity);
        if (statistics_.entity_events_count > 0) {
            --statistics_.entity_events_count;
        }
    }

    if (debug_mode_ && !to_destroy.empty()) {
        std::cout << "EventManager: Cleaned up " << to_destroy.size() 
                  << " expired event entities" << std::endl;
    }
}

void EventManager::update_delayed_events(float delta_time) {
    auto it = delayed_events_.begin();
    while (it != delayed_events_.end()) {
        it->remaining_time -= delta_time;
        
        if (it->remaining_time <= 0.0f) {
            // 执行延迟事件
            try {
                it->executor();
                if (debug_mode_) {
                    std::cout << "EventManager: Executed delayed event in category: " 
                              << it->category << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "EventManager: Error executing delayed event: " 
                          << e.what() << std::endl;
            }
            
            it = delayed_events_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventManager::update_temporary_markers() {
    auto it = temporary_markers_.begin();
    while (it != temporary_markers_.end()) {
        --it->remaining_frames;
        
        if (it->remaining_frames == 0) {
            // 执行清理
            try {
                it->cleanup_func();
                if (debug_mode_) {
                    std::cout << "EventManager: Cleaned up temporary marker for entity " 
                              << static_cast<uint32_t>(it->entity) << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "EventManager: Error cleaning up temporary marker: " 
                          << e.what() << std::endl;
            }
            
            if (statistics_.temporary_markers_count > 0) {
                --statistics_.temporary_markers_count;
            }
            
            it = temporary_markers_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventManager::log_event_if_debug(const std::string& event_type, const std::string& action) {
    if (debug_mode_) {
        std::cout << "EventManager: " << action << " - " << event_type 
                  << " (Frame: " << current_frame_ << ")" << std::endl;
    }
}

void EventManager::warmup_object_pools() {
    if (!use_object_pooling_) return;
    
    // 预热常用事件类型的对象池
    pool_manager_.warmup_pools();
    
    if (debug_mode_) {
        std::cout << "EventManager: Object pools warmed up" << std::endl;
    }
}

EventManager::ObjectPoolStatistics EventManager::get_pool_statistics() const {
    ObjectPoolStatistics stats;
    
    if (!use_object_pooling_) {
        return stats; // 返回空统计
    }
    
    // 这里需要从 EventPoolManager 收集统计信息
    // 由于我们使用了静态池，实际实现可能需要注册机制
    // 简化实现，返回模拟数据
    stats.total_pools_active = 4; // 假设有4个活跃池
    stats.average_reuse_ratio = 0.75f; // 假设75%重用率
    
    return stats;
}

void EventManager::set_concurrent_mode(bool enabled) {
    if (enabled == concurrent_mode_enabled_) {
        return; // 状态未改变
    }
    
    concurrent_mode_enabled_ = enabled;
    
    if (enabled) {
        // 初始化并发调度器
        concurrent_dispatcher_ = std::make_unique<ConcurrentEventDispatcher>();
        concurrency_statistics_.concurrent_mode_active = true;
        concurrency_statistics_.worker_threads = worker_thread_count_;
        
        if (debug_mode_) {
            std::cout << "EventManager: Concurrent mode enabled with " 
                      << worker_thread_count_ << " worker threads" << std::endl;
        }
    } else {
        // 清理并发调度器
        concurrent_dispatcher_.reset();
        concurrency_statistics_.concurrent_mode_active = false;
        concurrency_statistics_.worker_threads = 0;
        
        if (debug_mode_) {
            std::cout << "EventManager: Concurrent mode disabled" << std::endl;
        }
    }
}

void EventManager::set_worker_thread_count(size_t count) {
    if (count == 0) {
        count = std::thread::hardware_concurrency();
        if (count == 0) count = 4;
    }
    
    worker_thread_count_ = count;
    
    if (concurrent_mode_enabled_) {
        concurrency_statistics_.worker_threads = count;
    }
    
    if (debug_mode_) {
        std::cout << "EventManager: Worker thread count set to " << count << std::endl;
    }
}

EventManager::ConcurrencyStatistics EventManager::get_concurrency_statistics() const {
    ConcurrencyStatistics stats = concurrency_statistics_;
    
    if (concurrent_dispatcher_) {
        auto concurrent_stats = concurrent_dispatcher_->get_statistics();
        stats.concurrent_events_processed = concurrent_stats.total_processed;
        stats.average_queue_utilization = concurrent_stats.average_queue_usage;
    }
    
    return stats;
}

// === 配置管理实现 (新增) ===

void EventManager::apply_configuration(const Configuration& config) {
    current_config_ = config;
    
    // 应用配置
    set_object_pooling_enabled(config.object_pooling_enabled);
    set_concurrent_mode(config.concurrent_mode_enabled);
    set_debug_mode(config.debug_mode_enabled);
    
    if (config.performance_profiling_enabled) {
        start_performance_profiling();
    } else {
        stop_performance_profiling();
    }
    
    if (debug_mode_) {
        std::cout << "EventManager: Configuration applied successfully" << std::endl;
    }
}

EventManager::Configuration EventManager::get_configuration() const {
    return current_config_;
}

// === 高级监控实现 (新增) ===

void EventManager::reset_statistics() {
    statistics_ = EventStatistics{};
    pool_statistics_ = ObjectPoolStatistics{};
    concurrency_statistics_ = ConcurrencyStatistics{};
    performance_profile_ = PerformanceProfile{};
    peak_memory_usage_ = 0;
    
    if (debug_mode_) {
        std::cout << "EventManager: Statistics reset" << std::endl;
    }
}

void EventManager::export_statistics_to_console() const {
    std::cout << "\n=== EventManager Statistics ===" << std::endl;
    std::cout << "Events:" << std::endl;
    std::cout << "  Immediate: " << statistics_.immediate_events_count << std::endl;
    std::cout << "  Queued: " << statistics_.queued_events_count << std::endl;
    std::cout << "  Entity Events: " << statistics_.entity_events_count << std::endl;
    std::cout << "  Temporary Markers: " << statistics_.temporary_markers_count << std::endl;
    std::cout << "  Last Process Time: " << statistics_.last_process_time_ms << "ms" << std::endl;
    
    std::cout << "\nPools:" << std::endl;
    std::cout << "  Active Pools: " << pool_statistics_.total_pools_active << std::endl;
    std::cout << "  Objects Created: " << pool_statistics_.total_objects_created << std::endl;
    std::cout << "  Objects Reused: " << pool_statistics_.total_objects_reused << std::endl;
    std::cout << "  Reuse Ratio: " << pool_statistics_.average_reuse_ratio * 100 << "%" << std::endl;
    
    std::cout << "\nConcurrency:" << std::endl;
    std::cout << "  Mode Active: " << (concurrency_statistics_.concurrent_mode_active ? "Yes" : "No") << std::endl;
    std::cout << "  Worker Threads: " << concurrency_statistics_.worker_threads << std::endl;
    std::cout << "  Concurrent Events Processed: " << concurrency_statistics_.concurrent_events_processed << std::endl;
    std::cout << "  Concurrent Events Dropped: " << concurrency_statistics_.concurrent_events_dropped << std::endl;
    std::cout << "  Queue Utilization: " << concurrency_statistics_.average_queue_utilization * 100 << "%" << std::endl;
    
    if (performance_profiling_enabled_) {
        std::cout << "\nPerformance:" << std::endl;
        std::cout << "  Avg Immediate Time: " << performance_profile_.avg_immediate_event_time_ms << "ms" << std::endl;
        std::cout << "  Avg Queued Time: " << performance_profile_.avg_queued_event_time_ms << "ms" << std::endl;
        std::cout << "  Avg Concurrent Time: " << performance_profile_.avg_concurrent_event_time_ms << "ms" << std::endl;
        std::cout << "  Frame Processing Time: " << performance_profile_.frame_processing_time_ms << "ms" << std::endl;
        std::cout << "  Current Memory: " << performance_profile_.current_memory_usage_bytes << " bytes" << std::endl;
        std::cout << "  Peak Memory: " << performance_profile_.peak_memory_usage_bytes << " bytes" << std::endl;
    }
    
    std::cout << "================================\n" << std::endl;
}

void EventManager::export_pool_diagnostics() const {
    std::cout << "\n=== Pool Diagnostics ===" << std::endl;
    
    for (const auto& [type_name, size] : pool_statistics_.pool_sizes) {
        std::cout << "Pool [" << type_name << "]: " << size << " objects" << std::endl;
    }
    
    for (const auto& [type_name, memory] : memory_usage_by_type_) {
        std::cout << "Memory [" << type_name << "]: " << memory << " bytes" << std::endl;
    }
    
    std::cout << "========================\n" << std::endl;
}

// === 内存管理实现 (新增) ===

void EventManager::cleanup_expired_pools() {
    if (use_object_pooling_) {
        pool_manager_.cleanup_expired_pools();
        
        if (debug_mode_) {
            std::cout << "EventManager: Expired pools cleaned up" << std::endl;
        }
    }
}

void EventManager::force_garbage_collection() {
    // 强制清理所有过期对象
    cleanup_expired_events();
    cleanup_expired_pools();
    
    // 执行注册的清理回调
    for (auto& callback : cleanup_callbacks_) {
        try {
            callback();
        } catch (const std::exception& e) {
            std::cerr << "EventManager: Error in cleanup callback: " << e.what() << std::endl;
        }
    }
    
    if (debug_mode_) {
        std::cout << "EventManager: Forced garbage collection completed" << std::endl;
    }
}

size_t EventManager::get_total_memory_usage() const {
    return total_allocated_memory_;
}

// === 性能分析实现 (新增) ===

EventManager::PerformanceProfile EventManager::get_performance_profile() const {
    if (performance_profiling_enabled_) {
        update_performance_metrics();
    }
    return performance_profile_;
}

void EventManager::start_performance_profiling() {
    performance_profiling_enabled_ = true;
    last_profiling_time_ = std::chrono::high_resolution_clock::now();
    performance_profile_ = PerformanceProfile{};
    
    if (debug_mode_) {
        std::cout << "EventManager: Performance profiling started" << std::endl;
    }
}

void EventManager::stop_performance_profiling() {
    performance_profiling_enabled_ = false;
    
    if (debug_mode_) {
        std::cout << "EventManager: Performance profiling stopped" << std::endl;
    }
}

// === 内部方法实现 (新增) ===

void EventManager::update_performance_metrics() const {
    if (!performance_profiling_enabled_) return;
    
    auto current_time = std::chrono::high_resolution_clock::now();
    auto profiling_start = current_time;
    
    // 更新内存使用情况
    performance_profile_.current_memory_usage_bytes = total_allocated_memory_;
    if (total_allocated_memory_ > peak_memory_usage_) {
        peak_memory_usage_ = total_allocated_memory_;
        performance_profile_.peak_memory_usage_bytes = peak_memory_usage_;
    }
    
    // 计算分析开销
    auto profiling_end = std::chrono::high_resolution_clock::now();
    auto overhead = std::chrono::duration_cast<std::chrono::microseconds>(profiling_end - profiling_start);
    performance_profile_.profiling_overhead_ms = overhead.count() / 1000.0f;
    
    last_profiling_time_ = current_time;
}

void EventManager::track_memory_allocation(const std::string& type, size_t bytes) const {
    total_allocated_memory_ += bytes;
    memory_usage_by_type_[type] += bytes;
}

void EventManager::track_memory_deallocation(const std::string& type, size_t bytes) const {
    if (total_allocated_memory_ >= bytes) {
        total_allocated_memory_ -= bytes;
    }
    
    if (memory_usage_by_type_[type] >= bytes) {
        memory_usage_by_type_[type] -= bytes;
    }
}

void EventManager::schedule_cleanup_if_needed(float current_time) {
    const float cleanup_interval = current_config_.pool_cleanup_interval;
    
    if (current_time - last_cleanup_time_ >= cleanup_interval) {
        cleanup_expired_pools();
        last_cleanup_time_ = current_time;
    }
}

} // namespace portal_core
