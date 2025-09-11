#pragma once

#include "unified_render_manager.h"

namespace portal_core {
namespace debug {

// 便利API类，简化常用调试绘制操作
class UnifiedDebugDraw {
public:
    // ========== 3D 世界空间绘制 API ==========
    
    // 基础3D绘制
    static void draw_line(const Vector3& start, const Vector3& end, const render::Color4f& color = render::Color4f::WHITE, float thickness = 1.0f);
    static void draw_box(const Vector3& center, const Vector3& size, const render::Color4f& color = render::Color4f::WHITE, bool filled = false);
    static void draw_sphere(const Vector3& center, float radius, const render::Color4f& color = render::Color4f::WHITE, int segments = 16, bool filled = false);
    
    // 便利3D绘制
    static void draw_cross(const Vector3& center, float size = 0.1f, const render::Color4f& color = render::Color4f::WHITE);
    static void draw_coordinate_axes(const Vector3& origin, float size = 1.0f);
    static void draw_arrow(const Vector3& start, const Vector3& end, const render::Color4f& color = render::Color4f::YELLOW, float head_size = 0.1f);
    static void draw_grid(const Vector3& center, const Vector3& size, int divisions_x = 10, int divisions_z = 10, const render::Color4f& color = render::Color4f::WHITE);
    
    // 物理调试
    static void draw_aabb(const Vector3& min, const Vector3& max, const render::Color4f& color = render::Color4f::GREEN);
    static void draw_obb(const Vector3& center, const Vector3& size, const Vector3& rotation, const render::Color4f& color = render::Color4f::BLUE);
    static void draw_ray(const Vector3& origin, const Vector3& direction, float length = 1.0f, const render::Color4f& color = render::Color4f::RED);
    
    // ========== 2D UI空间绘制 API ==========
    
    // 基础UI绘制
    static void draw_ui_rect(const Vector2& position, const Vector2& size, const render::Color4f& color = render::Color4f::WHITE, bool filled = true, float border_width = 1.0f);
    static void draw_ui_text(const Vector2& position, const std::string& text, const render::Color4f& color = render::Color4f::WHITE, float font_size = 14.0f, int align = 0);
    static void draw_ui_line(const Vector2& start, const Vector2& end, const render::Color4f& color = render::Color4f::WHITE, float thickness = 1.0f);
    
    // 便利UI绘制
    static void draw_ui_circle(const Vector2& center, float radius, const render::Color4f& color = render::Color4f::WHITE, int segments = 16, bool filled = true);
    static void draw_ui_cross(const Vector2& center, float size = 5.0f, const render::Color4f& color = render::Color4f::WHITE);
    
    // UI组件绘制
    static void draw_ui_window(const Vector2& position, const Vector2& size, const std::string& title = "", const render::Color4f& color = render::Color4f(0.2f, 0.2f, 0.2f, 0.9f));
    static void draw_ui_button(const Vector2& position, const Vector2& size, const std::string& label = "", bool pressed = false, const render::Color4f& color = render::Color4f(0.4f, 0.4f, 0.4f, 1.0f));
    static void draw_ui_slider(const Vector2& position, const Vector2& size, float value, float min_val = 0.0f, float max_val = 1.0f, const std::string& label = "");
    static void draw_ui_progress_bar(const Vector2& position, const Vector2& size, float progress, const render::Color4f& bg_color = render::Color4f(0.3f, 0.3f, 0.3f, 1.0f), const render::Color4f& fg_color = render::Color4f::GREEN);
    
    // 图表和数据可视化
    static void draw_ui_graph(const Vector2& position, const Vector2& size, const std::vector<float>& values, const render::Color4f& color = render::Color4f::CYAN, const std::string& title = "");
    static void draw_ui_histogram(const Vector2& position, const Vector2& size, const std::vector<float>& values, const render::Color4f& color = render::Color4f::BLUE, const std::string& title = "");
    
    // ========== 高级功能 ==========
    
    // 持续时间控制
    static void draw_line_timed(const Vector3& start, const Vector3& end, float duration, const render::Color4f& color = render::Color4f::WHITE, float thickness = 1.0f);
    static void draw_ui_text_timed(const Vector2& position, const std::string& text, float duration, const render::Color4f& color = render::Color4f::WHITE, float font_size = 14.0f);
    
    // 一次性绘制（仅当前帧）
    static void draw_line_once(const Vector3& start, const Vector3& end, const render::Color4f& color = render::Color4f::WHITE, float thickness = 1.0f);
    static void draw_ui_text_once(const Vector2& position, const std::string& text, const render::Color4f& color = render::Color4f::WHITE, float font_size = 14.0f);
    
    // 自定义命令提交
    template<typename T>
    static void submit_custom_command(const T& data, uint32_t custom_type, uint32_t layer = static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG), uint32_t flags = render::RENDER_FLAG_NONE);
    
    // ========== 系统控制 ==========
    
    // 清理命令
    static void clear_all();
    static void clear_3d();
    static void clear_ui();
    static void clear_layer(uint32_t layer);
    
    // 系统状态
    static void set_enabled(bool enabled);
    static bool is_enabled();
    
    // 统计信息
    static render::RenderStats get_stats();
    static void print_stats();
    
private:
    // 便利函数用于获取渲染管理器
    static render::UnifiedRenderManager& get_manager() { return render::UnifiedRenderManager::instance(); }
};

// 便利宏定义
#define UNIFIED_DEBUG_DRAW portal_core::debug::UnifiedDebugDraw

}} // namespace portal_core::debug
