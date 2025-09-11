#pragma once

#include "unified_render_types.h"

namespace portal_core {
namespace render {

// 统一渲染接口
class IUnifiedRenderer {
public:
    virtual ~IUnifiedRenderer() = default;
    
    // 提交单个渲染命令
    virtual void submit_command(const UnifiedRenderCommand& command) = 0;
    
    // 批量提交渲染命令
    virtual void submit_commands(const UnifiedRenderCommand* commands, size_t count) = 0;
    
    // 清空所有命令队列
    virtual void clear_commands() = 0;
    
    // 清空指定层级的命令
    virtual void clear_commands_by_layer(uint32_t layer) = 0;
    
    // 清空指定类型的命令
    virtual void clear_commands_by_type(RenderCommandType type) = 0;
    
    // 执行渲染
    virtual void render() = 0;
    
    // 更新和清理过期命令
    virtual void update(float delta_time) = 0;
    
    // 获取命令统计
    virtual size_t get_command_count() const = 0;
    virtual size_t get_command_count_by_layer(uint32_t layer) const = 0;
    virtual size_t get_command_count_by_type(RenderCommandType type) const = 0;
    
    // 获取渲染统计
    virtual RenderStats get_render_stats() const = 0;
    
    // 渲染器能力查询
    virtual bool supports_command_type(RenderCommandType type) const = 0;
    virtual bool supports_layer(uint32_t layer) const = 0;
    
    // 渲染器状态
    virtual bool is_enabled() const = 0;
    virtual void set_enabled(bool enabled) = 0;
    
    // 获取渲染器名称（用于调试）
    virtual const char* get_renderer_name() const = 0;
};

}} // namespace portal_core::render
