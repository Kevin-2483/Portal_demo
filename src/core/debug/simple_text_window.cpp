#include "simple_text_window.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "imgui.h"
#include <ctime>
#include <sstream>

namespace portal_core {
namespace debug {

SimpleTextWindow::SimpleTextWindow(const std::string& id, const std::string& title)
    : DebugWindow(id, title)
    , text_content_("欢迎使用Portal Demo调试系统！\n这是一个简单的ImGui文字窗口。")
    , text_color_(1.0f, 1.0f, 1.0f, 1.0f) // 白色
    , show_timestamp_(false)
{
    // 设置默认窗口大小
    set_size(Vector2(400, 300));
    set_position(Vector2(50, 50));
    
    // 初始化一些示例文字行
    text_lines_.push_back("Portal Demo 调试窗口");
    text_lines_.push_back("当前时间: " + get_current_time());
    text_lines_.push_back("状态: 正常运行");
    text_lines_.push_back("ImGui版本: " + std::string(IMGUI_VERSION));
}

void SimpleTextWindow::render() {
    if (!should_render()) return;
    
    begin_window();
    
    // 显示主要文字内容
    ImGui::TextColored(text_color_, "%s", text_content_.c_str());
    
    ImGui::Separator();
    
    // 显示文字行列表
    if (!text_lines_.empty()) {
        ImGui::Text("信息列表:");
        
        // 创建一个滚动区域
        if (ImGui::BeginChild("TextLines", ImVec2(0, 150), true)) {
            for (size_t i = 0; i < text_lines_.size(); ++i) {
                ImGui::Text("[%zu] %s", i + 1, text_lines_[i].c_str());
            }
        }
        ImGui::EndChild();
    }
    
    ImGui::Separator();
    
    // 控制按钮
    if (ImGui::Button("添加时间戳")) {
        add_line("时间戳: " + get_current_time());
    }
    
    ImGui::SameLine();
    if (ImGui::Button("清空文字")) {
        clear_text();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("重置内容")) {
        text_content_ = "内容已重置！\n当前时间: " + get_current_time();
    }
    
    // 颜色选择器
    ImGui::Separator();
    ImGui::Text("文字颜色:");
    ImGui::ColorEdit4("##TextColor", (float*)&text_color_);
    
    // 显示窗口信息
    ImGui::Separator();
    ImGui::Text("窗口信息:");
    ImGui::Text("ID: %s", get_id().c_str());
    ImGui::Text("标题: %s", get_title().c_str());
    ImGui::Text("可见: %s", is_visible() ? "是" : "否");
    ImGui::Text("文字行数: %zu", text_lines_.size());
    
    end_window();
}

void SimpleTextWindow::add_line(const std::string& line) {
    text_lines_.push_back(line);
    
    // 限制最大行数，避免内存无限增长
    const size_t max_lines = 100;
    if (text_lines_.size() > max_lines) {
        text_lines_.erase(text_lines_.begin());
    }
}

void SimpleTextWindow::clear_text() {
    text_lines_.clear();
    text_content_ = "文字已清空。";
}

void SimpleTextWindow::set_text_color(float r, float g, float b, float a) {
    text_color_ = ImVec4(r, g, b, a);
}

std::string SimpleTextWindow::get_current_time() {
    std::time_t now = std::time(nullptr);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buffer);
}

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED