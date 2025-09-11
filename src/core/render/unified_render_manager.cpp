#include "unified_render_manager.h"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace portal_core {
namespace render {

// 预定义颜色实现
const Color4f Color4f::WHITE(1.0f, 1.0f, 1.0f, 1.0f);
const Color4f Color4f::BLACK(0.0f, 0.0f, 0.0f, 1.0f);
const Color4f Color4f::RED(1.0f, 0.0f, 0.0f, 1.0f);
const Color4f Color4f::GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const Color4f Color4f::BLUE(0.0f, 0.0f, 1.0f, 1.0f);
const Color4f Color4f::YELLOW(1.0f, 1.0f, 0.0f, 1.0f);
const Color4f Color4f::CYAN(0.0f, 1.0f, 1.0f, 1.0f);
const Color4f Color4f::MAGENTA(1.0f, 0.0f, 1.0f, 1.0f);
const Color4f Color4f::TRANSPARENT(0.0f, 0.0f, 0.0f, 0.0f);

// StoredRenderCommand 实现
void StoredRenderCommand::create_from_command(const UnifiedRenderCommand& cmd, uint64_t current_frame_id) {
    command = cmd;
    command.frame_id = current_frame_id;
    created_time = std::chrono::steady_clock::now();
    
    // 创建数据的独立副本
    if (cmd.data && cmd.data_size > 0) {
        data_copy = std::make_unique<uint8_t[]>(cmd.data_size);
        std::memcpy(data_copy.get(), cmd.data, cmd.data_size);
        command.data = data_copy.get();
    } else {
        command.data = nullptr;
        command.data_size = 0;
    }
}

// UnifiedRenderManager 实现
UnifiedRenderManager::UnifiedRenderManager() 
    : current_frame_id_(0)
    , enabled_(true) {
}

UnifiedRenderManager::~UnifiedRenderManager() {
    clear_commands();
    renderers_.clear();
}

UnifiedRenderManager& UnifiedRenderManager::instance() {
    static UnifiedRenderManager instance;
    return instance;
}

void UnifiedRenderManager::register_renderer(IUnifiedRenderer* renderer) {
    if (!renderer) return;
    
    // 检查是否已经注册
    auto it = std::find(renderers_.begin(), renderers_.end(), renderer);
    if (it == renderers_.end()) {
        renderers_.push_back(renderer);
    }
}

void UnifiedRenderManager::unregister_renderer(IUnifiedRenderer* renderer) {
    auto it = std::find(renderers_.begin(), renderers_.end(), renderer);
    if (it != renderers_.end()) {
        renderers_.erase(it);
    }
}

void UnifiedRenderManager::clear_renderers() {
    renderers_.clear();
}

void UnifiedRenderManager::submit_command(const UnifiedRenderCommand& command) {
    if (!enabled_) return;
    
    StoredRenderCommand stored_cmd;
    stored_cmd.create_from_command(command, current_frame_id_);
    
    command_queue_.push_back(std::move(stored_cmd));
    
    // 更新统计
    command_count_by_type_[command.type]++;
    command_count_by_layer_[command.layer]++;
}

void UnifiedRenderManager::submit_commands(const UnifiedRenderCommand* commands, size_t count) {
    if (!enabled_ || !commands) return;
    
    for (size_t i = 0; i < count; ++i) {
        submit_command(commands[i]);
    }
}

void UnifiedRenderManager::clear_commands() {
    command_queue_.clear();
    command_count_by_type_.clear();
    command_count_by_layer_.clear();
}

void UnifiedRenderManager::clear_commands_by_layer(uint32_t layer) {
    auto it = std::remove_if(command_queue_.begin(), command_queue_.end(),
        [layer](const StoredRenderCommand& cmd) {
            return cmd.command.layer == layer;
        });
    
    if (it != command_queue_.end()) {
        command_queue_.erase(it, command_queue_.end());
        
        // 重新计算统计
        command_count_by_type_.clear();
        command_count_by_layer_.clear();
        for (const auto& cmd : command_queue_) {
            command_count_by_type_[cmd.command.type]++;
            command_count_by_layer_[cmd.command.layer]++;
        }
    }
}

void UnifiedRenderManager::clear_commands_by_type(RenderCommandType type) {
    auto it = std::remove_if(command_queue_.begin(), command_queue_.end(),
        [type](const StoredRenderCommand& cmd) {
            return cmd.command.type == type;
        });
    
    if (it != command_queue_.end()) {
        command_queue_.erase(it, command_queue_.end());
        
        // 重新计算统计
        command_count_by_type_.clear();
        command_count_by_layer_.clear();
        for (const auto& cmd : command_queue_) {
            command_count_by_type_[cmd.command.type]++;
            command_count_by_layer_[cmd.command.layer]++;
        }
    }
}

void UnifiedRenderManager::clear_expired_commands(float delta_time) {
    auto now = std::chrono::steady_clock::now();
    
    auto it = std::remove_if(command_queue_.begin(), command_queue_.end(),
        [now](const StoredRenderCommand& cmd) {
            // 检查一次性命令（ONE_FRAME标志）
            if (cmd.command.flags & RENDER_FLAG_ONE_FRAME) {
                return true;
            }
            
            // 检查持续时间
            if (cmd.command.duration >= 0.0f) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cmd.created_time).count() / 1000.0f;
                return elapsed >= cmd.command.duration;
            }
            
            return false;
        });
    
    if (it != command_queue_.end()) {
        command_queue_.erase(it, command_queue_.end());
        
        // 重新计算统计
        command_count_by_type_.clear();
        command_count_by_layer_.clear();
        for (const auto& cmd : command_queue_) {
            command_count_by_type_[cmd.command.type]++;
            command_count_by_layer_[cmd.command.layer]++;
        }
    }
}

