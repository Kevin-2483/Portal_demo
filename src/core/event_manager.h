#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <type_traits>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include "event_pool_and_concurrency.h"

namespace portal_core {

// 事件组件标记 SFINAE 检测 (C++17 兼容)
template<typename T, typename = void>
struct is_event_component : std::false_type {};

template<typename T>
struct is_event_component<T, std::void_t<typename T::is_event_component>> : std::true_type {};

template<typename T>
static constexpr bool is_event_component_v = is_event_component<T>::value;

// 事件处理策略枚举
enum class EventHandlingStrategy {
    IMMEDIATE,      // 立即处理 (适用于音效、UI更新等)
    QUEUED,         // 队列处理 (适用于需要统一处理的事件)
    ENTITY_BASED,   // 实体事件 (适用于状态变化、持续效果)
    COMPONENT_BASED // 组件事件 (适用于临时标记、一次性效果)
};

// 事件优先级
enum class EventPriority {
    CRITICAL = 0,   // 关键事件 (错误、崩溃等)
    HIGH = 1,       // 高优先级 (玩家输入、碰撞等)
    NORMAL = 2,     // 普通事件 (状态更新、UI等)
    LOW = 3         // 低优先级 (日志、统计等)
};

// 事件元数据
struct EventMetadata {
    EventPriority priority = EventPriority::NORMAL;
    float delay = 0.0f;                    // 延迟处理时间(秒)
    bool auto_cleanup = true;              // 是否自动清理
    std::string category = "default";      // 事件分类
    uint32_t frame_lifetime = 1;           // 事件存活帧数
};

/**
 * 统一事件管理器
 * 
 * 提供多种事件处理策略的统一接口，根据事件特性自动选择最优处理方式：
 * - Dispatcher: 用于即时/队列事件，适合音效、粒子效果等
 * - Entity Events: 用于状态事件，适合持续效果、状态变化等
 * - Component Events: 用于标记事件，适合一次性触发器等
 */
class EventManager {
public:
    explicit EventManager(entt::registry& registry);
    ~EventManager() = default;

    // 禁止拷贝，允许移动
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    EventManager(EventManager&&) = default;
    EventManager& operator=(EventManager&&) = default;

    // === 模式一: Dispatcher 事件 (即时/队列) ===

    /**
     * 立即发布事件，同步调用所有监听者
     * 适用于: 音效播放、UI更新、粒子效果等需要立即响应的事件
     */
    template<typename TEvent>
    void publish_immediate(const TEvent& event, const EventMetadata& metadata = {});

    /**
     * 将事件加入队列，在帧末或指定时机统一处理
     * 适用于: 状态同步、批量更新、需要排序的事件等
     */
    template<typename TEvent>
    void enqueue(const TEvent& event, const EventMetadata& metadata = {});

    /**
     * 订阅 Dispatcher 事件
     * 返回 sink 用于连接/断开监听函数
     */
    template<typename TEvent>
    auto subscribe() -> decltype(auto);

    // === 模式二: 实体事件 (数据驱动状态) ===

    /**
     * 创建事件实体，将事件作为组件附加
     * 适用于: 复杂状态、可查询的事件、需要持久化的状态等
     */
    template<typename TEventComponent, 
             typename = std::enable_if_t<is_event_component_v<TEventComponent>>>
    entt::entity create_entity_event(TEventComponent&& event_component, 
                                   const EventMetadata& metadata = {});

    /**
     * 向已有实体添加事件组件
     * 适用于: 实体状态变化、属性修改、状态标记等
     */
    template<typename TEventComponent,
             typename = std::enable_if_t<is_event_component_v<TEventComponent>>>
    void add_component_event(entt::entity target_entity, 
                           TEventComponent&& event_component,
                           const EventMetadata& metadata = {});

    // === 模式三: 组件标记事件 (临时标记) ===

    /**
     * 添加临时标记组件，自动在指定帧数后清理
     * 适用于: 一次性触发器、临时状态、帧级别的标记等
     */
    template<typename TEventComponent,
             typename = std::enable_if_t<is_event_component_v<TEventComponent>>>
    void add_temporary_marker(entt::entity target_entity,
                            TEventComponent&& event_component,
                            uint32_t lifetime_frames = 1);

    // === 高级功能 ===

    /**
     * 延迟执行事件
     */
    template<typename TEvent>
    void schedule_event(const TEvent& event, float delay_seconds, 
                       EventHandlingStrategy strategy = EventHandlingStrategy::QUEUED);

