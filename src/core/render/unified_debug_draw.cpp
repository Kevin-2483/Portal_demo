#include "unified_debug_draw.h"
#include <cmath>

namespace portal_core {
namespace debug {

// ========== 3D 世界空间绘制实现 ==========

void UnifiedDebugDraw::draw_line(const Vector3& start, const Vector3& end, const render::Color4f& color, float thickness) {
    render::Line3DData data(start, end, color, thickness);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_LINE_3D, data, 
                                   static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_box(const Vector3& center, const Vector3& size, const render::Color4f& color, bool filled) {
    render::Box3DData data(center, size, color, filled);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_BOX_3D, data,
                                   static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_sphere(const Vector3& center, float radius, const render::Color4f& color, int segments, bool filled) {
    render::Sphere3DData data(center, radius, color, segments, filled);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_SPHERE_3D, data,
                                   static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_cross(const Vector3& center, float size, const render::Color4f& color) {
    float half_size = size * 0.5f;
    
    // X轴
    draw_line(Vector3(center.GetX() - half_size, center.GetY(), center.GetZ()),
              Vector3(center.GetX() + half_size, center.GetY(), center.GetZ()), 
              render::Color4f::RED);
    
    // Y轴
    draw_line(Vector3(center.GetX(), center.GetY() - half_size, center.GetZ()),
              Vector3(center.GetX(), center.GetY() + half_size, center.GetZ()), 
              render::Color4f::GREEN);
    
    // Z轴
    draw_line(Vector3(center.GetX(), center.GetY(), center.GetZ() - half_size),
              Vector3(center.GetX(), center.GetY(), center.GetZ() + half_size), 
              render::Color4f::BLUE);
}

void UnifiedDebugDraw::draw_coordinate_axes(const Vector3& origin, float size) {
    // X轴 - 红色
    draw_line(origin, Vector3(origin.GetX() + size, origin.GetY(), origin.GetZ()), render::Color4f::RED, 2.0f);
    
    // Y轴 - 绿色
    draw_line(origin, Vector3(origin.GetX(), origin.GetY() + size, origin.GetZ()), render::Color4f::GREEN, 2.0f);
    
    // Z轴 - 蓝色
    draw_line(origin, Vector3(origin.GetX(), origin.GetY(), origin.GetZ() + size), render::Color4f::BLUE, 2.0f);
}

void UnifiedDebugDraw::draw_arrow(const Vector3& start, const Vector3& end, const render::Color4f& color, float head_size) {
    // 主线
    draw_line(start, end, color, 2.0f);
    
    // 计算方向
    Vector3 direction = end - start;
    float length = direction.Length();
    if (length < 0.001f) return;
    
    direction = direction.Normalized();
    
    // 箭头头部
    Vector3 perpendicular1, perpendicular2;
    if (std::abs(direction.GetY()) < 0.9f) {
        perpendicular1 = Vector3(0, 1, 0);
    } else {
        perpendicular1 = Vector3(1, 0, 0);
    }
    
    // 叉积计算垂直向量
    perpendicular2 = direction.Cross(perpendicular1);
    perpendicular1 = direction.Cross(perpendicular2);
    
    Vector3 head_back = end - direction * head_size;
    
    // 绘制箭头头部
    draw_line(end, head_back + perpendicular1 * head_size * 0.5f, color);
    draw_line(end, head_back - perpendicular1 * head_size * 0.5f, color);
    draw_line(end, head_back + perpendicular2 * head_size * 0.5f, color);
    draw_line(end, head_back - perpendicular2 * head_size * 0.5f, color);
}

void UnifiedDebugDraw::draw_grid(const Vector3& center, const Vector3& size, int divisions_x, int divisions_z, const render::Color4f& color) {
    float step_x = size.GetX() / divisions_x;
    float step_z = size.GetZ() / divisions_z;
    float half_x = size.GetX() * 0.5f;
    float half_z = size.GetZ() * 0.5f;
    
    // X方向的线
    for (int i = 0; i <= divisions_x; ++i) {
        float x = center.GetX() - half_x + i * step_x;
        draw_line(Vector3(x, center.GetY(), center.GetZ() - half_z),
                  Vector3(x, center.GetY(), center.GetZ() + half_z), color);
    }
    
    // Z方向的线
    for (int i = 0; i <= divisions_z; ++i) {
        float z = center.GetZ() - half_z + i * step_z;
        draw_line(Vector3(center.GetX() - half_x, center.GetY(), z),
                  Vector3(center.GetX() + half_x, center.GetY(), z), color);
    }
}

void UnifiedDebugDraw::draw_aabb(const Vector3& min, const Vector3& max, const render::Color4f& color) {
    Vector3 center = (min + max) * 0.5f;
    Vector3 size = max - min;
    draw_box(center, size, color, false);
}

void UnifiedDebugDraw::draw_obb(const Vector3& center, const Vector3& size, const Vector3& rotation, const render::Color4f& color) {
    // 简化版本，暂时忽略旋转，直接绘制AABB
    // 在完整实现中需要应用旋转变换
    draw_box(center, size, color, false);
}

void UnifiedDebugDraw::draw_ray(const Vector3& origin, const Vector3& direction, float length, const render::Color4f& color) {
    Vector3 end = origin + direction * length;
    draw_arrow(origin, end, color, length * 0.1f);
}

// ========== 2D UI空间绘制实现 ==========

void UnifiedDebugDraw::draw_ui_rect(const Vector2& position, const Vector2& size, const render::Color4f& color, bool filled, float border_width) {
    render::UIRectData data(position, size, color, filled, border_width);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_UI_RECT, data,
                                   static_cast<uint32_t>(render::RenderLayer::UI_CONTENT));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_ui_text(const Vector2& position, const std::string& text, const render::Color4f& color, float font_size, int align) {
    render::UITextData data(position, text, color, font_size, align);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_UI_TEXT, data,
                                   static_cast<uint32_t>(render::RenderLayer::UI_CONTENT));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_ui_line(const Vector2& start, const Vector2& end, const render::Color4f& color, float thickness) {
    render::UILineData data(start, end, color, thickness);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_UI_LINE, data,
                                   static_cast<uint32_t>(render::RenderLayer::UI_CONTENT));
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_ui_circle(const Vector2& center, float radius, const render::Color4f& color, int segments, bool filled) {
    // 用线段近似绘制圆形
    float angle_step = 2.0f * 3.14159f / segments;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angle_step;
        float angle2 = (i + 1) * angle_step;
        
        Vector2 p1(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius);
        Vector2 p2(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius);
        
        draw_ui_line(p1, p2, color, 1.0f);
        
        if (filled) {
            draw_ui_line(center, p1, color, 1.0f);
        }
    }
}

void UnifiedDebugDraw::draw_ui_cross(const Vector2& center, float size, const render::Color4f& color) {
    float half_size = size * 0.5f;
    draw_ui_line(Vector2(center.x - half_size, center.y), Vector2(center.x + half_size, center.y), color);
    draw_ui_line(Vector2(center.x, center.y - half_size), Vector2(center.x, center.y + half_size), color);
}

void UnifiedDebugDraw::draw_ui_window(const Vector2& position, const Vector2& size, const std::string& title, const render::Color4f& color) {
    // 窗口背景
    draw_ui_rect(position, size, color, true);
    
    // 窗口边框
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 标题栏
    if (!title.empty()) {
        Vector2 title_pos(position.x + 5, position.y + 5);
        draw_ui_text(title_pos, title, render::Color4f::WHITE, 12.0f);
        
        // 标题栏分隔线
        Vector2 line_start(position.x, position.y + 20);
        Vector2 line_end(position.x + size.x, position.y + 20);
        draw_ui_line(line_start, line_end, render::Color4f::WHITE);
    }
}

void UnifiedDebugDraw::draw_ui_button(const Vector2& position, const Vector2& size, const std::string& label, bool pressed, const render::Color4f& color) {
    render::Color4f button_color = pressed ? 
        render::Color4f(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, color.a) : color;
    
    // 按钮背景
    draw_ui_rect(position, size, button_color, true);
    
    // 按钮边框
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 按钮文字
    if (!label.empty()) {
        Vector2 text_pos(position.x + size.x * 0.5f, position.y + size.y * 0.5f);
        draw_ui_text(text_pos, label, render::Color4f::WHITE, 12.0f, 1); // 居中对齐
    }
}

void UnifiedDebugDraw::draw_ui_slider(const Vector2& position, const Vector2& size, float value, float min_val, float max_val, const std::string& label) {
    // 滑动条背景
    draw_ui_rect(position, size, render::Color4f(0.3f, 0.3f, 0.3f, 1.0f), true);
    
    // 滑动条边框
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 滑动条填充
    float normalized_value = (value - min_val) / (max_val - min_val);
    normalized_value = std::max(0.0f, std::min(1.0f, normalized_value));
    
    Vector2 fill_size(size.x * normalized_value, size.y);
    draw_ui_rect(position, fill_size, render::Color4f::BLUE, true);
    
    // 标签
    if (!label.empty()) {
        Vector2 label_pos(position.x, position.y - 15);
        draw_ui_text(label_pos, label + ": " + std::to_string(value), render::Color4f::WHITE, 10.0f);
    }
}

void UnifiedDebugDraw::draw_ui_progress_bar(const Vector2& position, const Vector2& size, float progress, const render::Color4f& bg_color, const render::Color4f& fg_color) {
    progress = std::max(0.0f, std::min(1.0f, progress));
    
    // 背景
    draw_ui_rect(position, size, bg_color, true);
    
    // 进度填充
    Vector2 fill_size(size.x * progress, size.y);
    draw_ui_rect(position, fill_size, fg_color, true);
    
    // 边框
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 进度文字
    std::string progress_text = std::to_string(static_cast<int>(progress * 100)) + "%";
    Vector2 text_pos(position.x + size.x * 0.5f, position.y + size.y * 0.5f);
    draw_ui_text(text_pos, progress_text, render::Color4f::WHITE, 10.0f, 1);
}

void UnifiedDebugDraw::draw_ui_graph(const Vector2& position, const Vector2& size, const std::vector<float>& values, const render::Color4f& color, const std::string& title) {
    if (values.empty()) return;
    
    // 图表背景
    draw_ui_rect(position, size, render::Color4f(0.1f, 0.1f, 0.1f, 0.8f), true);
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 标题
    if (!title.empty()) {
        Vector2 title_pos(position.x + 5, position.y + 5);
        draw_ui_text(title_pos, title, render::Color4f::WHITE, 12.0f);
    }
    
    // 找到最大值和最小值
    float min_val = values[0];
    float max_val = values[0];
    for (float val : values) {
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
    }
    
    float range = max_val - min_val;
    if (range < 0.001f) range = 1.0f;
    
    // 绘制数据点
    float x_step = size.x / (values.size() - 1);
    Vector2 graph_start(position.x, position.y + 20);
    Vector2 graph_size(size.x, size.y - 25);
    
    for (size_t i = 1; i < values.size(); ++i) {
        float normalized_y1 = (values[i-1] - min_val) / range;
        float normalized_y2 = (values[i] - min_val) / range;
        
        Vector2 p1(graph_start.x + (i-1) * x_step, 
                   graph_start.y + graph_size.y - normalized_y1 * graph_size.y);
        Vector2 p2(graph_start.x + i * x_step, 
                   graph_start.y + graph_size.y - normalized_y2 * graph_size.y);
        
        draw_ui_line(p1, p2, color, 2.0f);
    }
}

void UnifiedDebugDraw::draw_ui_histogram(const Vector2& position, const Vector2& size, const std::vector<float>& values, const render::Color4f& color, const std::string& title) {
    if (values.empty()) return;
    
    // 直方图背景
    draw_ui_rect(position, size, render::Color4f(0.1f, 0.1f, 0.1f, 0.8f), true);
    draw_ui_rect(position, size, render::Color4f::WHITE, false, 1.0f);
    
    // 标题
    if (!title.empty()) {
        Vector2 title_pos(position.x + 5, position.y + 5);
        draw_ui_text(title_pos, title, render::Color4f::WHITE, 12.0f);
    }
    
    // 找到最大值
    float max_val = 0.0f;
    for (float val : values) {
        max_val = std::max(max_val, val);
    }
    
    if (max_val < 0.001f) max_val = 1.0f;
    
    // 绘制柱状图
    float bar_width = size.x / values.size();
    Vector2 graph_start(position.x, position.y + 20);
    Vector2 graph_size(size.x, size.y - 25);
    
    for (size_t i = 0; i < values.size(); ++i) {
        float normalized_height = values[i] / max_val;
        
        Vector2 bar_pos(graph_start.x + i * bar_width, 
                        graph_start.y + graph_size.y - normalized_height * graph_size.y);
        Vector2 bar_size(bar_width - 1, normalized_height * graph_size.y);
        
        draw_ui_rect(bar_pos, bar_size, color, true);
    }
}

// ========== 高级功能实现 ==========

void UnifiedDebugDraw::draw_line_timed(const Vector3& start, const Vector3& end, float duration, const render::Color4f& color, float thickness) {
    render::Line3DData data(start, end, color, thickness);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_LINE_3D, data,
                                   static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG));
    cmd.duration = duration;
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_ui_text_timed(const Vector2& position, const std::string& text, float duration, const render::Color4f& color, float font_size) {
    render::UITextData data(position, text, color, font_size, 0);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_UI_TEXT, data,
                                   static_cast<uint32_t>(render::RenderLayer::UI_OVERLAY));
    cmd.duration = duration;
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_line_once(const Vector3& start, const Vector3& end, const render::Color4f& color, float thickness) {
    render::Line3DData data(start, end, color, thickness);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_LINE_3D, data,
                                   static_cast<uint32_t>(render::RenderLayer::WORLD_DEBUG),
                                   render::RENDER_FLAG_ONE_FRAME);
    get_manager().submit_command(cmd);
}

void UnifiedDebugDraw::draw_ui_text_once(const Vector2& position, const std::string& text, const render::Color4f& color, float font_size) {
    render::UITextData data(position, text, color, font_size, 0);
    render::UnifiedRenderCommand cmd(render::RenderCommandType::DRAW_UI_TEXT, data,
                                   static_cast<uint32_t>(render::RenderLayer::UI_OVERLAY),
                                   render::RENDER_FLAG_ONE_FRAME);
    get_manager().submit_command(cmd);
}

// ========== 系统控制实现 ==========

void UnifiedDebugDraw::clear_all() {
    get_manager().clear_commands();
}

void UnifiedDebugDraw::clear_3d() {
    // 清除所有3D命令类型
    auto& manager = get_manager();
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_LINE_3D);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_BOX_3D);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_SPHERE_3D);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_MESH_3D);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_ARROW_3D);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_COORDINATE_AXES_3D);
}

