#include "render/godot_renderer_ui.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>

using namespace godot;

namespace portal_gdext {
namespace render {

void GodotRendererUI::_bind_methods() {
    // 如果需要暴露给GDScript，在这里绑定方法
}

GodotRendererUI::GodotRendererUI() 
    : enabled_(true) {
}

GodotRendererUI::~GodotRendererUI() {
    // 清理资源
}

void GodotRendererUI::_ready() {
    // 设置为全屏覆盖
    set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    
    // 确保这个控件不会阻挡鼠标事件
    set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    
    // 设置为最高层级
    set_z_index(1000);
    
    UtilityFunctions::print("GodotRendererUI initialized");
}

void GodotRendererUI::_draw() {
    if (!enabled_) return;
    
    // 绘制所有UI命令
    for (const auto& command : commands_) {
        switch (command.type) {
            case portal_core::render::RenderCommandType::DRAW_UI_RECT:
                if (command.data && command.data_size == sizeof(portal_core::render::UIRectData)) {
                    render_ui_rect(*static_cast<const portal_core::render::UIRectData*>(command.data));
                }
                break;
                
            case portal_core::render::RenderCommandType::DRAW_UI_TEXT:
                if (command.data && command.data_size == sizeof(portal_core::render::UITextData)) {
                    render_ui_text(*static_cast<const portal_core::render::UITextData*>(command.data));
                }
                break;
                
            case portal_core::render::RenderCommandType::DRAW_UI_LINE:
                if (command.data && command.data_size == sizeof(portal_core::render::UILineData)) {
                    render_ui_line(*static_cast<const portal_core::render::UILineData*>(command.data));
                }
                break;
                
            default:
                // 不支持的UI命令类型，忽略
                break;
        }
    }
}

void GodotRendererUI::submit_command(const portal_core::render::UnifiedRenderCommand& command) {
    if (!enabled_) return;
    commands_.push_back(command);
}

void GodotRendererUI::clear_commands() {
    commands_.clear();
}

void GodotRendererUI::render() {
    if (!enabled_) return;
    
    // 触发重绘
    queue_redraw();
}

void GodotRendererUI::update(float delta_time) {
    // UI渲染器的更新逻辑
    // 目前主要是触发重绘
    if (!commands_.empty()) {
        queue_redraw();
    }
}

void GodotRendererUI::set_enabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        clear_commands();
        queue_redraw();
    }
}

void GodotRendererUI::render_ui_rect(const portal_core::render::UIRectData& data) {
    Vector2 position = to_godot_vector2(data.position);
    Vector2 size = to_godot_vector2(data.size);
    Color color = to_godot_color(data.color);
    
    Rect2 rect(position, size);
    
    if (data.filled) {
        // 绘制填充矩形
        draw_rect(rect, color);
    } else {
        // 绘制边框
        draw_rect_outline(rect, color, data.border_width);
    }
}

void GodotRendererUI::render_ui_text(const portal_core::render::UITextData& data) {
    Vector2 position = to_godot_vector2(data.position);
    Color color = to_godot_color(data.color);
    String text = String::utf8(data.text.c_str());
    
    draw_text_with_font(position, text, color, data.font_size, data.align);
}

void GodotRendererUI::render_ui_line(const portal_core::render::UILineData& data) {
    Vector2 start = to_godot_vector2(data.start);
    Vector2 end = to_godot_vector2(data.end);
    Color color = to_godot_color(data.color);
    
    draw_line(start, end, color, data.thickness);
}

Vector2 GodotRendererUI::to_godot_vector2(const portal_core::Vector2& vec) const {
    return Vector2(vec.x, vec.y);
}

Color GodotRendererUI::to_godot_color(const portal_core::render::Color4f& color) const {
    return Color(color.r, color.g, color.b, color.a);
}

void GodotRendererUI::draw_rect_outline(const Rect2& rect, const Color& color, float width) {
    // 绘制矩形的4条边
    Vector2 top_left = rect.position;
    Vector2 top_right = Vector2(rect.position.x + rect.size.x, rect.position.y);
    Vector2 bottom_left = Vector2(rect.position.x, rect.position.y + rect.size.y);
    Vector2 bottom_right = rect.position + rect.size;
    
    // 上边
    draw_line(top_left, top_right, color, width);
    // 右边
    draw_line(top_right, bottom_right, color, width);
    // 下边
    draw_line(bottom_right, bottom_left, color, width);
    // 左边
    draw_line(bottom_left, top_left, color, width);
}

void GodotRendererUI::draw_text_with_font(const Vector2& position, const String& text, 
                                         const Color& color, float font_size, int align) {
    // 获取默认字体
    Ref<Font> font;
    Ref<Theme> theme = get_theme();
    if (theme.is_valid()) {
        font = theme->get_default_font();
    }
    
    if (!font.is_valid()) {
        // 如果没有字体，使用系统默认方式绘制
        draw_string(font, position, text, HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(font_size), color);
        return;
    }
    
    Vector2 text_size = font->get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(font_size));
    Vector2 draw_position = position;
    
    // 根据对齐方式调整位置
    switch (align) {
        case 1: // 居中对齐
            draw_position.x -= text_size.x * 0.5f;
            break;
        case 2: // 右对齐
            draw_position.x -= text_size.x;
            break;
        default: // 左对齐 (0)
            break;
    }
    
    draw_string(font, draw_position, text, HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(font_size), color);
}

}} // namespace portal_gdext::render
