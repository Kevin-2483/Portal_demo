#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <type_traits>
#include <functional>
#include <unordered_set>
#include <algorithm>

namespace portal_core {

/**
 * 模板化对象池 - 用于高效管理事件对象的内存分配
 * 
 * 特别适用于频繁创建/销毁的事件组件，显著减少内存碎片和分配开销
 * 
 * 设计原则：
 * - all_objects_ 拥有所有对象的唯一所有权
 * - available_ 只存储裸指针作为可借用对象的索引
 * - 使用 RAII 自动管理对象归还
 */
template<typename T>
class EventPool {
public:
    static_assert(std::is_default_constructible<T>::value, "T must be default constructible");
    
    // 定义智能指针类型
    using unique_obj_ptr = std::unique_ptr<T, std::function<void(T*)>>;
    
    EventPool(size_t initial_capacity = 0, size_t max_capacity = 1024)
        : max_capacity_(max_capacity) {
        if (initial_capacity > 0) {
            reserve(initial_capacity);
        }
    }
    
    ~EventPool() {
        clear();
    }
    
    // 禁止拷贝，允许移动
    EventPool(const EventPool&) = delete;
    EventPool& operator=(const EventPool&) = delete;
    EventPool(EventPool&&) = default;
    EventPool& operator=(EventPool&&) = default;
    
    /**
     * 从池中获取一个对象
     * 如果池为空，则创建新对象
     */
    template<typename... Args>
    unique_obj_ptr acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);

        T* obj = nullptr;
        
        if (!available_.empty()) {
            // 从池中重用对象
            obj = available_.back();
            available_.pop_back();
            ++stats_.reused_count;
        } else {
            // 需要创建新对象
            if (all_objects_.size() >= max_capacity_) {
                // 已达到最大容量，返回空指针
                return nullptr;
            }
            
            // 创建新对象并获取所有权
            auto new_obj = std::make_unique<T>();
            obj = new_obj.get();
            owned_pointers_.insert(obj);  // O(1) 插入到指针集合
            all_objects_.push_back(std::move(new_obj));
            ++stats_.created_count;
        }

        // 使用完美转发重新初始化对象
        if constexpr (sizeof...(args) > 0) {
            *obj = T(std::forward<Args>(args)...);
        } else {
            // 重置为默认状态
            *obj = T{};
        }

        ++stats_.active_count;

        // 返回带自定义删除器的智能指针
        return unique_obj_ptr(obj, [this](T* ptr) { this->release(ptr); });
    }
    
    /**
     * 预分配指定数量的对象
     */
    void reserve(size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t current_total = all_objects_.size();
        if (current_total >= count) {
            return; // 已经有足够的对象
        }
        
        size_t needed = count - current_total;
        size_t can_create = (current_total + needed <= max_capacity_) ? needed : 
                           (max_capacity_ > current_total ? max_capacity_ - current_total : 0);
        
        for (size_t i = 0; i < can_create; ++i) {
            auto new_obj = std::make_unique<T>();
            T* obj_ptr = new_obj.get();
            owned_pointers_.insert(obj_ptr);  // O(1) 插入到指针集合
            all_objects_.push_back(std::move(new_obj));
            available_.push_back(obj_ptr);
            ++stats_.created_count;
        }
    }
    
    /**
     * 清空池中所有对象
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 清空所有容器 - unique_ptr 会自动释放内存
        all_objects_.clear();
        available_.clear();
        owned_pointers_.clear();  // 清空指针集合
        
        // 重置统计信息
        stats_.created_count = 0;
        stats_.reused_count = 0;
        stats_.active_count = 0;
    }
    
    /**
     * 获取池的统计信息
     */
    struct PoolStatistics {
        size_t created_count = 0;    // 创建的对象总数（历史累计，只增不减）
        size_t reused_count = 0;     // 重用的对象总数
        size_t active_count = 0;     // 当前被借出的对象数
        size_t available_count = 0;  // 当前可用对象数
        size_t total_objects = 0;    // 池中对象总数（active + available）
        float reuse_ratio = 0.0f;    // 重用率
    };
    
    PoolStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        PoolStatistics result = stats_;
        result.available_count = available_.size();
        result.total_objects = all_objects_.size();
        
        size_t total_acquisitions = stats_.created_count + stats_.reused_count;
        if (total_acquisitions > 0) {
            result.reuse_ratio = static_cast<float>(stats_.reused_count) / total_acquisitions;
        }
        
        return result;
    }
    
    /**
     * 收缩池大小，释放多余的对象
     * 优化版本：使用批量删除，避免 O(N*M) 复杂度
     */
    void shrink_to_fit(size_t target_available = 32) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (available_.size() <= target_available) {
            return;  // 不需要收缩
        }

        size_t to_remove_count = available_.size() - target_available;

        // 1. 创建一个包含待删除指针的 set，用于 O(1) 查找
        std::unordered_set<T*> pointers_to_remove;
        for (size_t i = 0; i < to_remove_count; ++i) {
            pointers_to_remove.insert(available_[available_.size() - 1 - i]);
        }

        // 2. 使用 std::remove_if 一次性从 all_objects_ 中移除所有待删除的 unique_ptr
        //    复杂度：O(N) 而非 O(N*M)
        auto new_end = std::remove_if(all_objects_.begin(), all_objects_.end(),
            [&](const std::unique_ptr<T>& p) {
                return pointers_to_remove.count(p.get()) > 0;
            });
        all_objects_.erase(new_end, all_objects_.end());

        // 3. 从 owned_pointers_ 中清理待删除的指针
        for (T* ptr : pointers_to_remove) {
            owned_pointers_.erase(ptr);
        }

        // 4. 调整 available_ 大小
        available_.resize(target_available);
        
        // 注意：created_count 保持不变（历史累计）
    }

