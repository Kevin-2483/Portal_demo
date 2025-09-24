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

#ifdef PORTAL_DEBUG_GUI_ENABLED
            case portal_core::render::RenderCommandType::DRAW_IMGUI_VERTICES:
                if (command.data && command.data_size >= sizeof(portal_core::render::ImGuiMeshData)) {
                    render_imgui_mesh(*static_cast<const portal_core::render::ImGuiMeshData*>(command.data));
                }
                break;
                
            case portal_core::render::RenderCommandType::DRAW_IMGUI_TEXTURE:
                if (command.data && command.data_size == sizeof(portal_core::render::ImGuiTextureData)) {
                    render_imgui_texture(*static_cast<const portal_core::render::ImGuiTextureData*>(command.data));
                }
                break;
#endif
                
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

#ifdef PORTAL_DEBUG_GUI_ENABLED
void GodotRendererUI::render_imgui_mesh(const portal_core::render::ImGuiMeshData& data) {
    if (!data.vertices || data.vertex_count == 0 || !data.indices || data.index_count == 0) {
        return;
    }
    
    // 设置裁剪区域
    if (data.use_clipping) {
        Vector2 clip_pos(data.clip_rect_min.x, data.clip_rect_min.y);
        Vector2 clip_size(data.clip_rect_max.x - data.clip_rect_min.x, data.clip_rect_max.y - data.clip_rect_min.y);
        Rect2 clip_rect(clip_pos, clip_size);
        
        // 应用裁剪
        set_clip_contents(true);
        // 注意：Godot的裁剪是基于节点的，这里我们需要手动检查每个顶点是否在裁剪区域内
    }
    
    // 将ImGui顶点转换为Godot格式并绘制三角形
    for (uint32_t i = 0; i < data.index_count; i += 3) {
        if (i + 2 >= data.index_count) break;
        
        // 获取三角形的三个顶点索引
        uint16_t idx0 = data.indices[i];
        uint16_t idx1 = data.indices[i + 1];
        uint16_t idx2 = data.indices[i + 2];
        
        if (idx0 >= data.vertex_count || idx1 >= data.vertex_count || idx2 >= data.vertex_count) {
            continue;
        }
        
        // 获取顶点数据
        const auto& v0 = data.vertices[idx0];
        const auto& v1 = data.vertices[idx1];
        const auto& v2 = data.vertices[idx2];
        
        // 转换为Godot格式
        Vector2 p0(v0.pos.x, v0.pos.y);
        Vector2 p1(v1.pos.x, v1.pos.y);
        Vector2 p2(v2.pos.x, v2.pos.y);
        
        // 数据有效性检查
        // 1. 检查坐标是否为有效数值
        if (!Math::is_finite(p0.x) || !Math::is_finite(p0.y) ||
            !Math::is_finite(p1.x) || !Math::is_finite(p1.y) ||
            !Math::is_finite(p2.x) || !Math::is_finite(p2.y)) {
            continue; // 跳过无效坐标
        }
        
        // 2. 检查坐标是否在合理范围内（避免极大值导致的渲染问题）
        const float MAX_COORD = 100000.0f;
        if (Math::abs(p0.x) > MAX_COORD || Math::abs(p0.y) > MAX_COORD ||
            Math::abs(p1.x) > MAX_COORD || Math::abs(p1.y) > MAX_COORD ||
            Math::abs(p2.x) > MAX_COORD || Math::abs(p2.y) > MAX_COORD) {
            continue; // 跳过超出范围的坐标
        }
        
        // 3. 检查是否为退化三角形（三点共线或重合）
        Vector2 edge1 = p1 - p0;
        Vector2 edge2 = p2 - p0;
        float cross_product = edge1.x * edge2.y - edge1.y * edge2.x;
        const float MIN_AREA = 0.01f; // 最小三角形面积阈值
        if (Math::abs(cross_product) < MIN_AREA) {
            continue; // 跳过退化三角形
        }
        
        // 4. 应用裁剪检查（如果启用了裁剪）
        if (data.use_clipping) {
            Vector2 clip_min(data.clip_rect_min.x, data.clip_rect_min.y);
            Vector2 clip_max(data.clip_rect_max.x, data.clip_rect_max.y);
            
            // 检查三角形是否完全在裁剪区域外
            if ((p0.x < clip_min.x && p1.x < clip_min.x && p2.x < clip_min.x) ||
                (p0.x > clip_max.x && p1.x > clip_max.x && p2.x > clip_max.x) ||
                (p0.y < clip_min.y && p1.y < clip_min.y && p2.y < clip_min.y) ||
                (p0.y > clip_max.y && p1.y > clip_max.y && p2.y > clip_max.y)) {
                continue; // 跳过完全在裁剪区域外的三角形
            }
        }
        
        Color c0(
            ((v0.col >> 0) & 0xFF) / 255.0f,   // R
            ((v0.col >> 8) & 0xFF) / 255.0f,   // G
            ((v0.col >> 16) & 0xFF) / 255.0f,  // B
            ((v0.col >> 24) & 0xFF) / 255.0f   // A
        );
        Color c1(
            ((v1.col >> 0) & 0xFF) / 255.0f,   // R
            ((v1.col >> 8) & 0xFF) / 255.0f,   // G
            ((v1.col >> 16) & 0xFF) / 255.0f,  // B
            ((v1.col >> 24) & 0xFF) / 255.0f   // A
        );
        Color c2(
            ((v2.col >> 0) & 0xFF) / 255.0f,   // R
            ((v2.col >> 8) & 0xFF) / 255.0f,   // G
            ((v2.col >> 16) & 0xFF) / 255.0f,  // B
            ((v2.col >> 24) & 0xFF) / 255.0f   // A
        );
        
        // 5. 检查颜色透明度，跳过完全透明的三角形
        if (c0.a <= 0.0f && c1.a <= 0.0f && c2.a <= 0.0f) {
            continue;
        }
        
        // 准备绘制数据
        PackedVector2Array triangle_points;
        PackedColorArray triangle_colors;
        triangle_points.push_back(p0);
        triangle_points.push_back(p1);
        triangle_points.push_back(p2);
        triangle_colors.push_back(c0);
        triangle_colors.push_back(c1);
        triangle_colors.push_back(c2);
        
        // 绘制三角形
        if (data.texture_id != ImTextureID_Invalid) {
            // 有纹理，绘制带纹理的三角形
            draw_polygon(triangle_points, triangle_colors);
        } else {
            // 无纹理，直接绘制彩色三角形
            draw_polygon(triangle_points, triangle_colors);
        }
    }
    
    // 重置裁剪
    set_clip_contents(false);
}

void GodotRendererUI::render_imgui_texture(const portal_core::render::ImGuiTextureData& data) {
    // 这个方法用于处理纯纹理绘制命令（如果需要的话）
    // 目前ImGui的纹理绘制主要通过render_imgui_mesh处理
    // 这里可以作为扩展点，用于特殊的纹理处理需求
    
    if (data.texture_id == ImTextureID_Invalid) {
        return;
    }
    
    // TODO: 根据texture_id获取Godot纹理并绘制
    // 这需要实现纹理管理系统来映射ImGui纹理ID到Godot纹理
}
#endif

}} // namespace portal_gdext::render
