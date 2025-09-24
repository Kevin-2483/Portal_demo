#pragma once

#include "core/debug/debug_config.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "simple_text_window.h"
#include "debug_gui_system.h"
#include <memory>

namespace portal_core {
namespace debug {

/**
 * SimpleTextWindow管理器
 * 负责创建和注册SimpleTextWindow到DebugGUISystem
 */
class SimpleTextWindowManager {
public:
    // 单例访问
    static SimpleTextWindowManager& instance();
    
    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // 窗口管理
    void create_default_window();
    SimpleTextWindow* get_window(const std::string& window_id = "simple_text");
    
    // 便利方法
    void show_window(bool show = true);
    void add_text_line(const std::string& text);
    void set_window_text(const std::string& text);
    
private:
    SimpleTextWindowManager() = default;
    ~SimpleTextWindowManager() = default;
    SimpleTextWindowManager(const SimpleTextWindowManager&) = delete;
    SimpleTextWindowManager& operator=(const SimpleTextWindowManager&) = delete;
    
    bool initialized_ = false;
    SimpleTextWindow* text_window_ = nullptr;
};

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED