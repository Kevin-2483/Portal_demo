#pragma once

#include "core/debug/portal_build_config.h"
#include <godot_cpp/classes/node3d.hpp>
#include "../render/godot_unified_renderer.h"
#include <memory>

#ifdef PORTAL_DEBUG_GUI_ENABLED
#include "core/debug/debug_gui_system.h"
#endif

namespace portal_gdext {
namespace debug {

/**
 * Godot统一调试渲染桥接节点
 * 用于在Godot场景中集成统一渲染系统
 */
class UnifiedDebugRenderBridge : public godot::Node3D {
    GDCLASS(UnifiedDebugRenderBridge, godot::Node3D)
    
private:
    std::unique_ptr<render::GodotUnifiedRenderer> unified_renderer_;
    godot::Node3D* world_node_;
    godot::Control* ui_node_;
    bool auto_register_;
    bool initialized_;
    
#ifdef PORTAL_DEBUG_GUI_ENABLED
    bool debug_gui_initialized_;
    bool debug_gui_enabled_;
    float frame_accumulator_;
    int frame_count_;
#endif
    
public:
    UnifiedDebugRenderBridge();
    ~UnifiedDebugRenderBridge();
    
    // Godot方法
    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;
    
    static void _bind_methods();
    
    // 配置方法
    void set_world_node(godot::Node3D* world_node);
    godot::Node3D* get_world_node() const { return world_node_; }
    
    void set_ui_node(godot::Control* ui_node);
    godot::Control* get_ui_node() const { return ui_node_; }
    
    void set_auto_register(bool auto_register);
    bool get_auto_register() const { return auto_register_; }
    
    // 控制方法
    bool initialize_renderer();
    void shutdown_renderer();
    bool is_initialized() const { return initialized_; }
    
    // 重新初始化方法
    bool reinitialize_renderer();
    
    // 便利方法，供GDScript调用
    void clear_all_debug();
    void toggle_renderer(bool enabled);
    
#ifdef PORTAL_DEBUG_GUI_ENABLED
    // Debug GUI 控制方法
    bool initialize_debug_gui(const godot::String& font_resource_path = "");
    void shutdown_debug_gui();
    bool is_debug_gui_initialized() const { return debug_gui_initialized_; }
    
    void set_debug_gui_enabled(bool enabled);
    bool get_debug_gui_enabled() const { return debug_gui_enabled_; }
    
    // GUI窗口管理
    void show_all_gui_windows();
    void hide_all_gui_windows();
    void toggle_gui_window(const godot::String& window_id);
    void print_gui_stats();
    
    // 获取调试GUI系统实例
    portal_core::debug::DebugGUISystem* get_debug_gui_system();
    
private:
    // 资源路径解析辅助方法
    std::string resolve_resource_path(const godot::String& resource_path);
#endif
    
    // 获取渲染器（供其他C++代码使用）
    render::GodotUnifiedRenderer* get_unified_renderer() const { return unified_renderer_.get(); }
    
private:
    void register_with_manager();
    void unregister_from_manager();
};

}} // namespace portal_gdext::debug
