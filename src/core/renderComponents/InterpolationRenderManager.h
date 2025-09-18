#ifndef INTERPOLATION_RENDER_MANAGER_H
#define INTERPOLATION_RENDER_MANAGER_H

#include "TransformRenderProxy.h"
#include <entt/entt.hpp>
#include <chrono>

namespace portal_core {

/**
 * 插值渲染管理器
 * 
 * 负责管理渲染插值计算，为Godot端提供平滑的渲染数据。
 * 这个管理器计算逻辑tick之间的插值，确保渲染帧率与逻辑帧率解耦。
 * 
 * 核心功能：
 * 1. 时间管理：跟踪逻辑tick时间和渲染时间
 * 2. 插值计算：基于时间差计算插值因子
 * 3. 数据提供：为Godot端提供插值后的变换数据
 * 4. 性能优化：缓存计算结果，避免重复计算
 * 
 * 工作原理：
 * - 逻辑系统以固定频率更新（如60Hz）
 * - 渲染系统以可变频率更新（如120Hz、144Hz等）
 * - 插值管理器在两个逻辑tick之间提供平滑的中间值
 */
class InterpolationRenderManager {
public:
    InterpolationRenderManager();
    ~InterpolationRenderManager() = default;

    /**
     * 初始化管理器
     * @param logic_tick_rate 逻辑tick频率（Hz）
     */
    void initialize(float logic_tick_rate = 60.0f);

    /**
     * 更新逻辑tick时间戳
     * 在每个逻辑tick结束时调用
     * @param current_time 当前逻辑时间
     */
    void update_logic_time(double current_time);

    /**
     * 获取指定实体的插值变换数据
     * 在Godot渲染帧中调用
     * @param registry ECS注册表
     * @param entity 目标实体
     * @param render_time 当前渲染时间
     * @return 插值后的变换数据，如果实体无效返回nullptr
     */
    const TransformRenderProxy::TransformData* get_interpolated_transform(
        entt::registry& registry, 
        entt::entity entity, 
        double render_time
    );

    /**
     * 批量获取所有实体的插值变换数据
     * 用于优化批量渲染更新
     * @param registry ECS注册表
     * @param render_time 当前渲染时间
     * @param out_transforms 输出的变换数据映射
     */
    void get_all_interpolated_transforms(
        entt::registry& registry,
        double render_time,
        std::unordered_map<entt::entity, TransformRenderProxy::TransformData>& out_transforms
    );

    /**
     * 计算插值因子
     * @param render_time 当前渲染时间
     * @return 插值因子 (0.0 = previous tick, 1.0 = current tick)
     */
    float calculate_interpolation_alpha(double render_time) const;

    /**
     * 获取当前逻辑tick时间
     */
    double get_current_logic_time() const { return current_logic_time_; }

    /**
     * 获取上一个逻辑tick时间
     */
    double get_previous_logic_time() const { return previous_logic_time_; }

    /**
     * 获取逻辑tick间隔
     */
    double get_logic_tick_interval() const { return logic_tick_interval_; }

    /**
     * 设置插值启用状态
     * @param enabled 是否启用插值
     */
    void set_interpolation_enabled(bool enabled) { interpolation_enabled_ = enabled; }

    /**
     * 检查插值是否启用
     */
    bool is_interpolation_enabled() const { return interpolation_enabled_; }

    /**
     * 获取统计信息
     */
    struct Statistics {
        size_t interpolated_entities_count = 0;
        double last_interpolation_alpha = 0.0;
        double average_interpolation_time_ms = 0.0;
        size_t total_interpolations = 0;
    };

    const Statistics& get_statistics() const { return statistics_; }

    /**
     * 重置统计信息
     */
    void reset_statistics();

private:
    // 时间管理
    double current_logic_time_;      // 当前逻辑tick时间
    double previous_logic_time_;     // 上一个逻辑tick时间
    double logic_tick_interval_;     // 逻辑tick间隔（秒）
    bool interpolation_enabled_;     // 插值启用标志

    // 性能优化缓存
    struct CacheEntry {
        TransformRenderProxy::TransformData transform_data;
        double cache_time;
        bool valid;
        
        CacheEntry() : cache_time(0.0), valid(false) {}
    };
    
    mutable std::unordered_map<entt::entity, CacheEntry> interpolation_cache_;
    mutable double last_cache_clear_time_;
    static constexpr double CACHE_LIFETIME = 0.016; // 16ms缓存生命周期

    // 统计信息
    mutable Statistics statistics_;
    mutable std::chrono::high_resolution_clock::time_point last_stats_update_;

    /**
     * 清理过期的缓存条目
     * @param current_time 当前时间
     */
    void cleanup_cache(double current_time) const;

    /**
     * 更新统计信息
     * @param interpolation_time 插值计算耗时
     */
    void update_statistics(double interpolation_time) const;

    /**
     * 验证插值因子的有效性
     * @param alpha 插值因子
     * @return 修正后的插值因子
     */
    float clamp_alpha(float alpha) const;
};

} // namespace portal_core

#endif // INTERPOLATION_RENDER_MANAGER_H