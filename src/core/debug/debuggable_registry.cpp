#include "debuggable_registry.h"

#ifdef PORTAL_DEBUG_ENABLED

#include <algorithm>
#include <iostream>
#include "imgui.h"

namespace portal_core {
namespace debug {

DebuggableRegistry& DebuggableRegistry::instance() {
    static DebuggableRegistry instance;
    return instance;
}

void DebuggableRegistry::register_debuggable(IDebuggable* debuggable) {
    if (!debuggable) {
        std::cerr << "DebuggableRegistry: Attempted to register null debuggable object" << std::endl;
        return;
    }
    
    // 检查是否已经注册
    if (is_registered(debuggable)) {
        std::cerr << "DebuggableRegistry: Object '" << debuggable->get_debug_name() 
                  << "' is already registered" << std::endl;
        return;
    }
    
    std::string name = debuggable->get_debug_name();
    
    // 检查名称冲突
    auto it = named_objects_.find(name);
    if (it != named_objects_.end()) {
        std::cerr << "DebuggableRegistry: Debug name '" << name 
                  << "' is already in use by another object" << std::endl;
        return;
    }
    
    // 注册对象
    registered_objects_.push_back(debuggable);
    named_objects_[name] = debuggable;
    
    std::cout << "DebuggableRegistry: Registered debuggable object '" << name << "'" << std::endl;
}

void DebuggableRegistry::unregister_debuggable(IDebuggable* debuggable) {
    if (!debuggable) {
        return;
    }
    
    // 从向量中移除
    auto vec_it = std::find(registered_objects_.begin(), registered_objects_.end(), debuggable);
    if (vec_it != registered_objects_.end()) {
        registered_objects_.erase(vec_it);
        
        // 从名称映射中移除
        std::string name = debuggable->get_debug_name();
        auto map_it = named_objects_.find(name);
        if (map_it != named_objects_.end()) {
            named_objects_.erase(map_it);
        }
        
        std::cout << "DebuggableRegistry: Unregistered debuggable object '" << name << "'" << std::endl;
    }
}

void DebuggableRegistry::render_all_gui() {
    if (!debug_enabled_) {
        return;
    }
    
    for (auto* debuggable : registered_objects_) {
        if (debuggable && debuggable->is_debug_enabled()) {
            try {
                debuggable->render_debug_gui();
            } catch (const std::exception& e) {
                std::cerr << "DebuggableRegistry: Exception in render_debug_gui() for '" 
                          << debuggable->get_debug_name() << "': " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "DebuggableRegistry: Unknown exception in render_debug_gui() for '" 
                          << debuggable->get_debug_name() << "'" << std::endl;
            }
        }
    }
}

void DebuggableRegistry::render_all_world() {
    if (!debug_enabled_) {
        return;
    }
    
    for (auto* debuggable : registered_objects_) {
        if (debuggable && debuggable->is_debug_enabled()) {
            try {
                debuggable->render_debug_world();
            } catch (const std::exception& e) {
                std::cerr << "DebuggableRegistry: Exception in render_debug_world() for '" 
                          << debuggable->get_debug_name() << "': " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "DebuggableRegistry: Unknown exception in render_debug_world() for '" 
                          << debuggable->get_debug_name() << "'" << std::endl;
            }
        }
    }
}

void DebuggableRegistry::render_debuggable_list() {
    if (!ImGui::Begin("调试对象列表")) {
        ImGui::End();
        return;
    }
    
    ImGui::Text("已注册对象数量: %zu", registered_objects_.size());
    ImGui::Separator();
    
    if (registered_objects_.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "没有注册的调试对象");
        ImGui::Text("提示: 实现 IDebuggable 接口并调用 PORTAL_REGISTER_DEBUGGABLE");
    } else {
        ImGui::Columns(3, "debuggable_columns");
        ImGui::Text("名称");
        ImGui::NextColumn();
        ImGui::Text("状态");
        ImGui::NextColumn();
        ImGui::Text("操作");
        ImGui::NextColumn();
        ImGui::Separator();
        
        for (auto* debuggable : registered_objects_) {
            if (!debuggable) continue;
            
            std::string name = debuggable->get_debug_name();
            bool enabled = debuggable->is_debug_enabled();
            
            ImGui::Text("%s", name.c_str());
            ImGui::NextColumn();
            
            if (enabled) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "启用");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "禁用");
            }
            ImGui::NextColumn();
            
            // 菜单操作
            std::string popup_id = "menu_" + name;
            if (ImGui::SmallButton(("菜单##" + name).c_str())) {
                ImGui::OpenPopup(popup_id.c_str());
            }
            
            if (ImGui::BeginPopup(popup_id.c_str())) {
                ImGui::Text("调试对象: %s", name.c_str());
                ImGui::Separator();
                
                // 调用对象自定义菜单
                debuggable->render_debug_menu();
                
                ImGui::EndPopup();
            }
            
            ImGui::NextColumn();
        }
        
        ImGui::Columns(1);
    }
    
    ImGui::Separator();
    
    // 全局控制
    ImGui::Text("全局控制:");
    bool global_enabled = debug_enabled_;
    if (ImGui::Checkbox("启用所有调试对象", &global_enabled)) {
        set_debug_enabled(global_enabled);
    }
    
    ImGui::End();
}

IDebuggable* DebuggableRegistry::find_by_name(const std::string& name) {
    auto it = named_objects_.find(name);
    return (it != named_objects_.end()) ? it->second : nullptr;
}

bool DebuggableRegistry::is_registered(IDebuggable* debuggable) const {
    return std::find(registered_objects_.begin(), registered_objects_.end(), debuggable) 
           != registered_objects_.end();
}

void DebuggableRegistry::clear_all() {
    std::cout << "DebuggableRegistry: Clearing all " << registered_objects_.size() 
              << " registered objects" << std::endl;
    
    registered_objects_.clear();
    named_objects_.clear();
}

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_ENABLED
