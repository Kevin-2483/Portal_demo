#include "event_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include "debug/portal_debug_logging.h"

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
        PORTAL_DEBUG_LOG("EventManager: Initialized with " << worker_thread_count_ 
                  << " worker threads detected");
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
        PORTAL_DEBUG_LOG("EventManager: Processed queued events in " 
                  << statistics_.last_process_time_ms << "ms");
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
        PORTAL_DEBUG_LOG("EventManager: Cleaned up " << to_destroy.size() 
                  << " expired event entities");
    }
}

void EventManager::update_delayed_events(float delta_time) {
    auto it = delayed_events_.begin();
    while (it != delayed_events_.end()) {
        it->remaining_time -= delta_time;
        
        if (it->remaining_time <= 0.0f) {
            // 直接执行回调，不使用异常处理
            if (it->executor) {
                it->executor();
            }
            if (debug_mode_) {
                PORTAL_DEBUG_LOG("EventManager: Executed delayed event in category: " 
                          << it->category);
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
            // 直接执行清理回调，不使用异常处理
            if (it->cleanup_func) {
                it->cleanup_func();
            }
            if (debug_mode_) {
                PORTAL_DEBUG_LOG("EventManager: Cleaned up temporary marker for entity " 
                          << static_cast<uint32_t>(it->entity));
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
        PORTAL_DEBUG_LOG("EventManager: " << action << " - " << event_type 
                  << " (Frame: " << current_frame_ << ")");
    }
}

void EventManager::warmup_object_pools() {
    if (!use_object_pooling_) return;
    
    // 预热常用事件类型的对象池
    pool_manager_.warmup_pools();
    
    if (debug_mode_) {
        PORTAL_DEBUG_LOG("EventManager: Object pools warmed up");
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
            PORTAL_DEBUG_LOG("EventManager: Concurrent mode enabled with " 
                      << worker_thread_count_ << " worker threads");
        }
    } else {
        // 清理并发调度器
        concurrent_dispatcher_.reset();
        concurrency_statistics_.concurrent_mode_active = false;
        concurrency_statistics_.worker_threads = 0;
        
        if (debug_mode_) {
            PORTAL_DEBUG_LOG("EventManager: Concurrent mode disabled");
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
        PORTAL_DEBUG_LOG("EventManager: Worker thread count set to " << count);
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
        PORTAL_DEBUG_LOG("EventManager: Configuration applied successfully");
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
        PORTAL_DEBUG_LOG("EventManager: Statistics reset");
    }
}

void EventManager::export_statistics_to_console() const {
    PORTAL_DEBUG_LOG("\n=== EventManager Statistics ===");
    PORTAL_DEBUG_LOG("Events:");
    PORTAL_DEBUG_LOG("  Immediate: " << statistics_.immediate_events_count);
    PORTAL_DEBUG_LOG("  Queued: " << statistics_.queued_events_count);
    PORTAL_DEBUG_LOG("  Entity Events: " << statistics_.entity_events_count);
    PORTAL_DEBUG_LOG("  Temporary Markers: " << statistics_.temporary_markers_count);
    PORTAL_DEBUG_LOG("  Last Process Time: " << statistics_.last_process_time_ms << "ms");
    
    PORTAL_DEBUG_LOG("\nPools:");
    PORTAL_DEBUG_LOG("  Active Pools: " << pool_statistics_.total_pools_active);
    PORTAL_DEBUG_LOG("  Objects Created: " << pool_statistics_.total_objects_created);
    PORTAL_DEBUG_LOG("  Objects Reused: " << pool_statistics_.total_objects_reused);
    PORTAL_DEBUG_LOG("  Reuse Ratio: " << pool_statistics_.average_reuse_ratio * 100 << "%");
    
    PORTAL_DEBUG_LOG("\nConcurrency:");
    PORTAL_DEBUG_LOG("  Mode Active: " << (concurrency_statistics_.concurrent_mode_active ? "Yes" : "No"));
    PORTAL_DEBUG_LOG("  Worker Threads: " << concurrency_statistics_.worker_threads);
    PORTAL_DEBUG_LOG("  Concurrent Events Processed: " << concurrency_statistics_.concurrent_events_processed);
    PORTAL_DEBUG_LOG("  Concurrent Events Dropped: " << concurrency_statistics_.concurrent_events_dropped);
    PORTAL_DEBUG_LOG("  Queue Utilization: " << concurrency_statistics_.average_queue_utilization * 100 << "%");
    
    if (performance_profiling_enabled_) {
        PORTAL_DEBUG_LOG("\nPerformance:");
        PORTAL_DEBUG_LOG("  Avg Immediate Time: " << performance_profile_.avg_immediate_event_time_ms << "ms");
        PORTAL_DEBUG_LOG("  Avg Queued Time: " << performance_profile_.avg_queued_event_time_ms << "ms");
        PORTAL_DEBUG_LOG("  Avg Concurrent Time: " << performance_profile_.avg_concurrent_event_time_ms << "ms");
        PORTAL_DEBUG_LOG("  Frame Processing Time: " << performance_profile_.frame_processing_time_ms << "ms");
        PORTAL_DEBUG_LOG("  Current Memory: " << performance_profile_.current_memory_usage_bytes << " bytes");
        PORTAL_DEBUG_LOG("  Peak Memory: " << performance_profile_.peak_memory_usage_bytes << " bytes");
    }
    
    PORTAL_DEBUG_LOG("================================\n");
}

void EventManager::export_pool_diagnostics() const {
    PORTAL_DEBUG_LOG("\n=== Pool Diagnostics ===");
    
    for (const auto& [type_name, size] : pool_statistics_.pool_sizes) {
        PORTAL_DEBUG_LOG("Pool [" << type_name << "]: " << size << " objects");
    }
    
    for (const auto& [type_name, memory] : memory_usage_by_type_) {
        PORTAL_DEBUG_LOG("Memory [" << type_name << "]: " << memory << " bytes");
    }
    
    PORTAL_DEBUG_LOG("========================\n");
}

// === 内存管理实现 (新增) ===

void EventManager::cleanup_expired_pools() {
    if (use_object_pooling_) {
        pool_manager_.cleanup_expired_pools();
        
        if (debug_mode_) {
            PORTAL_DEBUG_LOG("EventManager: Expired pools cleaned up");
        }
    }
}

void EventManager::force_garbage_collection() {
    // 强制清理所有过期对象
    cleanup_expired_events();
    cleanup_expired_pools();
    
    // 执行注册的清理回调
    for (auto& callback : cleanup_callbacks_) {
        // 直接执行清理回调，不使用异常处理
        if (callback) {
            callback();
        }
    }
    
    if (debug_mode_) {
        PORTAL_DEBUG_LOG("EventManager: Forced garbage collection completed");
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
        PORTAL_DEBUG_LOG("EventManager: Performance profiling started");
    }
}

void EventManager::stop_performance_profiling() {
    performance_profiling_enabled_ = false;
    
    if (debug_mode_) {
        PORTAL_DEBUG_LOG("EventManager: Performance profiling stopped");
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
