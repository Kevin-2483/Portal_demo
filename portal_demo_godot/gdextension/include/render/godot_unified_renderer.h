#pragma once

#include "core/render/i_unified_renderer.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <vector>
#include <memory>

namespace portal_gdext {
namespace render {

// 前向声明
class GodotRenderer3D;
class GodotRendererUI;

/**
 * Godot统一渲染器实现
 * 实现统一渲染接口，分发命令到3D和UI渲染器
 */
class GodotUnifiedRenderer : public portal_core::render::IUnifiedRenderer {
private:
    std::vector<portal_core::render::UnifiedRenderCommand> command_queue_;
    std::unique_ptr<GodotRenderer3D> renderer_3d_;
    GodotRendererUI* renderer_ui_;  // 改为原始指针，因为这是Godot对象
    
    portal_core::render::RenderStats stats_;
    bool enabled_;
    
public:
    GodotUnifiedRenderer();
    virtual ~GodotUnifiedRenderer();
    
    // 初始化渲染器（需要在Godot场景树中调用）
    bool initialize(godot::Node3D* world_node, godot::Control* ui_node);
    void shutdown();
    
    // IUnifiedRenderer 接口实现
    void submit_command(const portal_core::render::UnifiedRenderCommand& command) override;
    void submit_commands(const portal_core::render::UnifiedRenderCommand* commands, size_t count) override;
    void clear_commands() override;
    void clear_commands_by_layer(uint32_t layer) override;
    void clear_commands_by_type(portal_core::render::RenderCommandType type) override;
    void render() override;
    void update(float delta_time) override;
    
    // 统计和查询
    size_t get_command_count() const override;
    size_t get_command_count_by_layer(uint32_t layer) const override;
    size_t get_command_count_by_type(portal_core::render::RenderCommandType type) const override;
    portal_core::render::RenderStats get_render_stats() const override;
    
    // 能力查询
    bool supports_command_type(portal_core::render::RenderCommandType type) const override;
    bool supports_layer(uint32_t layer) const override;
    
    // 状态管理
    bool is_enabled() const override { return enabled_; }
    void set_enabled(bool enabled) override;
    const char* get_renderer_name() const override { return "GodotUnifiedRenderer"; }
    
private:
    void dispatch_3d_command(const portal_core::render::UnifiedRenderCommand& command);
    void dispatch_ui_command(const portal_core::render::UnifiedRenderCommand& command);
    bool is_3d_command(portal_core::render::RenderCommandType type) const;
    bool is_ui_command(portal_core::render::RenderCommandType type) const;
    void update_stats();
    void redistribute_commands();
};

}} // namespace portal_gdext::render
