#include "render_bridge/unified_render_bridge.h"
#include "core/debug/portal_build_config.h"
#include "core/render/unified_render_manager.h"
#include "core/render/unified_render_draw.h"
#include "core/math_types.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#ifdef PORTAL_DEBUG_GUI_ENABLED
#include "core/debug/debug_gui_system.h"
#include "core/debug/simple_text_window_manager.h"
#endif

using namespace godot;

namespace portal_gdext {
namespace render {

void UnifiedRenderBridge::_bind_methods() {
    // 属性绑定
    ClassDB::bind_method(D_METHOD("set_world_node", "world_node"), &UnifiedRenderBridge::set_world_node);
    ClassDB::bind_method(D_METHOD("get_world_node"), &UnifiedRenderBridge::get_world_node);
    
    ClassDB::bind_method(D_METHOD("set_ui_node", "ui_node"), &UnifiedRenderBridge::set_ui_node);
    ClassDB::bind_method(D_METHOD("get_ui_node"), &UnifiedRenderBridge::get_ui_node);
    
    ClassDB::bind_method(D_METHOD("set_auto_register", "auto_register"), &UnifiedRenderBridge::set_auto_register);
    ClassDB::bind_method(D_METHOD("get_auto_register"), &UnifiedRenderBridge::get_auto_register);
    
    // 渲染器管理方法
    ClassDB::bind_method(D_METHOD("initialize_renderer"), &UnifiedRenderBridge::initialize_renderer);
    ClassDB::bind_method(D_METHOD("shutdown_renderer"), &UnifiedRenderBridge::shutdown_renderer);
    ClassDB::bind_method(D_METHOD("is_initialized"), &UnifiedRenderBridge::is_initialized);
    ClassDB::bind_method(D_METHOD("reinitialize_renderer"), &UnifiedRenderBridge::reinitialize_renderer);
    
    // 调试控制方法
    ClassDB::bind_method(D_METHOD("clear_all_debug"), &UnifiedRenderBridge::clear_all_debug);
    ClassDB::bind_method(D_METHOD("toggle_renderer", "enabled"), &UnifiedRenderBridge::toggle_renderer);
    
#ifdef PORTAL_DEBUG_GUI_ENABLED
    // Debug GUI 方法
    ClassDB::bind_method(D_METHOD("initialize_debug_gui", "font_resource_path"), &UnifiedRenderBridge::initialize_debug_gui, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("shutdown_debug_gui"), &UnifiedRenderBridge::shutdown_debug_gui);
    ClassDB::bind_method(D_METHOD("is_debug_gui_initialized"), &UnifiedRenderBridge::is_debug_gui_initialized);
    
    ClassDB::bind_method(D_METHOD("set_debug_gui_enabled", "enabled"), &UnifiedRenderBridge::set_debug_gui_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_gui_enabled"), &UnifiedRenderBridge::get_debug_gui_enabled);
    
    // GUI 窗口控制方法
    ClassDB::bind_method(D_METHOD("show_all_gui_windows"), &UnifiedRenderBridge::show_all_gui_windows);
    ClassDB::bind_method(D_METHOD("hide_all_gui_windows"), &UnifiedRenderBridge::hide_all_gui_windows);
    ClassDB::bind_method(D_METHOD("toggle_gui_window", "window_id"), &UnifiedRenderBridge::toggle_gui_window);
    ClassDB::bind_method(D_METHOD("print_gui_stats"), &UnifiedRenderBridge::print_gui_stats);
    
    // 属性导出
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_gui_enabled"), "set_debug_gui_enabled", "get_debug_gui_enabled");
#endif
    
    // 属性导出
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_node", PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_world_node", "get_world_node");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ui_node", PROPERTY_HINT_NODE_TYPE, "Control"), "set_ui_node", "get_ui_node");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_register"), "set_auto_register", "get_auto_register");
}

// 构造函数和析构函数
UnifiedRenderBridge::UnifiedRenderBridge() 
    : world_node_(nullptr)
    , ui_node_(nullptr)
    , auto_register_(true)
    , initialized_(false) {
    
    unified_renderer_ = std::make_unique<render::GodotUnifiedRenderer>();

#ifdef PORTAL_DEBUG_GUI_ENABLED
    debug_gui_initialized_ = false;
    debug_gui_enabled_ = true;
    frame_accumulator_ = 0.0f;
    frame_count_ = 0;
#endif
}

