#pragma once

#include "core/render/unified_render_types.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <vector>

namespace portal_gdext {
namespace render {

/**
 * Godot UI渲染器
 * 处理2D屏幕空间的UI绘制命令
 */
class GodotRendererUI : public godot::Control {
    GDCLASS(GodotRendererUI, godot::Control)
    
private:
    std::vector<portal_core::render::UnifiedRenderCommand> commands_;
    bool enabled_;
    
public:
    GodotRendererUI();
    ~GodotRendererUI();
    
    // Godot方法
    void _ready() override;
    void _draw() override;
    
    static void _bind_methods();
    
    // 命令处理
    void submit_command(const portal_core::render::UnifiedRenderCommand& command);
    void clear_commands();
    void render();
    void update(float delta_time);
    
    // 状态
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    
    // 统计
    size_t get_command_count() const { return commands_.size(); }
    
private:
    void render_ui_rect(const portal_core::render::UIRectData& data);
    void render_ui_text(const portal_core::render::UITextData& data);
    void render_ui_line(const portal_core::render::UILineData& data);
    
    // 转换辅助函数
    godot::Vector2 to_godot_vector2(const portal_core::Vector2& vec) const;
    godot::Color to_godot_color(const portal_core::render::Color4f& color) const;
    
    // 绘制辅助函数
    void draw_rect_outline(const godot::Rect2& rect, const godot::Color& color, float width);
    void draw_text_with_font(const godot::Vector2& position, const godot::String& text, 
                            const godot::Color& color, float font_size, int align);
};

}} // namespace portal_gdext::render
