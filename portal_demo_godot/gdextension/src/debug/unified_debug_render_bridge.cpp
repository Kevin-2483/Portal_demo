#include "debug/unified_debug_render_bridge.h"
#include "core/debug/portal_build_config.h"
#include "core/render/unified_render_manager.h"
#include "core/render/unified_debug_draw.h"
#include "core/math_types.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

#ifdef PORTAL_DEBUG_GUI_ENABLED
#include "core/debug/debug_gui_system.h"
#include "core/debug/simple_text_window_manager.h"
#endif

using namespace godot;

namespace portal_gdext {
namespace debug {

void UnifiedDebugRenderBridge::_bind_methods() {
    // 属性绑定
    ClassDB::bind_method(D_METHOD("set_world_node", "world_node"), &UnifiedDebugRenderBridge::set_world_node);
    ClassDB::bind_method(D_METHOD("get_world_node"), &UnifiedDebugRenderBridge::get_world_node);
    
    ClassDB::bind_method(D_METHOD("set_ui_node", "ui_node"), &UnifiedDebugRenderBridge::set_ui_node);
    ClassDB::bind_method(D_METHOD("get_ui_node"), &UnifiedDebugRenderBridge::get_ui_node);
    
    ClassDB::bind_method(D_METHOD("set_auto_register", "auto_register"), &UnifiedDebugRenderBridge::set_auto_register);
    ClassDB::bind_method(D_METHOD("get_auto_register"), &UnifiedDebugRenderBridge::get_auto_register);
    
    // 控制方法
    ClassDB::bind_method(D_METHOD("initialize_renderer"), &UnifiedDebugRenderBridge::initialize_renderer);
    ClassDB::bind_method(D_METHOD("shutdown_renderer"), &UnifiedDebugRenderBridge::shutdown_renderer);
    ClassDB::bind_method(D_METHOD("is_initialized"), &UnifiedDebugRenderBridge::is_initialized);
    ClassDB::bind_method(D_METHOD("reinitialize_renderer"), &UnifiedDebugRenderBridge::reinitialize_renderer);
    
    // 便利方法
    ClassDB::bind_method(D_METHOD("clear_all_debug"), &UnifiedDebugRenderBridge::clear_all_debug);
    ClassDB::bind_method(D_METHOD("toggle_renderer", "enabled"), &UnifiedDebugRenderBridge::toggle_renderer);
    
#ifdef PORTAL_DEBUG_GUI_ENABLED
    // Debug GUI 控制方法绑定
    ClassDB::bind_method(D_METHOD("initialize_debug_gui"), &UnifiedDebugRenderBridge::initialize_debug_gui);
    ClassDB::bind_method(D_METHOD("shutdown_debug_gui"), &UnifiedDebugRenderBridge::shutdown_debug_gui);
    ClassDB::bind_method(D_METHOD("is_debug_gui_initialized"), &UnifiedDebugRenderBridge::is_debug_gui_initialized);
    
    ClassDB::bind_method(D_METHOD("set_debug_gui_enabled", "enabled"), &UnifiedDebugRenderBridge::set_debug_gui_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_gui_enabled"), &UnifiedDebugRenderBridge::get_debug_gui_enabled);
    
    // Debug GUI 便利方法绑定
    ClassDB::bind_method(D_METHOD("show_all_gui_windows"), &UnifiedDebugRenderBridge::show_all_gui_windows);
    ClassDB::bind_method(D_METHOD("hide_all_gui_windows"), &UnifiedDebugRenderBridge::hide_all_gui_windows);
    ClassDB::bind_method(D_METHOD("toggle_gui_window", "window_id"), &UnifiedDebugRenderBridge::toggle_gui_window);
    ClassDB::bind_method(D_METHOD("print_gui_stats"), &UnifiedDebugRenderBridge::print_gui_stats);
    
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_gui_enabled"), "set_debug_gui_enabled", "get_debug_gui_enabled");
#endif
    
    // 属性注册
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_node", PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_world_node", "get_world_node");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ui_node", PROPERTY_HINT_NODE_TYPE, "Control"), "set_ui_node", "get_ui_node");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_register"), "set_auto_register", "get_auto_register");
}

UnifiedDebugRenderBridge::UnifiedDebugRenderBridge() 
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

UnifiedDebugRenderBridge::~UnifiedDebugRenderBridge() {
#ifdef PORTAL_DEBUG_GUI_ENABLED
    shutdown_debug_gui();
#endif
    shutdown_renderer();
}

