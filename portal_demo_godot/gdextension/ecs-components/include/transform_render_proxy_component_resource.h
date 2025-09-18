#pragma once

#include "ipresettable_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node3d.hpp>

// 引入C++ ECS组件定义
#include "core/renderComponents/TransformRenderProxy.h"
#include "core/renderComponents/InterpolationRenderManager.h"
#include "core/components/transform_component.h"

using namespace godot;

/**
 * 变换渲染代理组件资源
 * 
 * 这个组件资源用于管理实体的渲染变换数据，支持插值渲染。
 * 它从C++核心的TransformRenderProxy组件同步数据，并将插值后的
 * 变换应用到Godot节点上，实现平滑的渲染效果。
 * 
 * 主要功能：
 * 1. 从TransformRenderProxy获取插值变换数据
 * 2. 将变换数据同步到Godot节点
 * 3. 支持渲染频率与逻辑频率解耦
 * 4. 提供调试和统计信息
 * 
 * 继承自IPresettableResource，支持预设保存/加载功能
 */
class TransformRenderProxyComponentResource : public IPresettableResource
{
    GDCLASS(TransformRenderProxyComponentResource, IPresettableResource)

private:
    // 插值设置
    bool interpolation_enabled;
    
    // 调试设置
    bool debug_mode;
    bool show_interpolation_info;
    
    // 性能设置
    bool use_cache;
    float cache_lifetime;
    
    // 同步设置
    bool auto_sync_to_node;
    bool sync_position;
    bool sync_rotation;
    bool sync_scale;
    
    // 插值渲染管理器的静态实例
    static portal_core::InterpolationRenderManager* interpolation_manager;

protected:
    static void _bind_methods();

public:
    TransformRenderProxyComponentResource();
    ~TransformRenderProxyComponentResource();

    // 插值设置
    void set_interpolation_enabled(bool p_enabled);
    bool get_interpolation_enabled() const;
    
    // 调试设置
    void set_debug_mode(bool p_debug);
    bool get_debug_mode() const;
    void set_show_interpolation_info(bool p_show);
    bool get_show_interpolation_info() const;
    
    // 性能设置
    void set_use_cache(bool p_use);
    bool get_use_cache() const;
    void set_cache_lifetime(float p_lifetime);
    float get_cache_lifetime() const;
    
    // 同步设置
    void set_auto_sync_to_node(bool p_auto);
    bool get_auto_sync_to_node() const;
    void set_sync_position(bool p_sync);
    bool get_sync_position() const;
    void set_sync_rotation(bool p_sync);
    bool get_sync_rotation() const;
    void set_sync_scale(bool p_sync);
    bool get_sync_scale() const;

    // IPresettableResource接口实现
    virtual bool apply_to_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool remove_from_entity(entt::registry& registry, entt::entity entity) override;
    virtual bool has_component(const entt::registry& registry, entt::entity entity) const override;
    virtual String get_component_type_name() const override;
    
    /**
     * 核心同步方法：将插值后的变换数据同步到Godot节点
     * 这个方法会调用插值渲染管理器获取当前帧的插值数据
     * @param registry ECS注册表
     * @param entity 目标实体
     * @param target_node 目标Godot节点
     */
    virtual void sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node) override;

    // 预设相关
    virtual String get_preset_display_name() const override {
        return "Transform Render Proxy";
    }

    // 约束验证
    String get_constraint_warnings() const override;

    // 自动填充功能
    virtual Array get_auto_fill_capabilities() const override;
    virtual Dictionary auto_fill_from_node(Node* target_node, const String& capability_name = "") override;

    // 插值管理器相关
    static void set_interpolation_manager(portal_core::InterpolationRenderManager* manager);
    static portal_core::InterpolationRenderManager* get_interpolation_manager();

    // 调试和统计信息
    Dictionary get_interpolation_statistics() const;
    String get_debug_info(entt::registry& registry, entt::entity entity) const;

private:
    /**
     * 将C++变换数据转换为Godot Transform3D
     */
    Transform3D cpp_transform_to_godot(const portal_core::TransformRenderProxy::TransformData& transform_data) const;
    
    /**
     * 应用变换到Node3D节点
     */
    void apply_transform_to_node3d(Node3D* node3d, const Transform3D& transform) const;
    
    /**
     * 获取当前渲染时间
     */
    double get_current_render_time() const;
    
    /**
     * 输出调试信息
     */
    void log_debug_info(const String& message) const;
    
    /**
     * 从Node3D自动填充设置
     */
    Dictionary auto_fill_from_node3d(Node* node);
};