UnifiedRenderBridge::~UnifiedRenderBridge() {
#ifdef PORTAL_DEBUG_GUI_ENABLED
    shutdown_debug_gui();
#endif
    shutdown_renderer();
}

void UnifiedRenderBridge::_ready() {
    set_process_mode(Node::PROCESS_MODE_ALWAYS);
    
    UtilityFunctions::print("UnifiedRenderBridge: Node ready");
    
    // 如果没有设置世界节点，使用当前节点
    if (!world_node_) {
        world_node_ = this;
        UtilityFunctions::print("UnifiedRenderBridge: Using self as world node");
    }
    
    // 如果没有设置UI节点，尝试找到合适的UI容器
    if (!ui_node_) {
        // 尝试获取当前场景的根节点或视口
        Node* scene_root = get_tree()->get_current_scene();
        if (scene_root) {
            // 查找Canvas Layer或Control节点
            for (int i = 0; i < scene_root->get_child_count(); ++i) {
                Node* child = scene_root->get_child(i);
                Control* control = Object::cast_to<Control>(child);
                if (control) {
                    ui_node_ = control;
                    UtilityFunctions::print("UnifiedRenderBridge: Found UI node: ", control->get_name());
                    break;
                }
            }
        }
        
        // 如果还是没找到，创建一个
        if (!ui_node_) {
            UtilityFunctions::print("UnifiedRenderBridge: No UI node found, will use world node");
        }
    }
    
    // 自动初始化
    if (auto_register_) {
        if (initialize_renderer()) {
            UtilityFunctions::print("UnifiedRenderBridge: Auto-initialized successfully");
            
#ifdef PORTAL_DEBUG_GUI_ENABLED
            // 自动初始化 Debug GUI
            if (initialize_debug_gui()) {
                UtilityFunctions::print("UnifiedRenderBridge: Debug GUI auto-initialized successfully");
            } else {
                UtilityFunctions::printerr("UnifiedRenderBridge: Debug GUI auto-initialization failed");
            }
#endif
        } else {
            UtilityFunctions::printerr("UnifiedRenderBridge: Auto-initialization failed");
        }
    }
}

void UnifiedRenderBridge::_process(double delta) {
  if (!initialized_ || !unified_renderer_) return;
  
  float delta_f = static_cast<float>(delta);
  auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
  
  // 1. (清理) 首先，更新渲染管理器，这将清理掉上一帧或更早的、已经渲染完毕的指令。
  // 这是最关键的改动：将 update 调用从末尾移到开头。
  render_manager.update(delta_f);
  
  // 2. (推进) 将帧计数器推进到当前帧。
  render_manager.advance_frame();
  
#ifdef PORTAL_DEBUG_GUI_ENABLED
  // 3. (提交) 现在为当前帧提交新的GUI渲染指令。
  if (debug_gui_initialized_ && debug_gui_enabled_) {
      frame_accumulator_ += delta_f;
      frame_count_++;
      
      auto& gui_system = portal_core::debug::DebugGUISystem::instance();
      gui_system.update(delta_f);
      gui_system.render();
      gui_system.flush_to_unified_renderer(); // 在这里提交新指令
      
      if (frame_accumulator_ >= 1.0f) {
          frame_accumulator_ = 0.0f;
          frame_count_ = 0;
      }
  }
#endif
  
  // 4. (更新Godot侧) 更新 Godot 渲染器本身的状态。
  unified_renderer_->update(delta_f);
  
  // 5. (分发) 将当前帧所有待处理的指令（包括刚刚提交的GUI指令）分发出去，准备渲染。
  render_manager.flush_commands();
}


void UnifiedRenderBridge::_exit_tree() {
#ifdef PORTAL_DEBUG_GUI_ENABLED
    shutdown_debug_gui();
#endif
    shutdown_renderer();
}

