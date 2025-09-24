#pragma once

#include "core/debug/debug_config.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "debug_gui_system.h"
#include <string>

namespace portal_core
{
    namespace debug
    {

        /**
         * 简单的文字显示窗口
         * 用于显示基本文本信息的ImGui窗口
         */
        class SimpleTextWindow : public DebugWindow
        {
        public:
            SimpleTextWindow(const std::string &id = "simple_text",
                             const std::string &title = "简单文字窗口");

            // 实现DebugWindow接口
            void render() override;

            // 设置显示的文字内容
            void set_text(const std::string &text) { text_content_ = text; }
            const std::string &get_text() const { return text_content_; }

            // 添加文字行
            void add_line(const std::string &line);
            void clear_text();

            // 设置文字颜色
            void set_text_color(float r, float g, float b, float a = 1.0f);

        private:
            std::string text_content_;
            std::vector<std::string> text_lines_;
            ImVec4 text_color_;
            bool show_timestamp_;

            // 辅助方法
            std::string get_current_time();

        }; // namespace debug
    } // namespace portal_core
}
#endif // PORTAL_DEBUG_GUI_ENABLED