    /**
     * 批量发布事件
     */
    template<typename TEvent>
    void publish_batch(const std::vector<TEvent>& events, 
                      EventHandlingStrategy strategy = EventHandlingStrategy::QUEUED);

    /**
     * 取消指定类型的所有队列事件
     */
    template<typename TEvent>
    void cancel_queued_events();

    // === 对象池管理 (新增) ===

    /**
     * 启用/禁用对象池 - 减少内存分配开销
     */
    void set_object_pooling_enabled(bool enabled) { use_object_pooling_ = enabled; }
    bool is_object_pooling_enabled() const { return use_object_pooling_; }

    /**
     * 预热对象池 - 预分配常用事件对象
     */
    void warmup_object_pools();

    /**
     * 获取对象池统计信息
     */
    struct ObjectPoolStatistics {
        size_t total_pools_active = 0;
        size_t total_objects_created = 0;
        size_t total_objects_reused = 0;
        float average_reuse_ratio = 0.0f;
        std::unordered_map<std::string, size_t> pool_sizes;
    };
    
    ObjectPoolStatistics get_pool_statistics() const;

    // === 并发支持 (新增) ===

    /**
     * 启用/禁用多线程模式
     */
    void set_concurrent_mode(bool enabled);
    bool is_concurrent_mode() const { return concurrent_mode_enabled_; }

    /**
     * 线程安全的事件入队 (仅在并发模式下可用)
     */
    template<typename TEvent>
    bool enqueue_concurrent(const TEvent& event);

    /**
     * 设置工作线程数量 (默认为硬件线程数)
     */
    void set_worker_thread_count(size_t count);
    size_t get_worker_thread_count() const { return worker_thread_count_; }

    /**
     * 获取并发统计信息
     */
    struct ConcurrencyStatistics {
        bool concurrent_mode_active = false;
        size_t worker_threads = 0;
        size_t concurrent_events_processed = 0;
        size_t concurrent_events_dropped = 0;
        float average_queue_utilization = 0.0f;
        std::unordered_map<std::string, size_t> thread_workload;
    };
    
    ConcurrencyStatistics get_concurrency_statistics() const;

    // === 系统管理 ===

    /**
     * 处理所有队列中的事件 (在游戏循环的特定阶段调用)
     */
    void process_queued_events(float delta_time);

    /**
     * 清理过期的事件实体和组件
     */
    void cleanup_expired_events();

    /**
     * 获取事件统计信息
     */
    struct EventStatistics {
        uint32_t immediate_events_count = 0;
        uint32_t queued_events_count = 0;
        uint32_t entity_events_count = 0;
        uint32_t temporary_markers_count = 0;
        float last_process_time_ms = 0.0f;
        std::unordered_map<std::string, uint32_t> events_by_category;
    };

    const EventStatistics& get_statistics() const { return statistics_; }

    /**
     * 启用/禁用事件调试模式
     */
    void set_debug_mode(bool enabled) { debug_mode_ = enabled; }
    bool is_debug_mode() const { return debug_mode_; }

    /**
     * 获取注册表的访问权限 (用于事件处理器需要访问注册表的情况)
     */
    entt::registry& get_registry() { return registry_; }
    const entt::registry& get_registry() const { return registry_; }

    // === 配置管理 (新增) ===
    
    /**
     * 事件管理器配置结构
     */
    struct Configuration {
        bool object_pooling_enabled = true;
        bool concurrent_mode_enabled = false;
        bool debug_mode_enabled = false;
        size_t concurrent_queue_size = 10000;
        size_t pool_initial_size = 100;
        size_t pool_max_size = 1000;
        float pool_cleanup_interval = 30.0f; // seconds
        int max_events_per_frame = 1000;
        int max_temporary_marker_frames = 300; // 5 seconds at 60fps
        float performance_profiling_enabled = false;
    };
    
    void apply_configuration(const Configuration& config);
    Configuration get_configuration() const;

    // === 高级监控接口 (新增) ===
    
    void reset_statistics();
    void export_statistics_to_console() const;
    void export_pool_diagnostics() const;
    
    /**
     * 内存管理
     */
    void cleanup_expired_pools();
    void force_garbage_collection();
    size_t get_total_memory_usage() const;
    
    /**
     * 性能分析接口
     */
    struct PerformanceProfile {
        float avg_immediate_event_time_ms = 0.0f;
        float avg_queued_event_time_ms = 0.0f;
        float avg_concurrent_event_time_ms = 0.0f;
        float frame_processing_time_ms = 0.0f;
        size_t peak_memory_usage_bytes = 0;
        size_t current_memory_usage_bytes = 0;
        float profiling_overhead_ms = 0.0f;
    };
    