private:
    mutable std::mutex mutex_;
    
    // 所有对象的唯一所有者
    std::vector<std::unique_ptr<T>> all_objects_;
    
    // 可借用对象的索引（裸指针，不拥有所有权）
    std::vector<T*> available_;
    
    // O(1) 指针合法性检查用的集合
    std::unordered_set<T*> owned_pointers_;
    
    size_t max_capacity_;
    PoolStatistics stats_;
    
    /**
     * 释放对象回池中
     * 优化版本：使用 O(1) 指针合法性检查
     */
    void release(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 使用 O(1) 平均复杂度的查找，而非 O(N) 线性搜索
        if (owned_pointers_.count(obj) > 0) {
            // 重置对象到默认状态
            *obj = T{};
            
            // 将对象放回可用池
            available_.push_back(obj);
            --stats_.active_count;
        }
        // 如果指针不属于池，则静默忽略
        // 在Debug模式下可以添加断言或日志
#ifdef _DEBUG
        else {
            // 可选：在调试模式下记录非法指针访问
            // std::cerr << "Warning: Attempting to release invalid pointer to EventPool" << std::endl;
        }
#endif
    }
};

/**
 * 对象池管理器 - 管理多种类型的事件对象池
 */
class EventPoolManager {
public:
    static EventPoolManager& get_instance() {
        static EventPoolManager instance;
        return instance;
    }
    
    /**
     * 获取指定类型的对象池
     */
    template<typename T>
    EventPool<T>& get_pool() {
        static EventPool<T> pool;
        return pool;
    }
    
    /**
     * 预热所有已知的事件类型池
     */
    void warmup_pools() {
        // 预热常用事件类型的对象池
        // 注意：这些类型需要在实际使用前定义
        // get_pool<struct PoisonedEventComponent>().reserve(32);
        // get_pool<struct BurningEventComponent>().reserve(16);
        // get_pool<struct InvulnerableMarkerComponent>().reserve(8);
        // get_pool<struct HitMarkerComponent>().reserve(16);
    }
    
    /**
     * 清理过期的对象池
     */
    void cleanup_expired_pools() {
        // 由于使用静态池，这里可以是空实现
        // 或者实现池的垃圾收集逻辑
    }
    
    /**
     * 获取所有池的统计信息
     */
    struct GlobalPoolStatistics {
        size_t total_pools = 0;
        size_t total_created = 0;
        size_t total_reused = 0;
        float average_reuse_ratio = 0.0f;
    };
    
    // 注意：实际实现需要维护池的注册表来支持全局统计
    // 这里简化处理
    
private:
    EventPoolManager() = default;
};

/**
 * 无锁事件队列 - 用于多线程环境下的事件处理
 * 
 * 使用无锁队列避免线程同步开销，特别适用于高频事件场景
 */
template<typename T>
class LockFreeEventQueue {
public:
    LockFreeEventQueue(size_t capacity = 4096) : capacity_(capacity) {
        buffer_ = std::make_unique<Event[]>(capacity_);
    }
    
