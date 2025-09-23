#include "RenderSyncSystem.h"
#include "../components/transform_component.h"
#include "../renderComponents/TransformRenderProxy.h"
#include <iostream>
#include "../debug/portal_debug_logging.h"

namespace portal_core {

RenderSyncSystem::RenderSyncSystem() 
    : current_tick_time(0.0), sync_count(0) {
}

bool RenderSyncSystem::initialize() {
    PORTAL_DEBUG_LOG("RenderSyncSystem: Initializing...");
    current_tick_time = 0.0;
    sync_count = 0;
    return true;
}

void RenderSyncSystem::update(entt::registry& registry, float delta_time) {
    // 更新当前tick时间
    current_tick_time += delta_time;
    
    // 重置同步计数器
    sync_count = 0;
    
    // 1. 为新的TransformComponent创建对应的渲染代理组件
    auto transform_view = registry.view<TransformComponent>();
    for (auto [entity, transform] : transform_view.each()) {
        if (!registry.all_of<TransformRenderProxy>(entity)) {
            create_render_proxy_for_entity(registry, entity);
        }
    }
    
    // 2. 同步所有有TransformComponent的实体
    auto sync_view = registry.view<TransformComponent, TransformRenderProxy>();
    for (auto [entity, transform, render_proxy] : sync_view.each()) {
        sync_transform_entity(registry, entity);
    }
    
    // 3. 清理孤立的渲染代理组件
    cleanup_orphaned_proxies(registry);
    
    // 4. 更新统计信息
    update_statistics(registry);
    
    if (sync_count > 0) {
        PORTAL_DEBUG_LOG("RenderSyncSystem: Synced " << sync_count << " entities");
    }
}

void RenderSyncSystem::cleanup() {
    PORTAL_DEBUG_LOG("RenderSyncSystem: Cleaning up...");
}

std::vector<std::string> RenderSyncSystem::get_dependencies() const {
    // 渲染同步系统应该在所有逻辑系统之后执行
    // 这里列出主要的逻辑系统，确保它们先执行
    return {
        "PhysicsSystem",
        "PhysicsCommandSystem", 
        "PhysicsQuerySystem",
        "XRotationSystem",
        "YRotationSystem", 
        "ZRotationSystem"
    };
}

void RenderSyncSystem::sync_transform_entity(entt::registry& registry, entt::entity entity) {
    auto& transform = registry.get<TransformComponent>(entity);
    auto& render_proxy = registry.get<TransformRenderProxy>(entity);
    
    // 检查数据是否发生变化（优化：避免不必要的更新）
    if (render_proxy.has_changed(transform.position, transform.rotation, transform.scale)) {
        // 更新渲染代理组件
        render_proxy.update_from_transform(
            transform.position, 
            transform.rotation, 
            transform.scale, 
            current_tick_time
        );
        
        sync_count++;
    }
}

void RenderSyncSystem::create_render_proxy_for_entity(entt::registry& registry, entt::entity entity) {
    auto& transform = registry.get<TransformComponent>(entity);
    
    // 创建新的渲染代理组件
    auto& render_proxy = registry.emplace<TransformRenderProxy>(entity);
    
    // 初始化渲染代理组件（设置初始状态，避免插值问题）
    render_proxy.set_initial_transform(
        transform.position,
        transform.rotation, 
        transform.scale,
        current_tick_time
    );
    
    PORTAL_DEBUG_LOG("RenderSyncSystem: Created TransformRenderProxy for entity " 
              << static_cast<uint32_t>(entity));
}

void RenderSyncSystem::cleanup_orphaned_proxies(entt::registry& registry) {
    // 查找所有有TransformRenderProxy但没有TransformComponent的实体
    std::vector<entt::entity> orphaned_entities;
    
    auto proxy_view = registry.view<TransformRenderProxy>();
    for (auto [entity, proxy] : proxy_view.each()) {
        if (!registry.all_of<TransformComponent>(entity)) {
            orphaned_entities.push_back(entity);
        }
    }
    
    // 移除孤立的渲染代理组件
    for (auto entity : orphaned_entities) {
        registry.remove<TransformRenderProxy>(entity);
        PORTAL_DEBUG_LOG("RenderSyncSystem: Removed orphaned TransformRenderProxy from entity " 
                  << static_cast<uint32_t>(entity));
    }
}

void RenderSyncSystem::update_statistics(entt::registry& registry) {
    // 这里可以收集和更新系统统计信息
    // 例如：同步的实体数量、性能指标等
    
    // 当前实现只是简单的计数，将来可以扩展
    auto transform_count = registry.view<TransformComponent>().size();
    auto proxy_count = registry.view<TransformRenderProxy>().size();
    
    // 可以在调试模式下输出更详细的统计信息
    #ifdef DEBUG
    if (sync_count > 0) {
        PORTAL_DEBUG_LOG("RenderSyncSystem Stats - Transform entities: " << transform_count 
                  << ", Proxy entities: " << proxy_count 
                  << ", Synced this frame: " << sync_count);
    }
    #endif
}

void RenderSyncSystem::on_transform_component_added(entt::registry& registry, entt::entity entity) {
    // 当TransformComponent被添加时，自动创建对应的渲染代理组件
    // 这个方法可以通过事件系统调用，但目前在update中处理
    create_render_proxy_for_entity(registry, entity);
}

void RenderSyncSystem::on_transform_component_removed(entt::registry& registry, entt::entity entity) {
    // 当TransformComponent被移除时，自动移除对应的渲染代理组件
    if (registry.all_of<TransformRenderProxy>(entity)) {
        registry.remove<TransformRenderProxy>(entity);
        PORTAL_DEBUG_LOG("RenderSyncSystem: Removed TransformRenderProxy for removed TransformComponent on entity " 
                  << static_cast<uint32_t>(entity));
    }
}

} // namespace portal_core