    PerformanceProfile get_performance_profile() const;
    void start_performance_profiling();
    void stop_performance_profiling();
    bool is_performance_profiling_enabled() const { return performance_profiling_enabled_; }

private:
    entt::registry& registry_;
    entt::dispatcher dispatcher_;
    
    // 高级功能开关
    bool use_object_pooling_ = true;           // 默认启用对象池
    bool concurrent_mode_enabled_ = false;    // 默认禁用并发模式
    size_t worker_thread_count_ = 0;          // 工作线程数 (0 = 自动检测)
    
    // 对象池管理器
    EventPoolManager& pool_manager_;
    
    // 并发事件调度器
    std::unique_ptr<ConcurrentEventDispatcher> concurrent_dispatcher_;
    
    // 延迟事件队列
    struct DelayedEvent {
        std::function<void()> executor;
        float remaining_time;
        EventPriority priority;
        std::string category;
    };
    std::vector<DelayedEvent> delayed_events_;

    // 临时标记管理
    struct TemporaryMarker {
        entt::entity entity;
        std::function<void()> cleanup_func;
        uint32_t remaining_frames;
    };
    std::vector<TemporaryMarker> temporary_markers_;

    // 统计信息
    mutable EventStatistics statistics_;
    mutable ObjectPoolStatistics pool_statistics_;
    mutable ConcurrencyStatistics concurrency_statistics_;
    bool debug_mode_ = false;
    uint32_t current_frame_ = 0;

    // === 配置管理 (新增) ===
    Configuration current_config_;
    
    // === 性能分析 (新增) ===
    bool performance_profiling_enabled_ = false;
    mutable PerformanceProfile performance_profile_;
    mutable std::chrono::high_resolution_clock::time_point last_profiling_time_;
    mutable size_t peak_memory_usage_ = 0;
    
    // === 内存使用跟踪 (新增) ===
    mutable size_t total_allocated_memory_ = 0;
    mutable std::unordered_map<std::string, size_t> memory_usage_by_type_;
    
    // === 清理管理 (新增) ===
    float last_cleanup_time_ = 0.0f;
    std::vector<std::function<void()>> cleanup_callbacks_;

    // 内部辅助方法
    void update_delayed_events(float delta_time);
    void update_temporary_markers();
    void log_event_if_debug(const std::string& event_type, const std::string& action);
    
    // === 新增内部方法 ===
    void update_performance_metrics() const;
    void track_memory_allocation(const std::string& type, size_t bytes) const;
    void track_memory_deallocation(const std::string& type, size_t bytes) const;
    void schedule_cleanup_if_needed(float current_time);
};

// === 模板实现 ===

template<typename TEvent>
void EventManager::publish_immediate(const TEvent& event, const EventMetadata& metadata) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEvent).name(), "publish_immediate");
    }
    
    dispatcher_.trigger(event);
    ++statistics_.immediate_events_count;
    ++statistics_.events_by_category[metadata.category];
}

template<typename TEvent>
void EventManager::enqueue(const TEvent& event, const EventMetadata& metadata) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEvent).name(), "enqueue");
    }

    if (metadata.delay > 0.0f) {
        schedule_event(event, metadata.delay, EventHandlingStrategy::QUEUED);
    } else {
        dispatcher_.enqueue(event);
        ++statistics_.queued_events_count;
        ++statistics_.events_by_category[metadata.category];
    }
}

template<typename TEvent>
auto EventManager::subscribe() -> decltype(auto) {
    return dispatcher_.sink<TEvent>();
}

template<typename TEventComponent, typename>
entt::entity EventManager::create_entity_event(TEventComponent&& event_component, 
                                              const EventMetadata& metadata) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEventComponent).name(), "create_entity_event");
    }

    const auto event_entity = registry_.create();
    
    if (use_object_pooling_) {
        // 使用对象池创建组件
        auto& pool = pool_manager_.get_pool<TEventComponent>();
        auto pooled_component = pool.acquire(std::forward<TEventComponent>(event_component));
        
        // 将池化对象的内容复制到实体组件
        registry_.emplace<TEventComponent>(event_entity, *pooled_component);
        
        // pooled_component 会在作用域结束时自动归还到池中
    } else {
        // 直接创建组件
        registry_.emplace<TEventComponent>(event_entity, std::forward<TEventComponent>(event_component));
    }
    
    // 添加元数据组件
    struct EventMetadataComponent {
        using is_event_component = void;
        EventMetadata metadata;
        uint32_t creation_frame;
    };
    
    registry_.emplace<EventMetadataComponent>(event_entity, 
        EventMetadataComponent{metadata, current_frame_});

    ++statistics_.entity_events_count;
    ++statistics_.events_by_category[metadata.category];
    
    return event_entity;
}

