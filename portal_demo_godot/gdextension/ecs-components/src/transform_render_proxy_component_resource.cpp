#include "transform_render_proxy_component_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <chrono>
#include "game_core_manager.h"
#include "component_registrar.h"

// Include the necessary C++ ECS components
#include "core/components/transform_component.h"
#include "core/renderComponents/TransformRenderProxy.h"

using namespace godot;

// 静态成员初始化
portal_core::InterpolationRenderManager *TransformRenderProxyComponentResource::interpolation_manager = nullptr;

void TransformRenderProxyComponentResource::_bind_methods()
{
    // 插值设置
    ClassDB::bind_method(D_METHOD("set_interpolation_enabled", "enabled"), &TransformRenderProxyComponentResource::set_interpolation_enabled);
    ClassDB::bind_method(D_METHOD("get_interpolation_enabled"), &TransformRenderProxyComponentResource::get_interpolation_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interpolation_enabled"), "set_interpolation_enabled", "get_interpolation_enabled");

    // 调试设置
    ClassDB::bind_method(D_METHOD("set_debug_mode", "debug"), &TransformRenderProxyComponentResource::set_debug_mode);
    ClassDB::bind_method(D_METHOD("get_debug_mode"), &TransformRenderProxyComponentResource::get_debug_mode);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_mode"), "set_debug_mode", "get_debug_mode");

    ClassDB::bind_method(D_METHOD("set_show_interpolation_info", "show"), &TransformRenderProxyComponentResource::set_show_interpolation_info);
    ClassDB::bind_method(D_METHOD("get_show_interpolation_info"), &TransformRenderProxyComponentResource::get_show_interpolation_info);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_interpolation_info"), "set_show_interpolation_info", "get_show_interpolation_info");

    // 性能设置
    ClassDB::bind_method(D_METHOD("set_use_cache", "use"), &TransformRenderProxyComponentResource::set_use_cache);
    ClassDB::bind_method(D_METHOD("get_use_cache"), &TransformRenderProxyComponentResource::get_use_cache);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_cache"), "set_use_cache", "get_use_cache");

    ClassDB::bind_method(D_METHOD("set_cache_lifetime", "lifetime"), &TransformRenderProxyComponentResource::set_cache_lifetime);
    ClassDB::bind_method(D_METHOD("get_cache_lifetime"), &TransformRenderProxyComponentResource::get_cache_lifetime);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cache_lifetime", PROPERTY_HINT_RANGE, "0.001,1.0,0.001"), "set_cache_lifetime", "get_cache_lifetime");

    // 同步设置
    ClassDB::bind_method(D_METHOD("set_auto_sync_to_node", "auto"), &TransformRenderProxyComponentResource::set_auto_sync_to_node);
    ClassDB::bind_method(D_METHOD("get_auto_sync_to_node"), &TransformRenderProxyComponentResource::get_auto_sync_to_node);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_to_node"), "set_auto_sync_to_node", "get_auto_sync_to_node");

    ClassDB::bind_method(D_METHOD("set_sync_position", "sync"), &TransformRenderProxyComponentResource::set_sync_position);
    ClassDB::bind_method(D_METHOD("get_sync_position"), &TransformRenderProxyComponentResource::get_sync_position);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_position"), "set_sync_position", "get_sync_position");

    ClassDB::bind_method(D_METHOD("set_sync_rotation", "sync"), &TransformRenderProxyComponentResource::set_sync_rotation);
    ClassDB::bind_method(D_METHOD("get_sync_rotation"), &TransformRenderProxyComponentResource::get_sync_rotation);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_rotation"), "set_sync_rotation", "get_sync_rotation");

    ClassDB::bind_method(D_METHOD("set_sync_scale", "sync"), &TransformRenderProxyComponentResource::set_sync_scale);
    ClassDB::bind_method(D_METHOD("get_sync_scale"), &TransformRenderProxyComponentResource::get_sync_scale);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_scale"), "set_sync_scale", "get_sync_scale");

    // 调试和统计方法
    ClassDB::bind_method(D_METHOD("get_interpolation_statistics"), &TransformRenderProxyComponentResource::get_interpolation_statistics);
    // 注意：get_debug_info方法不能直接绑定到Godot，因为entt类型不支持

    // 静态方法
    // 注意：静态方法也不能直接绑定portal_core类型到Godot
    // ClassDB::bind_static_method("TransformRenderProxyComponentResource", D_METHOD("set_interpolation_manager", "manager"), &TransformRenderProxyComponentResource::set_interpolation_manager);
    // ClassDB::bind_static_method("TransformRenderProxyComponentResource", D_METHOD("get_interpolation_manager"), &TransformRenderProxyComponentResource::get_interpolation_manager);
}

TransformRenderProxyComponentResource::TransformRenderProxyComponentResource()
    : interpolation_enabled(true), debug_mode(true), show_interpolation_info(true), use_cache(true), cache_lifetime(0.016f) // 16ms 默认缓存生命周期
      ,
      auto_sync_to_node(true), sync_position(true), sync_rotation(true), sync_scale(true)
{
}

TransformRenderProxyComponentResource::~TransformRenderProxyComponentResource()
{
}

// Getter 和 Setter 方法实现
void TransformRenderProxyComponentResource::set_interpolation_enabled(bool p_enabled)
{
    interpolation_enabled = p_enabled;
}

bool TransformRenderProxyComponentResource::get_interpolation_enabled() const
{
    return interpolation_enabled;
}

void TransformRenderProxyComponentResource::set_debug_mode(bool p_debug)
{
    debug_mode = p_debug;
}

bool TransformRenderProxyComponentResource::get_debug_mode() const
{
    return debug_mode;
}

void TransformRenderProxyComponentResource::set_show_interpolation_info(bool p_show)
{
    show_interpolation_info = p_show;
}

bool TransformRenderProxyComponentResource::get_show_interpolation_info() const
{
    return show_interpolation_info;
}

void TransformRenderProxyComponentResource::set_use_cache(bool p_use)
{
    use_cache = p_use;
}

bool TransformRenderProxyComponentResource::get_use_cache() const
{
    return use_cache;
}

void TransformRenderProxyComponentResource::set_cache_lifetime(float p_lifetime)
{
    cache_lifetime = p_lifetime;
}

float TransformRenderProxyComponentResource::get_cache_lifetime() const
{
    return cache_lifetime;
}

void TransformRenderProxyComponentResource::set_auto_sync_to_node(bool p_auto)
{
    auto_sync_to_node = p_auto;
}

bool TransformRenderProxyComponentResource::get_auto_sync_to_node() const
{
    return auto_sync_to_node;
}

void TransformRenderProxyComponentResource::set_sync_position(bool p_sync)
{
    sync_position = p_sync;
}

bool TransformRenderProxyComponentResource::get_sync_position() const
{
    return sync_position;
}

void TransformRenderProxyComponentResource::set_sync_rotation(bool p_sync)
{
    sync_rotation = p_sync;
}

bool TransformRenderProxyComponentResource::get_sync_rotation() const
{
    return sync_rotation;
}

void TransformRenderProxyComponentResource::set_sync_scale(bool p_sync)
{
    sync_scale = p_sync;
}

bool TransformRenderProxyComponentResource::get_sync_scale() const
{
    return sync_scale;
}

// IPresettableResource 接口实现
bool TransformRenderProxyComponentResource::apply_to_entity(entt::registry &registry, entt::entity entity)
{
    if (debug_mode)
    {
        log_debug_info("Applying TransformRenderProxy to entity " + String::num_int64(static_cast<uint32_t>(entity)));
    }

    // 确保实体有 TransformComponent
    if (!registry.all_of<portal_core::TransformComponent>(entity))
    {
        if (debug_mode)
        {
            log_debug_info("Entity missing TransformComponent, cannot apply TransformRenderProxy");
        }
        return false;
    }

    // 添加或更新 TransformRenderProxy 组件
    auto &render_proxy = registry.get_or_emplace<portal_core::TransformRenderProxy>(entity);

    // 从 TransformComponent 初始化渲染代理
    const auto &transform_comp = registry.get<portal_core::TransformComponent>(entity);
    render_proxy.set_initial_transform(
        transform_comp.position,
        transform_comp.rotation,
        transform_comp.scale,
        get_current_render_time());

    if (debug_mode)
    {
        log_debug_info("Successfully applied TransformRenderProxy to entity");
    }

    return true;
}

bool TransformRenderProxyComponentResource::remove_from_entity(entt::registry &registry, entt::entity entity)
{
    if (registry.all_of<portal_core::TransformRenderProxy>(entity))
    {
        registry.remove<portal_core::TransformRenderProxy>(entity);

        if (debug_mode)
        {
            log_debug_info("Removed TransformRenderProxy from entity " + String::num_int64(static_cast<uint32_t>(entity)));
        }
        return true;
    }
    return false;
}

bool TransformRenderProxyComponentResource::has_component(const entt::registry &registry, entt::entity entity) const
{
    return registry.all_of<portal_core::TransformRenderProxy>(entity);
}

String TransformRenderProxyComponentResource::get_component_type_name() const
{
    return "TransformRenderProxy";
}

// 核心同步方法
void TransformRenderProxyComponentResource::sync_to_node(entt::registry &registry, entt::entity entity, Node *target_node)
{
    if (!target_node)
    {
        if (debug_mode)
        {
            log_debug_info("Target node is null, cannot sync");
        }
        return;
    }

    // 检查是否有 TransformRenderProxy 组件
    if (!registry.all_of<portal_core::TransformRenderProxy>(entity))
    {
        if (debug_mode)
        {
            log_debug_info("Entity missing TransformRenderProxy component");
        }
        return;
    }

    // 尝试转换为 Node3D
    Node3D *node3d = Object::cast_to<Node3D>(target_node);
    if (!node3d)
    {
        if (debug_mode)
        {
            log_debug_info("Target node is not a Node3D, cannot apply transform");
        }
        return;
    }

    // 获取插值后的变换数据
    Transform3D interpolated_transform;

    if (interpolation_enabled && interpolation_manager)
    {
        // 使用插值渲染管理器获取平滑的变换数据
        double render_time = get_current_render_time();
        const auto *transform_data = interpolation_manager->get_interpolated_transform(registry, entity, render_time);

        if (transform_data)
        {
            interpolated_transform = cpp_transform_to_godot(*transform_data);

            if (debug_mode && show_interpolation_info)
            {
                float alpha = interpolation_manager->calculate_interpolation_alpha(render_time);
                log_debug_info("Using interpolated transform, alpha: " + String::num(alpha, 8));
            }
        }
        else
        {
            // 回退到当前帧数据
            const auto &render_proxy = registry.get<portal_core::TransformRenderProxy>(entity);
            interpolated_transform = cpp_transform_to_godot(render_proxy.current);

            if (debug_mode)
            {
                log_debug_info("Interpolation failed, using current frame data");
            }
        }
    }
    else
    {
        // 直接使用当前帧数据（无插值）
        const auto &render_proxy = registry.get<portal_core::TransformRenderProxy>(entity);
        interpolated_transform = cpp_transform_to_godot(render_proxy.current);

        if (debug_mode)
        {
            log_debug_info("Using non-interpolated transform");
        }
    }

    // 应用变换到节点
    apply_transform_to_node3d(node3d, interpolated_transform);
}

// 约束验证
String TransformRenderProxyComponentResource::get_constraint_warnings() const
{
    Array warnings;

    // 检查插值管理器
    if (interpolation_enabled && !interpolation_manager)
    {
        warnings.push_back("Interpolation enabled but InterpolationRenderManager not set");
    }

    // 检查缓存设置
    if (use_cache && cache_lifetime <= 0.0f)
    {
        warnings.push_back("Cache enabled but cache_lifetime is invalid (must be > 0)");
    }

    // 检查同步设置
    if (!sync_position && !sync_rotation && !sync_scale)
    {
        warnings.push_back("All sync options disabled - component will have no effect");
    }

    if (warnings.size() == 0)
    {
        return "";
    }

    String result = "Transform Render Proxy Warnings:\n";
    for (int i = 0; i < warnings.size(); i++)
    {
        result += "• " + warnings[i].operator String() + "\n";
    }
    return result;
}

// 自动填充功能
Array TransformRenderProxyComponentResource::get_auto_fill_capabilities() const
{
    Array capabilities;
    capabilities.push_back("from_node3d");
    return capabilities;
}

Dictionary TransformRenderProxyComponentResource::auto_fill_from_node(Node *target_node, const String &capability_name)
{
    Dictionary result;
    result["success"] = false;
    result["message"] = "";

    if (!target_node)
    {
        result["message"] = "Target node is null";
        return result;
    }

    if (capability_name == "from_node3d" || capability_name == "")
    {
        return auto_fill_from_node3d(target_node);
    }

    result["message"] = "Unknown capability: " + capability_name;
    return result;
}

// 静态方法
void TransformRenderProxyComponentResource::set_interpolation_manager(portal_core::InterpolationRenderManager *manager)
{
    interpolation_manager = manager;
}

portal_core::InterpolationRenderManager *TransformRenderProxyComponentResource::get_interpolation_manager()
{
    return interpolation_manager;
}

// 调试和统计信息
Dictionary TransformRenderProxyComponentResource::get_interpolation_statistics() const
{
    Dictionary stats;

    if (interpolation_manager)
    {
        auto manager_stats = interpolation_manager->get_statistics();
        stats["interpolated_entities_count"] = static_cast<int>(manager_stats.interpolated_entities_count);
        stats["last_interpolation_alpha"] = manager_stats.last_interpolation_alpha;
        stats["average_interpolation_time_ms"] = manager_stats.average_interpolation_time_ms;
        stats["total_interpolations"] = static_cast<int>(manager_stats.total_interpolations);
        stats["interpolation_enabled"] = interpolation_enabled;
    }
    else
    {
        stats["error"] = "InterpolationRenderManager not available";
        stats["interpolation_enabled"] = false;
    }

    return stats;
}

String TransformRenderProxyComponentResource::get_debug_info(entt::registry &registry, entt::entity entity) const
{
    String info = "TransformRenderProxy Debug Info:\n";

    // 基本状态
    info += "• Interpolation Enabled: " + String(interpolation_enabled ? "Yes" : "No") + "\n";
    info += "• Debug Mode: " + String(debug_mode ? "Yes" : "No") + "\n";
    info += "• Use Cache: " + String(use_cache ? "Yes" : "No") + "\n";

    // 同步设置
    info += "• Sync Position: " + String(sync_position ? "Yes" : "No") + "\n";
    info += "• Sync Rotation: " + String(sync_rotation ? "Yes" : "No") + "\n";
    info += "• Sync Scale: " + String(sync_scale ? "Yes" : "No") + "\n";

    // 组件状态
    if (has_component(registry, entity))
    {
        const auto &render_proxy = registry.get<portal_core::TransformRenderProxy>(entity);
        info += "• Component Present: Yes\n";
        info += "• Current Position: " + String("({0}, {1}, {2})").format(Array::make(render_proxy.current.position.GetX(), render_proxy.current.position.GetY(), render_proxy.current.position.GetZ())) + "\n";
        info += "• Needs Update: " + String(render_proxy.is_dirty() ? "Yes" : "No") + "\n";
        info += "• Last Update Time: " + String::num(render_proxy.last_update_time) + "\n";
    }
    else
    {
        info += "• Component Present: No\n";
    }

    // 插值管理器状态
    if (interpolation_manager)
    {
        info += "• Interpolation Manager: Available\n";
        info += "• Current Logic Time: " + String::num(interpolation_manager->get_current_logic_time()) + "\n";
        info += "• Logic Tick Interval: " + String::num(interpolation_manager->get_logic_tick_interval()) + "\n";
    }
    else
    {
        info += "• Interpolation Manager: Not Available\n";
    }

    return info;
}

// 私有辅助方法
Transform3D TransformRenderProxyComponentResource::cpp_transform_to_godot(const portal_core::TransformRenderProxy::TransformData &transform_data) const
{
    Transform3D godot_transform;

    // 转换位置
    if (sync_position)
    {
        godot_transform.origin = Vector3(
            transform_data.position.GetX(),
            transform_data.position.GetY(),
            transform_data.position.GetZ());
    }

    // 转换旋转
    if (sync_rotation)
    {
        Quaternion godot_quat(
            transform_data.rotation.GetX(),
            transform_data.rotation.GetY(),
            transform_data.rotation.GetZ(),
            transform_data.rotation.GetW());
        godot_transform.basis = Basis(godot_quat);
    }

    // 转换缩放
    if (sync_scale)
    {
        godot_transform.basis = godot_transform.basis.scaled(Vector3(
            transform_data.scale.GetX(),
            transform_data.scale.GetY(),
            transform_data.scale.GetZ()));
    }

    return godot_transform;
}

void TransformRenderProxyComponentResource::apply_transform_to_node3d(Node3D *node3d, const Transform3D &transform) const
{
    if (!node3d)
        return;

    // 根据同步设置应用变换
    if (sync_position)
    {
        node3d->set_position(transform.origin);
    }

    if (sync_rotation)
    {
        node3d->set_quaternion(transform.basis.get_rotation_quaternion());
    }

    if (sync_scale)
    {
        node3d->set_scale(transform.basis.get_scale());
    }
}

double TransformRenderProxyComponentResource::get_current_render_time() const
{
    // 使用GameCoreManager的统一时间基准
    return GameCoreManager::get_global_time();
}

void TransformRenderProxyComponentResource::log_debug_info(const String &message) const
{
    if (debug_mode)
    {
        UtilityFunctions::print("[TransformRenderProxy] " + message);
    }
}

Dictionary TransformRenderProxyComponentResource::auto_fill_from_node3d(Node *node)
{
    Dictionary result;
    result["success"] = false;
    result["message"] = "";

    Node3D *node3d = Object::cast_to<Node3D>(node);
    if (!node3d)
    {
        result["message"] = "Node is not a Node3D";
        return result;
    }

    // 从Node3D获取变换信息并设置默认值
    Transform3D transform = node3d->get_transform();

    // 根据变换数据设置同步选项
    sync_position = true;
    sync_rotation = true;
    sync_scale = true;

    // 如果节点有动画或者移动，启用插值
    interpolation_enabled = true;

    result["success"] = true;
    result["message"] = "Auto-filled from Node3D transform";
    return result;
}

// 注册组件资源
REGISTER_COMPONENT_RESOURCE(TransformRenderProxyComponentResource)