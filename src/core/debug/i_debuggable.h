#pragma once

#include "debug_config.h"

#ifdef PORTAL_DEBUG_ENABLED

#include <string>

namespace portal_core {
namespace debug {

/**
 * 可调试模块接口
 * 
 * 任何需要提供调试功能的系统都可以实现这个接口。
 * 实现此接口的类可以：
 * 1. 在ImGui中绘制自定义调试界面
 * 2. 在3D世界空间中绘制调试图形
 * 3. 被自动发现和管理
 * 
 * 设计原则：
 * - 按需实现：只有需要调试功能的系统才实现此接口
 * - 自主管理：各系统负责自己的调试内容
 * - 零依赖：不强制任何系统必须实现调试功能
 */
class IDebuggable {
public:
    virtual ~IDebuggable() = default;
    
    /**
     * 渲染GUI调试界面
     * 
     * 在此方法中使用ImGui API绘制调试界面。
     * 每帧调用一次（如果启用调试）。
     * 
     * 示例：
     * void render_debug_gui() override {
     *     if (ImGui::Begin("我的系统调试")) {
     *         ImGui::Text("状态: %s", get_status().c_str());
     *         ImGui::SliderFloat("参数", &my_param, 0.0f, 1.0f);
     *     }
     *     ImGui::End();
     * }
     */
    virtual void render_debug_gui() {}
    
    /**
     * 渲染世界空间调试图形
     * 
     * 在此方法中使用UnifiedDebugDraw API绘制3D调试图形。
     * 每帧调用一次（如果启用调试）。
     * 
     * 示例：
     * void render_debug_world() override {
     *     // 绘制碰撞盒
     *     UnifiedDebugDraw::draw_box(position, size, Color::GREEN);
     *     // 绘制速度向量
     *     UnifiedDebugDraw::draw_line(position, position + velocity, Color::RED);
     * }
     */
    virtual void render_debug_world() {}
    
    /**
     * 获取调试名称
     * 
     * 返回在调试界面中显示的名称。
     * 用于调试窗口标题、菜单项等。
     * 
     * @return 调试名称，应该是简短且唯一的标识符
     */
    virtual std::string get_debug_name() const = 0;
    
    /**
     * 检查是否启用调试
     * 
     * 返回此对象当前是否应该显示调试信息。
     * 可以用于运行时启用/禁用特定系统的调试。
     * 
     * @return true 如果应该显示调试信息
     */
    virtual bool is_debug_enabled() const { return true; }
    
    /**
     * 渲染调试菜单项
     * 
     * 在主调试菜单中显示的菜单项。
     * 可选实现，用于提供快速操作按钮。
     * 
     * 示例：
     * void render_debug_menu() override {
     *     if (ImGui::MenuItem("重置系统")) {
     *         reset();
     *     }
     *     if (ImGui::MenuItem("导出数据")) {
     *         export_debug_data();
     *     }
     * }
     */
    virtual void render_debug_menu() {}
};

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_ENABLED