template<typename TEventComponent, typename>
void EventManager::add_component_event(entt::entity target_entity, 
                                      TEventComponent&& event_component,
                                      const EventMetadata& metadata) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEventComponent).name(), "add_component_event");
    }

    if (use_object_pooling_) {
        // 使用对象池创建组件
        auto& pool = pool_manager_.get_pool<TEventComponent>();
        auto pooled_component = pool.acquire(std::forward<TEventComponent>(event_component));
        
        registry_.emplace_or_replace<TEventComponent>(target_entity, *pooled_component);
    } else {
        registry_.emplace_or_replace<TEventComponent>(target_entity, 
            std::forward<TEventComponent>(event_component));
    }
    
    ++statistics_.entity_events_count;
    ++statistics_.events_by_category[metadata.category];
}

template<typename TEventComponent, typename>
void EventManager::add_temporary_marker(entt::entity target_entity,
                                       TEventComponent&& event_component,
                                       uint32_t lifetime_frames) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEventComponent).name(), "add_temporary_marker");
    }

    if (use_object_pooling_) {
        auto& pool = pool_manager_.get_pool<TEventComponent>();
        auto pooled_component = pool.acquire(std::forward<TEventComponent>(event_component));
        
        registry_.emplace_or_replace<TEventComponent>(target_entity, *pooled_component);
    } else {
        registry_.emplace_or_replace<TEventComponent>(target_entity, 
            std::forward<TEventComponent>(event_component));
    }

    // 注册清理函数
    temporary_markers_.push_back({
        target_entity,
        [this, target_entity]() {
            registry_.remove<TEventComponent>(target_entity);
        },
        lifetime_frames
    });

    ++statistics_.temporary_markers_count;
}

template<typename TEvent>
bool EventManager::enqueue_concurrent(const TEvent& event) {
    if (!concurrent_mode_enabled_) {
        std::cerr << "EventManager: Concurrent mode not enabled!" << std::endl;
        return false;
    }
    
    if (!concurrent_dispatcher_) {
        std::cerr << "EventManager: Concurrent dispatcher not initialized!" << std::endl;
        return false;
    }
    
    bool success = concurrent_dispatcher_->enqueue_concurrent(event);
    
    if (success) {
        ++concurrency_statistics_.concurrent_events_processed;
    } else {
        ++concurrency_statistics_.concurrent_events_dropped;
    }
    
    return success;
}

template<typename TEvent>
void EventManager::schedule_event(const TEvent& event, float delay_seconds, 
                                 EventHandlingStrategy strategy) {
    if (debug_mode_) {
        log_event_if_debug(typeid(TEvent).name(), "schedule_event");
    }

    DelayedEvent delayed;
    delayed.remaining_time = delay_seconds;
    delayed.priority = EventPriority::NORMAL;
    delayed.category = "scheduled";

    switch (strategy) {
        case EventHandlingStrategy::IMMEDIATE:
            delayed.executor = [this, event]() { publish_immediate(event); };
            break;
        case EventHandlingStrategy::QUEUED:
            delayed.executor = [this, event]() { enqueue(event); };
            break;
        default:
            delayed.executor = [this, event]() { enqueue(event); };
            break;
    }

    delayed_events_.push_back(std::move(delayed));
}

template<typename TEvent>
void EventManager::publish_batch(const std::vector<TEvent>& events, 
                                EventHandlingStrategy strategy) {
    for (const auto& event : events) {
        switch (strategy) {
            case EventHandlingStrategy::IMMEDIATE:
                publish_immediate(event);
                break;
            case EventHandlingStrategy::QUEUED:
                enqueue(event);
                break;
            default:
                enqueue(event);
                break;
        }
    }
}

template<typename TEvent>
void EventManager::cancel_queued_events() {
    // EnTT dispatcher 没有直接的取消接口，这里可以实现自定义逻辑
    // 或者通过标记方式在处理时跳过
    if (debug_mode_) {
        log_event_if_debug(typeid(TEvent).name(), "cancel_queued_events");
    }
}

} // namespace portal_core