void UnifiedDebugRenderBridge::_ready() {
    set_process_mode(Node::PROCESS_MODE_ALWAYS);
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Node ready");
    
    // 如果没有设置世界节点，使用当前节点
    if (!world_node_) {
        world_node_ = this;
        UtilityFunctions::print("UnifiedDebugRenderBridge: Using self as world node");
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
                    UtilityFunctions::print("UnifiedDebugRenderBridge: Found UI node: ", control->get_name());
                    break;
                }
            }
        }
        
        // 如果还是没找到，创建一个
        if (!ui_node_) {
            UtilityFunctions::print("UnifiedDebugRenderBridge: No UI node found, will use world node");
        }
    }
    
    // 自动初始化
    if (auto_register_) {
        if (initialize_renderer()) {
            UtilityFunctions::print("UnifiedDebugRenderBridge: Auto-initialized successfully");
            
#ifdef PORTAL_DEBUG_GUI_ENABLED
            // 自动初始化 Debug GUI
            if (initialize_debug_gui()) {
                UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI auto-initialized successfully");
            } else {
                UtilityFunctions::printerr("UnifiedDebugRenderBridge: Debug GUI auto-initialization failed");
            }
#endif
        } else {
            UtilityFunctions::printerr("UnifiedDebugRenderBridge: Auto-initialization failed");
        }
    }
}

void UnifiedDebugRenderBridge::_process(double delta) {
    if (!initialized_ || !unified_renderer_) return;
    
    float delta_f = static_cast<float>(delta);
    
#ifdef PORTAL_DEBUG_GUI_ENABLED
    // 更新调试GUI系统
    if (debug_gui_initialized_ && debug_gui_enabled_) {
        frame_accumulator_ += delta_f;
        frame_count_++;
        
        auto& gui_system = portal_core::debug::DebugGUISystem::instance();
        gui_system.update(delta_f);
        gui_system.render();
        gui_system.flush_to_unified_renderer();
        
        // 每秒更新一次性能数据
        if (frame_accumulator_ >= 1.0f) {
            // 性能采样功能已移除，仅重置计数器
            frame_accumulator_ = 0.0f;
            frame_count_ = 0;
        }
    }
#endif
    
    // 更新渲染器
    unified_renderer_->update(delta_f);
    
    // 分发命令到渲染器（在清理过期命令之前）
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    render_manager.flush_commands();
    
    // 更新渲染管理器（清理过期命令）
    render_manager.update(delta_f);
    
    // 推进帧
    render_manager.advance_frame();
}

void UnifiedDebugRenderBridge::_exit_tree() {
#ifdef PORTAL_DEBUG_GUI_ENABLED
    shutdown_debug_gui();
#endif
    shutdown_renderer();
}

void UnifiedDebugRenderBridge::set_world_node(Node3D* world_node) {
    if (initialized_) {
        UtilityFunctions::print("UnifiedDebugRenderBridge: Changing world node requires reinitialization");
        shutdown_renderer();
        world_node_ = world_node;
        if (auto_register_) {
            initialize_renderer();
        }
    } else {
        world_node_ = world_node;
    }
}

void UnifiedDebugRenderBridge::set_ui_node(Control* ui_node) {
    if (initialized_) {
        UtilityFunctions::print("UnifiedDebugRenderBridge: Changing UI node requires reinitialization");
        shutdown_renderer();
        ui_node_ = ui_node;
        if (auto_register_) {
            initialize_renderer();
        }
    } else {
        ui_node_ = ui_node;
    }
}

void UnifiedDebugRenderBridge::set_auto_register(bool auto_register) {
    auto_register_ = auto_register;
}

bool UnifiedDebugRenderBridge::initialize_renderer() {
    if (initialized_) {
        UtilityFunctions::print("UnifiedDebugRenderBridge: Already initialized");
        return true;
    }
    
    if (!unified_renderer_) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: Unified renderer is null");
        return false;
    }
    
    if (!world_node_) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: World node is null");
        return false;
    }
    
    // 初始化统一渲染器
    if (!unified_renderer_->initialize(world_node_, ui_node_)) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: Failed to initialize unified renderer");
        return false;
    }
    
    // 注册到全局管理器
    register_with_manager();
    
    initialized_ = true;
    UtilityFunctions::print("UnifiedDebugRenderBridge: Initialization completed");
    
    return true;
}

void UnifiedDebugRenderBridge::shutdown_renderer() {
    if (!initialized_) return;
    
    // 从全局管理器注销
    unregister_from_manager();
    
    // 关闭渲染器
    if (unified_renderer_) {
        unified_renderer_->shutdown();
    }
    
    initialized_ = false;
    UtilityFunctions::print("UnifiedDebugRenderBridge: Shutdown completed");
}

