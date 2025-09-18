#ifndef RENDER_SYNC_SYSTEM_H
#define RENDER_SYNC_SYSTEM_H

#include "../system_base.h"
#include "../components/transform_component.h"
#include "../renderComponents/TransformRenderProxy.h"
#include <entt/entt.hpp>

namespace portal_core {

/**
 * 渲染同步系统
 * 
 * 负责将逻辑组件的数据同步到渲染代理组件。
 * 这个系统在每个逻辑tick的最后执行，确保渲染数据是最新的。
 * 
 * 职责：
 * 1. 从TransformComponent同步数据到TransformRenderProxy
 * 2. 管理渲染代理组件的生命周期
 * 3. 优化：只在数据变化时进行同步
 * 4. 双缓冲：将当前数据移动到历史数据，更新当前数据
 * 
 * 执行时机：
 * - 在所有逻辑系统执行完毕后
 * - 每个逻辑tick执行一次
 * - 优先级设置为最低，确保最后执行
 */
class RenderSyncSystem : public ISystem {
public:
    RenderSyncSystem();
    virtual ~RenderSyncSystem() = default;

    // 系统基础接口
    virtual bool initialize() override;
    virtual void update(entt::registry& registry, float delta_time) override;
    virtual void cleanup() override;
    
    // 系统信息
    virtual const char* get_name() const override { return "RenderSyncSystem"; }
    
    // 依赖关系（在所有逻辑系统之后执行）
    virtual std::vector<std::string> get_dependencies() const override;

private:
    // 当前逻辑tick的时间戳
    double current_tick_time;
    
    // 同步计数器（用于调试和统计）
    size_t sync_count;
    
    /**
     * 同步单个实体的变换数据
     * @param registry ECS注册表
     * @param entity 要同步的实体
     */
    void sync_transform_entity(entt::registry& registry, entt::entity entity);
    
    /**
     * 为实体创建渲染代理组件
     * @param registry ECS注册表
     * @param entity 目标实体
     */
    void create_render_proxy_for_entity(entt::registry& registry, entt::entity entity);
    
    /**
     * 清理无效的渲染代理组件
     * @param registry ECS注册表
     */
    void cleanup_orphaned_proxies(entt::registry& registry);
    
    /**
     * 更新系统统计信息
     */
    void update_statistics(entt::registry& registry);
    
    // 事件处理：当TransformComponent被添加时
    void on_transform_component_added(entt::registry& registry, entt::entity entity);
    
    // 事件处理：当TransformComponent被移除时
    void on_transform_component_removed(entt::registry& registry, entt::entity entity);
};

// 自动注册系统到POST_UPDATE阶段
REGISTER_SYSTEM_POST_UPDATE(RenderSyncSystem, 0, 0);

} // namespace portal_core

#endif // RENDER_SYNC_SYSTEM_H