void UnifiedDebugDraw::clear_ui() {
    // 清除所有UI命令类型
    auto& manager = get_manager();
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_RECT);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_TEXT);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_TEXTURE);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_LINE);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_CIRCLE);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_WINDOW);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_BUTTON);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_SLIDER);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_PROGRESS_BAR);
    manager.clear_commands_by_type(render::RenderCommandType::DRAW_UI_GRAPH_LINE);
}

void UnifiedDebugDraw::clear_layer(uint32_t layer) {
    get_manager().clear_commands_by_layer(layer);
}

void UnifiedDebugDraw::set_enabled(bool enabled) {
    get_manager().set_enabled(enabled);
}

bool UnifiedDebugDraw::is_enabled() {
    return get_manager().is_enabled();
}

render::RenderStats UnifiedDebugDraw::get_stats() {
    return get_manager().get_render_stats();
}

void UnifiedDebugDraw::print_stats() {
    get_manager().print_stats();
}

// 模板函数的显式实例化（如果需要）
template<typename T>
void UnifiedDebugDraw::submit_custom_command(const T& data, uint32_t custom_type, uint32_t layer, uint32_t flags) {
    render::UnifiedRenderCommand cmd;
    cmd.type = static_cast<render::RenderCommandType>(custom_type);
    cmd.data = const_cast<T*>(&data);
    cmd.data_size = sizeof(T);
    cmd.layer = layer;
    cmd.flags = flags;
    
    get_manager().submit_command(cmd);
}

}} // namespace portal_core::debug