void UnifiedRenderManager::flush_commands() {
    if (!enabled_) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 分发到所有渲染器
    for (auto* renderer : renderers_) {
        if (renderer && renderer->is_enabled()) {
            renderer->clear_commands();
            
            for (const auto& stored_cmd : command_queue_) {
                if (renderer->supports_command_type(stored_cmd.command.type) &&
                    renderer->supports_layer(stored_cmd.command.layer)) {
                    renderer->submit_command(stored_cmd.command);
                }
            }
            
            renderer->render();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // 更新统计信息
    current_stats_.total_commands = static_cast<uint32_t>(command_queue_.size());
    current_stats_.commands_3d = 0;
    current_stats_.commands_ui = 0;
    current_stats_.commands_custom = 0;
    current_stats_.total_vertices = 0;
    current_stats_.frame_time_ms = duration.count() / 1000.0f;
    
    for (const auto& [type, count] : command_count_by_type_) {
        if (static_cast<uint32_t>(type) >= 0x1000 && static_cast<uint32_t>(type) < 0x2000) {
            current_stats_.commands_3d += static_cast<uint32_t>(count);
        } else if (static_cast<uint32_t>(type) >= 0x2000 && static_cast<uint32_t>(type) < 0x8000) {
            current_stats_.commands_ui += static_cast<uint32_t>(count);
        } else {
            current_stats_.commands_custom += static_cast<uint32_t>(count);
        }
    }
}

void UnifiedRenderManager::update(float delta_time) {
    if (!enabled_) return;
    
    // 清理过期命令
    clear_expired_commands(delta_time);
    
    // 更新所有渲染器
    for (auto* renderer : renderers_) {
        if (renderer && renderer->is_enabled()) {
            renderer->update(delta_time);
        }
    }
}

size_t UnifiedRenderManager::get_command_count_by_layer(uint32_t layer) const {
    auto it = command_count_by_layer_.find(layer);
    return it != command_count_by_layer_.end() ? it->second : 0;
}

size_t UnifiedRenderManager::get_command_count_by_type(RenderCommandType type) const {
    auto it = command_count_by_type_.find(type);
    return it != command_count_by_type_.end() ? it->second : 0;
}

void UnifiedRenderManager::set_enabled(bool enabled) {
    enabled_ = enabled;
    
    // 同步到所有渲染器
    for (auto* renderer : renderers_) {
        if (renderer) {
            renderer->set_enabled(enabled);
        }
    }
}

void UnifiedRenderManager::print_stats() const {
    std::cout << "=== Unified Render Manager Stats ===" << std::endl;
    std::cout << "Total Commands: " << current_stats_.total_commands << std::endl;
    std::cout << "3D Commands: " << current_stats_.commands_3d << std::endl;
    std::cout << "UI Commands: " << current_stats_.commands_ui << std::endl;
    std::cout << "Custom Commands: " << current_stats_.commands_custom << std::endl;
    std::cout << "Frame Time: " << current_stats_.frame_time_ms << "ms" << std::endl;
    std::cout << "Registered Renderers: " << renderers_.size() << std::endl;
    std::cout << "Current Frame ID: " << current_frame_id_ << std::endl;
    std::cout << "Enabled: " << (enabled_ ? "Yes" : "No") << std::endl;
}

void UnifiedRenderManager::print_renderers() const {
    std::cout << "=== Registered Renderers ===" << std::endl;
    for (size_t i = 0; i < renderers_.size(); ++i) {
        auto* renderer = renderers_[i];
        if (renderer) {
            std::cout << "[" << i << "] " << renderer->get_renderer_name() 
                      << " (Enabled: " << (renderer->is_enabled() ? "Yes" : "No") << ")" << std::endl;
        } else {
            std::cout << "[" << i << "] <null renderer>" << std::endl;
        }
    }
}

}} // namespace portal_core::render