bool UnifiedDebugRenderBridge::reinitialize_renderer() {
    if (initialized_) {
        shutdown_renderer();
    }
    return initialize_renderer();
}

void UnifiedDebugRenderBridge::clear_all_debug() {
    if (!initialized_) return;
    unified_renderer_->clear_commands();
}

void UnifiedDebugRenderBridge::toggle_renderer(bool enabled) {
    if (!initialized_) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: Not initialized, cannot toggle renderer");
        return;
    }
    
    unified_renderer_->set_enabled(enabled);
    portal_core::debug::UnifiedDebugDraw::set_enabled(enabled);
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Renderer ", enabled ? "enabled" : "disabled");
}

void UnifiedDebugRenderBridge::register_with_manager() {
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    render_manager.register_renderer(unified_renderer_.get());
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Registered with render manager");
}

void UnifiedDebugRenderBridge::unregister_from_manager() {
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    render_manager.unregister_renderer(unified_renderer_.get());
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Unregistered from render manager");
}

#ifdef PORTAL_DEBUG_GUI_ENABLED

bool UnifiedDebugRenderBridge::initialize_debug_gui() {
    if (debug_gui_initialized_) {
        UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI already initialized");
        return true;
    }
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Initializing debug GUI system...");
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    if (!gui_system.initialize()) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: Failed to initialize debug GUI system");
        return false;
    }
    
    // 初始化SimpleTextWindow管理器
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    if (!text_window_manager.initialize()) {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: Failed to initialize SimpleTextWindow manager");
        return false;
    }
    
    gui_system.set_enabled(debug_gui_enabled_);
    debug_gui_initialized_ = true;
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI system initialized successfully");
    return true;
}

void UnifiedDebugRenderBridge::shutdown_debug_gui() {
    if (!debug_gui_initialized_) return;
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Shutting down debug GUI system");
    
    // 关闭SimpleTextWindow管理器
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.shutdown();
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    gui_system.shutdown();
    
    debug_gui_initialized_ = false;
    UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI system shut down");
}

void UnifiedDebugRenderBridge::set_debug_gui_enabled(bool enabled) {
    debug_gui_enabled_ = enabled;
    
    if (debug_gui_initialized_) {
        auto& gui_system = portal_core::debug::DebugGUISystem::instance();
        gui_system.set_enabled(enabled);
        
        UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI ", enabled ? "enabled" : "disabled");
    }
}

void UnifiedDebugRenderBridge::show_all_gui_windows() {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    
    // 显示所有已注册的窗口
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.show_window(true);
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Show all GUI windows called - SimpleTextWindow shown");
}

void UnifiedDebugRenderBridge::hide_all_gui_windows() {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    
    // 隐藏所有已注册的窗口
    auto& text_window_manager = portal_core::debug::SimpleTextWindowManager::instance();
    text_window_manager.show_window(false);
    
    UtilityFunctions::print("UnifiedDebugRenderBridge: Hide all GUI windows called - SimpleTextWindow hidden");
}

void UnifiedDebugRenderBridge::toggle_gui_window(const godot::String& window_id) {
    if (!debug_gui_initialized_) return;
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    std::string id_str = window_id.utf8().get_data();
    
    auto* window = gui_system.find_window(id_str);
    if (window) {
        bool new_state = !window->is_visible();
        window->set_visible(new_state);
        UtilityFunctions::print("UnifiedDebugRenderBridge: GUI Window '", window_id, "' ", new_state ? "shown" : "hidden");
    } else {
        UtilityFunctions::printerr("UnifiedDebugRenderBridge: GUI Window '", window_id, "' not found");
    }
}

void UnifiedDebugRenderBridge::print_gui_stats() {
    if (!debug_gui_initialized_) {
        UtilityFunctions::print("UnifiedDebugRenderBridge: Debug GUI not initialized");
        return;
    }
    
    auto& gui_system = portal_core::debug::DebugGUISystem::instance();
    const auto& stats = gui_system.get_stats();
    
    UtilityFunctions::print("=== Debug GUI Statistics ===");
    UtilityFunctions::print("Windows: ", (int)stats.window_count, " (Visible: ", (int)stats.visible_window_count, ")");
    UtilityFunctions::print("Frame time: ", stats.frame_time_ms, "ms");
    UtilityFunctions::print("Render time: ", stats.render_time_ms, "ms");
}

portal_core::debug::DebugGUISystem* UnifiedDebugRenderBridge::get_debug_gui_system() {
    if (!debug_gui_initialized_) return nullptr;
    return &portal_core::debug::DebugGUISystem::instance();
}

#endif // PORTAL_DEBUG_GUI_ENABLED

}} // namespace portal_gdext::debug
