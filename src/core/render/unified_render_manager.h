#pragma once

#include "i_unified_renderer.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace portal_core {
namespace render {

// 渲染命令存储结构（包含数据副本）
struct StoredRenderCommand {
    UnifiedRenderCommand command;
    std::unique_ptr<uint8_t[]> data_copy;  // 数据的独立副本
    std::chrono::steady_clock::time_point created_time;
    
    StoredRenderCommand() = default;
    StoredRenderCommand(const StoredRenderCommand&) = delete;
    StoredRenderCommand& operator=(const StoredRenderCommand&) = delete;
    StoredRenderCommand(StoredRenderCommand&&) = default;
    StoredRenderCommand& operator=(StoredRenderCommand&&) = default;
    
    // 从原始命令创建存储命令
    void create_from_command(const UnifiedRenderCommand& cmd, uint64_t current_frame_id);
};

// 统一渲染管理器
class UnifiedRenderManager {
private:
    std::vector<IUnifiedRenderer*> renderers_;
    std::vector<StoredRenderCommand> command_queue_;
    std::unordered_map<RenderCommandType, size_t> command_count_by_type_;
    std::unordered_map<uint32_t, size_t> command_count_by_layer_;
    
    RenderStats current_stats_;
    uint64_t current_frame_id_;
    bool enabled_;
    
    // 单例
    UnifiedRenderManager();
    
public:
    ~UnifiedRenderManager();
    
    // 单例访问
    static UnifiedRenderManager& instance();
    
    // 渲染器管理
    void register_renderer(IUnifiedRenderer* renderer);
    void unregister_renderer(IUnifiedRenderer* renderer);
    void clear_renderers();
    size_t get_renderer_count() const { return renderers_.size(); }
    
    // 命令提交
    void submit_command(const UnifiedRenderCommand& command);
    void submit_commands(const UnifiedRenderCommand* commands, size_t count);
    
    // 清理命令
    void clear_commands();
    void clear_commands_by_layer(uint32_t layer);
    void clear_commands_by_type(RenderCommandType type);
    void clear_expired_commands(float delta_time);
    
    // 分发到所有渲染器
    void flush_commands();
    
    // 更新
    void update(float delta_time);
    
    // 统计信息
    size_t get_command_count() const { return command_queue_.size(); }
    size_t get_command_count_by_layer(uint32_t layer) const;
    size_t get_command_count_by_type(RenderCommandType type) const;
    const RenderStats& get_render_stats() const { return current_stats_; }
    
    // 帧管理
    void advance_frame() { current_frame_id_++; }
    uint64_t get_current_frame_id() const { return current_frame_id_; }
    
    // 系统状态
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    
    // 调试功能
    void print_stats() const;
    void print_renderers() const;
};

}} // namespace portal_core::render
