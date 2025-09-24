#include "simple_text_window_manager.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "debug_gui_system.h"
#include <iostream>
#include <ctime>

namespace portal_core {
namespace debug {

// 辅助函数获取当前时间字符串
std::string get_current_time_string() {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);
    return std::string(buffer);
}

SimpleTextWindowManager& SimpleTextWindowManager::instance() {
    static SimpleTextWindowManager instance;
    return instance;
}

bool SimpleTextWindowManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    // 检查DebugGUISystem是否可用
    DebugGUISystem* debug_gui = &DebugGUISystem::instance();
    if (!debug_gui) {
        std::cerr << "SimpleTextWindowManager: DebugGUISystem not available" << std::endl;
        return false;
    }
    
    // 创建默认窗口
    create_default_window();
    
    initialized_ = true;
    std::cout << "SimpleTextWindowManager initialized successfully" << std::endl;
    return true;
}

void SimpleTextWindowManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    text_window_ = nullptr; // DebugGUISystem会负责清理
    initialized_ = false;
    std::cout << "SimpleTextWindowManager shutdown" << std::endl;
}

void SimpleTextWindowManager::create_default_window() {
    DebugGUISystem* debug_gui = &DebugGUISystem::instance();
    if (!debug_gui) {
        return;
    }
    
    // 创建SimpleTextWindow
    auto window = std::make_unique<SimpleTextWindow>("simple_text", "Simple Text Window");
    text_window_ = window.get();
    
    // 设置默认内容
    text_window_->set_text("欢迎使用Portal引擎调试界面！\n这是一个简单的文本显示窗口。");
    text_window_->add_line("当前时间: " + get_current_time_string());
    text_window_->set_text_color(1.0f, 1.0f, 1.0f, 1.0f); // 白色文本
    
    // 设置窗口属性
    text_window_->set_visible(true);
    text_window_->set_position(portal_core::make_vector2(50.0f, 50.0f));
    text_window_->set_size(portal_core::make_vector2(400.0f, 300.0f));
    
    // 注册到DebugGUISystem
    debug_gui->register_window(std::move(window));
    
    std::cout << "SimpleTextWindow created and registered" << std::endl;
}

SimpleTextWindow* SimpleTextWindowManager::get_window(const std::string& window_id) {
    if (window_id == "simple_text") {
        return text_window_;
    }
    return nullptr;
}

void SimpleTextWindowManager::show_window(bool show) {
    if (text_window_) {
        text_window_->set_visible(show);
    }
}

void SimpleTextWindowManager::add_text_line(const std::string& text) {
    if (text_window_) {
        text_window_->add_line(text);
    }
}

void SimpleTextWindowManager::set_window_text(const std::string& text) {
    if (text_window_) {
        text_window_->set_text(text);
    }
}

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED