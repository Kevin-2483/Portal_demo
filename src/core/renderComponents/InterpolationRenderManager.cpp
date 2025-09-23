#include "InterpolationRenderManager.h"
#include <algorithm>
#include <iostream>
#include "../debug/portal_debug_logging.h"

namespace portal_core
{

    InterpolationRenderManager::InterpolationRenderManager()
        : current_logic_time_(0.0), previous_logic_time_(0.0), logic_tick_interval_(0.0) // 初始化为0，等待initialize设置
          ,
          interpolation_enabled_(true), last_cache_clear_time_(0.0), last_stats_update_(std::chrono::high_resolution_clock::now())
    {
    }

    void InterpolationRenderManager::initialize(float logic_tick_rate)
{
  logic_tick_interval_ = 1.0f / logic_tick_rate;
  
  // 重置时间状态
  current_logic_time_ = 0.0;
  previous_logic_time_ = 0.0;
  
  // 清空缓存
  interpolation_cache_.clear();
  last_cache_clear_time_ = 0.0;
  
  PORTAL_DEBUG_LOG("[DEBUG] InterpolationRenderManager: Initialized with logic tick rate "
            << logic_tick_rate << "Hz (interval: " << logic_tick_interval_ << "s)");
}

    void InterpolationRenderManager::update_logic_time(double current_time)
    {
        // 更新逻辑时间
        previous_logic_time_ = current_logic_time_;
        current_logic_time_ = current_time;
        
        PORTAL_DEBUG_LOG("[DEBUG] InterpolationRenderManager::update_logic_time: "
                  << "previous=" << previous_logic_time_ 
                  << ", current=" << current_logic_time_);
        
        // 清理过期的缓存
        cleanup_cache(current_time);
    }

    const TransformRenderProxy::TransformData *InterpolationRenderManager::get_interpolated_transform(
        entt::registry &registry,
        entt::entity entity,
        double render_time)
    {

        auto start_time = std::chrono::high_resolution_clock::now();

        // 检查实体是否有TransformRenderProxy组件
        if (!registry.all_of<TransformRenderProxy>(entity))
        {
            return nullptr;
        }

        // 检查缓存
        auto cache_it = interpolation_cache_.find(entity);
        if (cache_it != interpolation_cache_.end() && cache_it->second.valid)
        {
            if (std::abs(cache_it->second.cache_time - render_time) < 0.001)
            { // 1ms容差
                return &cache_it->second.transform_data;
            }
        }

        auto &render_proxy = registry.get<TransformRenderProxy>(entity);

        // 如果插值未启用或组件未初始化，返回当前数据
        if (!interpolation_enabled_ || !render_proxy.is_initialized)
        {
            // 添加调试信息
            PORTAL_DEBUG_LOG("InterpolationRenderManager: No interpolation - enabled: "
                      << interpolation_enabled_ << ", initialized: " << render_proxy.is_initialized);

            auto &cache_entry = interpolation_cache_[entity];
            cache_entry.transform_data = render_proxy.get_current();
            cache_entry.cache_time = render_time;
            cache_entry.valid = true;

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            update_statistics(duration);

            return &cache_entry.transform_data;
        }

        // 计算插值因子
        float alpha = calculate_interpolation_alpha(render_time);
        alpha = clamp_alpha(alpha);

        // 获取插值后的变换数据
        auto &cache_entry = interpolation_cache_[entity];
        cache_entry.transform_data = render_proxy.get_interpolated(alpha);
        cache_entry.cache_time = render_time;
        cache_entry.valid = true;

        // 更新统计信息
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        update_statistics(duration);

        return &cache_entry.transform_data;
    }

    void InterpolationRenderManager::get_all_interpolated_transforms(
        entt::registry &registry,
        double render_time,
        std::unordered_map<entt::entity, TransformRenderProxy::TransformData> &out_transforms)
    {

        out_transforms.clear();

        if (!interpolation_enabled_)
        {
            // 如果插值未启用，直接返回当前数据
            auto view = registry.view<TransformRenderProxy>();
            for (auto entity : view)
            {
                auto &render_proxy = registry.get<TransformRenderProxy>(entity);
                out_transforms[entity] = render_proxy.get_current();
            }
            return;
        }

        // 计算插值因子
        float alpha = calculate_interpolation_alpha(render_time);
        alpha = clamp_alpha(alpha);

        // 批量处理所有实体
        auto view = registry.view<TransformRenderProxy>();
        for (auto entity : view)
        {
            auto &render_proxy = registry.get<TransformRenderProxy>(entity);

            if (render_proxy.is_initialized)
            {
                out_transforms[entity] = render_proxy.get_interpolated(alpha);
            }
            else
            {
                out_transforms[entity] = render_proxy.get_current();
            }
        }

        // 更新统计信息
        statistics_.interpolated_entities_count = out_transforms.size();
        statistics_.last_interpolation_alpha = alpha;
    }

    float InterpolationRenderManager::calculate_interpolation_alpha(double render_time) const
    {
        if (logic_tick_interval_ <= 0.0)
        {
            return 1.0f;
        }

        // 计算渲染时间相对于当前逻辑tick的位置
        // alpha表示从current_logic_time_向下一个tick的进度
        double time_since_current = render_time - current_logic_time_;
        double time_between_ticks = current_logic_time_ - previous_logic_time_;

        float alpha;
        if (time_between_ticks > 0.0)
        {
            alpha = static_cast<float>(time_since_current / time_between_ticks);
        }
        else
        {
            alpha = 0.0f;
        }

        // 调试信息
        PORTAL_DEBUG_LOG("[DEBUG] render_time: " << render_time
                  << ", previous_logic_time_: " << previous_logic_time_
                  << ", current_logic_time_: " << current_logic_time_
                  << ", logic_tick_interval_: " << logic_tick_interval_
                  << ", time_since_current: " << time_since_current
                  << ", time_between_ticks: " << time_between_ticks
                  << ", alpha: " << alpha);

        // 确保alpha在0-1范围内
        return std::max(0.0f, std::min(1.0f, alpha));
    }

    void InterpolationRenderManager::reset_statistics()
    {
        statistics_ = Statistics();
        last_stats_update_ = std::chrono::high_resolution_clock::now();
    }

    void InterpolationRenderManager::cleanup_cache(double current_time) const
    {
        if (current_time - last_cache_clear_time_ < CACHE_LIFETIME)
        {
            return; // 还没到清理时间
        }

        // 清理过期的缓存条目
        auto it = interpolation_cache_.begin();
        while (it != interpolation_cache_.end())
        {
            if (current_time - it->second.cache_time > CACHE_LIFETIME)
            {
                it = interpolation_cache_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        last_cache_clear_time_ = current_time;
    }

    void InterpolationRenderManager::update_statistics(double interpolation_time) const
    {
        statistics_.total_interpolations++;

        // 计算平均插值时间（简单的移动平均）
        const double alpha = 0.1; // 平滑因子
        statistics_.average_interpolation_time_ms =
            statistics_.average_interpolation_time_ms * (1.0 - alpha) +
            interpolation_time * alpha;
    }

    float InterpolationRenderManager::clamp_alpha(float alpha) const
    {
        // 将插值因子限制在合理范围内
        // 允许轻微的外推（负值和大于1的值），但不要过度
        return std::clamp(alpha, -0.1f, 1.1f);
    }

} // namespace portal_core