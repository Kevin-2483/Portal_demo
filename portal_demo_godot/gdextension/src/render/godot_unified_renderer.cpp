#include "render/godot_unified_renderer.h"
#include "render/godot_renderer_3d.h"
#include "render/godot_renderer_ui.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <chrono>

using namespace godot;

namespace portal_gdext {
namespace render {

GodotUnifiedRenderer::GodotUnifiedRenderer() 
    : enabled_(true) {
    renderer_3d_ = std::make_unique<GodotRenderer3D>();
    renderer_ui_ = memnew(GodotRendererUI);  // 使用memnew创建Godot对象
}

GodotUnifiedRenderer::~GodotUnifiedRenderer() {
    shutdown();
}

bool GodotUnifiedRenderer::initialize(godot::Node3D* world_node, godot::Control* ui_node) {
    if (!world_node) {
        UtilityFunctions::printerr("GodotUnifiedRenderer: world_node is null");
        return false;
    }
    
    // 初始化3D渲染器
    if (!renderer_3d_->initialize(world_node)) {
        UtilityFunctions::printerr("GodotUnifiedRenderer: Failed to initialize 3D renderer");
        return false;
    }
    
    // 初始化UI渲染器
    if (ui_node) {
        // 如果提供了UI节点，将UI渲染器添加为子节点
        ui_node->add_child(renderer_ui_);
    } else {
        // 否则添加到世界节点
        world_node->add_child(renderer_ui_);
    }
    
    UtilityFunctions::print("GodotUnifiedRenderer initialized successfully");
    return true;
}

void GodotUnifiedRenderer::shutdown() {
    if (renderer_3d_) {
        renderer_3d_->shutdown();
    }
    
    if (renderer_ui_) {
        // UI渲染器是Godot节点，需要从场景树中移除然后删除
        if (renderer_ui_->get_parent()) {
            renderer_ui_->get_parent()->remove_child(renderer_ui_);
        }
        memdelete(renderer_ui_);  // 使用memdelete删除Godot对象
        renderer_ui_ = nullptr;
    }
    
    clear_commands();
}

void GodotUnifiedRenderer::submit_command(const portal_core::render::UnifiedRenderCommand& command) {
    if (!enabled_) return;
    
    command_queue_.push_back(command);
    
    // 立即分发到相应的渲染器
    if (is_3d_command(command.type)) {
        dispatch_3d_command(command);
    } else if (is_ui_command(command.type)) {
        dispatch_ui_command(command);
    }
}

void GodotUnifiedRenderer::submit_commands(const portal_core::render::UnifiedRenderCommand* commands, size_t count) {
    if (!enabled_ || !commands) return;
    
    for (size_t i = 0; i < count; ++i) {
        submit_command(commands[i]);
    }
}

void GodotUnifiedRenderer::clear_commands() {
    command_queue_.clear();
    
    if (renderer_3d_) {
        renderer_3d_->clear_commands();
    }
    
    if (renderer_ui_) {
        renderer_ui_->clear_commands();
    }
}

void GodotUnifiedRenderer::clear_commands_by_layer(uint32_t layer) {
    // 从队列中移除指定层级的命令
    auto it = std::remove_if(command_queue_.begin(), command_queue_.end(),
        [layer](const portal_core::render::UnifiedRenderCommand& cmd) {
            return cmd.layer == layer;
        });
    
    command_queue_.erase(it, command_queue_.end());
    
    // 重新分发剩余命令
    redistribute_commands();
}

void GodotUnifiedRenderer::clear_commands_by_type(portal_core::render::RenderCommandType type) {
    // 从队列中移除指定类型的命令
    auto it = std::remove_if(command_queue_.begin(), command_queue_.end(),
        [type](const portal_core::render::UnifiedRenderCommand& cmd) {
            return cmd.type == type;
        });
    
    command_queue_.erase(it, command_queue_.end());
    
    // 重新分发剩余命令
    redistribute_commands();
}

void GodotUnifiedRenderer::render() {
    if (!enabled_) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 渲染3D内容
    if (renderer_3d_) {
        renderer_3d_->render();
    }
    
    // 渲染UI内容
    if (renderer_ui_) {
        renderer_ui_->render();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // 更新统计
    update_stats();
    stats_.frame_time_ms = duration.count() / 1000.0f;
}

void GodotUnifiedRenderer::update(float delta_time) {
    if (!enabled_) return;
    
    if (renderer_3d_) {
        renderer_3d_->update(delta_time);
    }
    
    if (renderer_ui_) {
        renderer_ui_->update(delta_time);
    }
}

size_t GodotUnifiedRenderer::get_command_count() const {
    return command_queue_.size();
}

size_t GodotUnifiedRenderer::get_command_count_by_layer(uint32_t layer) const {
    return std::count_if(command_queue_.begin(), command_queue_.end(),
        [layer](const portal_core::render::UnifiedRenderCommand& cmd) {
            return cmd.layer == layer;
        });
}

size_t GodotUnifiedRenderer::get_command_count_by_type(portal_core::render::RenderCommandType type) const {
    return std::count_if(command_queue_.begin(), command_queue_.end(),
        [type](const portal_core::render::UnifiedRenderCommand& cmd) {
            return cmd.type == type;
        });
}

portal_core::render::RenderStats GodotUnifiedRenderer::get_render_stats() const {
    return stats_;
}

bool GodotUnifiedRenderer::supports_command_type(portal_core::render::RenderCommandType type) const {
    // 检查是否支持该命令类型
    switch (type) {
        // 3D命令
        case portal_core::render::RenderCommandType::DRAW_LINE_3D:
        case portal_core::render::RenderCommandType::DRAW_BOX_3D:
        case portal_core::render::RenderCommandType::DRAW_SPHERE_3D:
        // UI命令
        case portal_core::render::RenderCommandType::DRAW_UI_RECT:
        case portal_core::render::RenderCommandType::DRAW_UI_TEXT:
        case portal_core::render::RenderCommandType::DRAW_UI_LINE:
            return true;
            
        default:
            return false;
    }
}

bool GodotUnifiedRenderer::supports_layer(uint32_t layer) const {
    // 所有层级都支持
    return true;
}

void GodotUnifiedRenderer::set_enabled(bool enabled) {
    enabled_ = enabled;
    
    if (renderer_3d_) {
        renderer_3d_->set_enabled(enabled);
    }
    
    if (renderer_ui_) {
        renderer_ui_->set_enabled(enabled);
    }
}

void GodotUnifiedRenderer::dispatch_3d_command(const portal_core::render::UnifiedRenderCommand& command) {
    if (renderer_3d_ && renderer_3d_->is_enabled()) {
        renderer_3d_->submit_command(command);
    }
}

void GodotUnifiedRenderer::dispatch_ui_command(const portal_core::render::UnifiedRenderCommand& command) {
    if (renderer_ui_ && renderer_ui_->is_enabled()) {
        renderer_ui_->submit_command(command);
    }
}

bool GodotUnifiedRenderer::is_3d_command(portal_core::render::RenderCommandType type) const {
    uint32_t type_value = static_cast<uint32_t>(type);
    return type_value >= 0x1000 && type_value < 0x2000;
}

bool GodotUnifiedRenderer::is_ui_command(portal_core::render::RenderCommandType type) const {
    uint32_t type_value = static_cast<uint32_t>(type);
    return type_value >= 0x2000 && type_value < 0x8000;
}

void GodotUnifiedRenderer::update_stats() {
    stats_.total_commands = static_cast<uint32_t>(command_queue_.size());
    stats_.commands_3d = 0;
    stats_.commands_ui = 0;
    stats_.commands_custom = 0;
    
    for (const auto& cmd : command_queue_) {
        if (is_3d_command(cmd.type)) {
            stats_.commands_3d++;
        } else if (is_ui_command(cmd.type)) {
            stats_.commands_ui++;
        } else {
            stats_.commands_custom++;
        }
    }
    
    // 估算顶点数（简化计算）
    stats_.total_vertices = stats_.commands_3d * 2 + stats_.commands_ui * 4; // 线段2个顶点，UI元素4个顶点
}

void GodotUnifiedRenderer::redistribute_commands() {
    // 清空渲染器
    if (renderer_3d_) {
        renderer_3d_->clear_commands();
    }
    
    if (renderer_ui_) {
        renderer_ui_->clear_commands();
    }
    
    // 重新分发所有命令
    for (const auto& command : command_queue_) {
        if (is_3d_command(command.type)) {
            dispatch_3d_command(command);
        } else if (is_ui_command(command.type)) {
            dispatch_ui_command(command);
        }
    }
}

}} // namespace portal_gdext::render