void UnifiedRenderBridge::set_world_node(Node3D* world_node) {
    if (initialized_) {
        UtilityFunctions::print("UnifiedRenderBridge: Changing world node requires reinitialization");
        shutdown_renderer();
        world_node_ = world_node;
        if (auto_register_) {
            initialize_renderer();
        }
    } else {
        world_node_ = world_node;
    }
}

void UnifiedRenderBridge::set_ui_node(Control* ui_node) {
    if (initialized_) {
        UtilityFunctions::print("UnifiedRenderBridge: Changing UI node requires reinitialization");
        shutdown_renderer();
        ui_node_ = ui_node;
        if (auto_register_) {
            initialize_renderer();
        }
    } else {
        ui_node_ = ui_node;
    }
}

void UnifiedRenderBridge::set_auto_register(bool auto_register) {
    auto_register_ = auto_register;
}

bool UnifiedRenderBridge::initialize_renderer() {
    if (initialized_) {
        UtilityFunctions::print("UnifiedRenderBridge: Already initialized");
        return true;
    }
    
    if (!unified_renderer_) {
        UtilityFunctions::printerr("UnifiedRenderBridge: Unified renderer is null");
        return false;
    }
    
    if (!world_node_) {
        UtilityFunctions::printerr("UnifiedRenderBridge: World node is null");
        return false;
    }
    
    // 初始化统一渲染器
    if (!unified_renderer_->initialize(world_node_, ui_node_)) {
        UtilityFunctions::printerr("UnifiedRenderBridge: Failed to initialize unified renderer");
        return false;
    }
    
    // 注册到全局管理器
    register_with_manager();
    
    initialized_ = true;
    UtilityFunctions::print("UnifiedRenderBridge: Initialization completed");
    
    return true;
}

void UnifiedRenderBridge::shutdown_renderer() {
    if (!initialized_) return;
    
    // 从全局管理器注销
    unregister_from_manager();
    
    // 关闭渲染器
    if (unified_renderer_) {
        unified_renderer_->shutdown();
    }
    
    initialized_ = false;
    UtilityFunctions::print("UnifiedRenderBridge: Shutdown completed");
}

bool UnifiedRenderBridge::reinitialize_renderer() {
    if (initialized_) {
        shutdown_renderer();
    }
    return initialize_renderer();
}

void UnifiedRenderBridge::clear_all_debug() {
    if (!initialized_) return;
    unified_renderer_->clear_commands();
}

void UnifiedRenderBridge::toggle_renderer(bool enabled) {
    if (!initialized_) {
        UtilityFunctions::printerr("UnifiedRenderBridge: Not initialized, cannot toggle renderer");
        return;
    }
    
    unified_renderer_->set_enabled(enabled);
    portal_core::render::UnifiedRenderDraw::set_enabled(enabled);
    
    UtilityFunctions::print("UnifiedRenderBridge: Renderer ", enabled ? "enabled" : "disabled");
}

void UnifiedRenderBridge::register_with_manager() {
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    render_manager.register_renderer(unified_renderer_.get());
    
    UtilityFunctions::print("UnifiedRenderBridge: Registered with render manager");
}

void UnifiedRenderBridge::unregister_from_manager() {
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    render_manager.unregister_renderer(unified_renderer_.get());
    
    UtilityFunctions::print("UnifiedRenderBridge: Unregistered from render manager");
}

#ifdef PORTAL_DEBUG_GUI_ENABLED

bool UnifiedRenderBridge::initialize_debug_gui(const godot::String& font_resource_path) {
    if (debug_gui_initialized_) {
        UtilityFunctions::print("UnifiedRenderBridge: Debug GUI already initialized");
        return true;
    }
    
    UtilityFunctions::print("UnifiedRenderBridge: Initializing debug GUI system...");
    
    // 解析字体资源路径
    std::string absolute_font_path = resolve_resource_path(font_resource_path);
    
    // 如果提供了字体路径但解析失败，记录警告
    if (!font_resource_path.is_empty() && absolute_font_path.empty()) {
        UtilityFunctions::print("Warning: Failed to resolve font resource path: ", font_resource_path);
    }
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    if (!gui_system.initialize(absolute_font_path)) {
        UtilityFunctions::printerr("UnifiedRenderBridge: Failed to initialize debug GUI system");
        return false;
    }
    
    // 初始化SimpleTextWindow管理器
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    if (!text_window_manager.initialize()) {
        UtilityFunctions::printerr("UnifiedRenderBridge: Failed to initialize SimpleTextWindow manager");
        return false;
    }
    
    gui_system.set_enabled(debug_gui_enabled_);
    debug_gui_initialized_ = true;
    
    UtilityFunctions::print("UnifiedRenderBridge: Debug GUI system initialized successfully");
    return true;
}