    /**
     * 非阻塞入队操作
     * @return true 成功入队，false 队列已满
     */
    bool enqueue(const T& event) {
        size_t write_pos = write_pos_.load(std::memory_order_relaxed);
        
        while (true) {
            const size_t next_write_pos = (write_pos + 1) % capacity_;
            
            if (next_write_pos == read_pos_.load(std::memory_order_acquire)) {
                // 队列已满
                return false;
            }
            
            // 尝试原子性地更新写位置
            if (write_pos_.compare_exchange_weak(write_pos, next_write_pos, 
                                               std::memory_order_acq_rel, 
                                               std::memory_order_relaxed)) {
                // 成功获取写位置，现在可以安全写入
                buffer_[write_pos].data = event;
                return true;
            }
            // 如果CAS失败，write_pos已经被更新为当前值，继续重试
        }
    }
    
    /**
     * 非阻塞出队操作
     * @return true 成功出队，false 队列为空
     */
    bool dequeue(T& event) {
        size_t read_pos = read_pos_.load(std::memory_order_relaxed);
        
        while (true) {
            if (read_pos == write_pos_.load(std::memory_order_acquire)) {
                // 队列为空
                return false;
            }
            
            const size_t next_read_pos = (read_pos + 1) % capacity_;
            
            // 尝试原子性地更新读位置
            if (read_pos_.compare_exchange_weak(read_pos, next_read_pos,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
                // 成功获取读位置，现在可以安全读取
                event = buffer_[read_pos].data;
                return true;
            }
            // 如果CAS失败，read_pos已经被更新为当前值，继续重试
        }
    }
    
    /**
     * 批量出队操作
     * @param events 输出缓冲区
     * @param max_count 最大出队数量
     * @return 实际出队数量
     */
    size_t dequeue_batch(std::vector<T>& events, size_t max_count) {
        events.clear();
        events.reserve(max_count);
        
        T event;
        size_t count = 0;
        
        while (count < max_count && dequeue(event)) {
            events.push_back(std::move(event));
            ++count;
        }
        
        return count;
    }
    
    /**
     * 检查队列是否为空
     */
    bool empty() const {
        return read_pos_.load(std::memory_order_acquire) == 
               write_pos_.load(std::memory_order_acquire);
    }
    
    /**
     * 获取队列当前大小 (近似值)
     */
    size_t size() const {
        const size_t write_pos = write_pos_.load(std::memory_order_acquire);
        const size_t read_pos = read_pos_.load(std::memory_order_acquire);
        
        if (write_pos >= read_pos) {
            return write_pos - read_pos;
        } else {
            return capacity_ - read_pos + write_pos;
        }
    }

private:
    struct alignas(64) Event {  // 缓存行对齐
        T data;
        Event() = default;
        Event(const T& t) : data(t) {}
        Event& operator=(const T& t) { data = t; return *this; }
        operator T() const { return data; }
    };
    
    std::unique_ptr<Event[]> buffer_;
    size_t capacity_;
    
    // 使用不同的缓存行避免 false sharing
    alignas(64) std::atomic<size_t> write_pos_{0};
    alignas(64) std::atomic<size_t> read_pos_{0};
};

/**
 * 多线程事件调度器 - 集成无锁队列和对象池
 */
class ConcurrentEventDispatcher {
public:
    ConcurrentEventDispatcher() = default;
    ~ConcurrentEventDispatcher() = default;
    
    /**
     * 线程安全的事件入队
     */
    template<typename T>
    bool enqueue_concurrent(const T& event) {
        auto& queue = get_queue<T>();
        return queue.enqueue(event);
    }
    
    /**
     * 批量处理队列中的事件 (主线程调用)
     */
    template<typename T, typename Handler>
    void process_events(Handler&& handler, size_t max_batch_size = 256) {
        auto& queue = get_queue<T>();
        std::vector<T> events;
        
        size_t processed = queue.dequeue_batch(events, max_batch_size);
        
        for (const auto& event : events) {
            handler(event);
        }
        
        // 更新统计
        stats_.total_processed += processed;
    }
    
    /**
     * 获取所有队列的统计信息
     */
    struct ConcurrentStats {
        size_t total_processed = 0;
        size_t total_dropped = 0;     // 队列满时丢弃的事件
        float average_queue_usage = 0.0f;
    };
    
    const ConcurrentStats& get_statistics() const { return stats_; }

private:
    template<typename T>
    LockFreeEventQueue<T>& get_queue() {
        static LockFreeEventQueue<T> queue;
        return queue;
    }
    
    ConcurrentStats stats_;
};

} // namespace portal_core