void UnifiedRenderBridge::shutdown_debug_gui() {
    if (!debug_gui_initialized_) return;
    
    UtilityFunctions::print("UnifiedRenderBridge: Shutting down debug GUI system");
    
    // 关闭SimpleTextWindow管理器
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.shutdown();
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    gui_system.shutdown();
    
    debug_gui_initialized_ = false;
    UtilityFunctions::print("UnifiedRenderBridge: Debug GUI system shut down");
}

void UnifiedRenderBridge::set_debug_gui_enabled(bool enabled) {
    debug_gui_enabled_ = enabled;
    
    if (debug_gui_initialized_) {
        auto& gui_system = portal_core::debug::DebugGUISystem::instance();
        gui_system.set_enabled(enabled);
        
        UtilityFunctions::print("UnifiedRenderBridge: Debug GUI ", enabled ? "enabled" : "disabled");
    }
}

void UnifiedRenderBridge::show_all_gui_windows() {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    
    // 显示所有已注册的窗口
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.show_window(true);
    
    UtilityFunctions::print("UnifiedRenderBridge: Show all GUI windows called - SimpleTextWindow shown");
}

void UnifiedRenderBridge::hide_all_gui_windows() {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    
    // 隐藏所有已注册的窗口
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.show_window(false);
    
    UtilityFunctions::print("UnifiedRenderBridge: Hide all GUI windows called - SimpleTextWindow hidden");
}

void UnifiedRenderBridge::toggle_gui_window(const godot::String& window_id) {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    std::string id_str = window_id.utf8().get_data();
    
    auto* window = gui_system.find_window(id_str);
    if (window) {
        bool new_state = !window->is_visible();
        window->set_visible(new_state);
        UtilityFunctions::print("UnifiedRenderBridge: GUI Window '", window_id, "' ", new_state ? "shown" : "hidden");
    } else {
        UtilityFunctions::printerr("UnifiedRenderBridge: GUI Window '", window_id, "' not found");
    }
}

void UnifiedRenderBridge::print_gui_stats() {
    if (!debug_gui_initialized_) {
        UtilityFunctions::print("UnifiedRenderBridge: Debug GUI not initialized");
        return;
    }
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    const auto& stats = gui_system.get_stats();
    
    UtilityFunctions::print("=== Debug GUI Statistics ===");
    UtilityFunctions::print("Windows: ", (int)stats.window_count, " (Visible: ", (int)stats.visible_window_count, ")");
    UtilityFunctions::print("Frame time: ", stats.frame_time_ms, "ms");
    UtilityFunctions::print("Render time: ", stats.render_time_ms, "ms");
}

portal_core::debug::DebugGUISystem* UnifiedRenderBridge::get_debug_gui_system() {
    if (!debug_gui_initialized_) return nullptr;
    return &portal_core::debug::DebugGUISystem::instance();
}

std::string UnifiedRenderBridge::resolve_resource_path(const godot::String& resource_path) {
    if (resource_path.is_empty()) {
        return "";
    }
    
    // 检查是否为Godot资源路径
    if (resource_path.begins_with("res://")) {
        // 使用ProjectSettings.globalize_path转换为绝对路径
        godot::ProjectSettings* project_settings = godot::ProjectSettings::get_singleton();
        if (project_settings) {
            godot::String absolute_path = project_settings->globalize_path(resource_path);
            return std::string(absolute_path.utf8().get_data());
        }
    }
    
    // 如果不是res://路径，直接返回原路径
    return std::string(resource_path.utf8().get_data());
}

#endif // PORTAL_DEBUG_GUI_ENABLED

}} // namespace portal_gdext::render
