#pragma once

#include "debug_config.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "imgui.h"
#include "../math_types.h"
#include "debuggable_registry.h"

namespace portal_core {
namespace debug {

// ==============================================================================
// 前向声明
// ==============================================================================
class DebugWindow;
class DebugUIComponent;

// ==============================================================================
// 调试GUI系统核心类
// ==============================================================================

/**
 * 基于ImGui的调试GUI系统
 * 负责管理调试窗口、组件和ImGui上下文
 */
class DebugGUISystem {
public:
    // 系统生命周期
    static DebugGUISystem& instance();
    bool initialize();
    void shutdown();
    
    // 更新和渲染
    void update(float delta_time);
    void render();
    void flush_to_unified_renderer();
    
    // 窗口管理
    void register_window(std::unique_ptr<DebugWindow> window);
    void unregister_window(const std::string& window_id);
    DebugWindow* find_window(const std::string& window_id);
    
    // 系统状态
    bool is_initialized() const { return initialized_; }
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    // ImGui上下文管理
    ImGuiContext* get_imgui_context() const { return imgui_context_; }
    
    // 统计信息
    struct GUIStats {
        size_t window_count = 0;
        size_t visible_window_count = 0;
        float frame_time_ms = 0.0f;
        float render_time_ms = 0.0f;
    };
    
    const GUIStats& get_stats() const { return stats_; }
    void print_stats() const;

private:
    DebugGUISystem() = default;
    ~DebugGUISystem() = default;
    DebugGUISystem(const DebugGUISystem&) = delete;
    DebugGUISystem& operator=(const DebugGUISystem&) = delete;
    
    // 初始化子系统
    bool initialize_imgui();
    void setup_imgui_style();
    
    // 内部状态
    bool initialized_ = false;
    bool enabled_ = true;
    ImGuiContext* imgui_context_ = nullptr;
    
    // 窗口管理
    std::vector<std::unique_ptr<DebugWindow>> windows_;
    std::unordered_map<std::string, DebugWindow*> window_map_;
    
    // 统计信息
    mutable GUIStats stats_;
    float frame_timer_ = 0.0f;
};

// ==============================================================================
// 调试窗口基类
// ==============================================================================

/**
 * 调试窗口抽象基类
 * 所有调试窗口都继承自这个类
 */
class DebugWindow {
public:
    DebugWindow(const std::string& id, const std::string& title);
    virtual ~DebugWindow() = default;
    
    // 窗口渲染接口 - 子类必须实现
    virtual void render() = 0;
    
    // 窗口控制
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    void set_title(const std::string& title) { title_ = title; }
    const std::string& get_title() const { return title_; }
    
    const std::string& get_id() const { return window_id_; }
    
    // 窗口位置和大小
    void set_position(const Vector2& pos) { position_ = pos; position_set_ = true; }
    void set_size(const Vector2& size) { size_ = size; size_set_ = true; }
    
    const Vector2& get_position() const { return position_; }
    const Vector2& get_size() const { return size_; }
    
    // 窗口标志
    void set_flags(ImGuiWindowFlags flags) { window_flags_ = flags; }
    ImGuiWindowFlags get_flags() const { return window_flags_; }

protected:
    // 便利的渲染帮助函数
    void begin_window();
    void end_window();
    bool should_render() const { return visible_; }
    
    // 窗口属性
    std::string window_id_;
    std::string title_;
    bool visible_ = true;
    
    Vector2 position_ = Vector2(100, 100);
    Vector2 size_ = Vector2(300, 200);
    bool position_set_ = false;
    bool size_set_ = false;
    
    ImGuiWindowFlags window_flags_ = ImGuiWindowFlags_None;
};

// ==============================================================================
// 预设UI组件
// ==============================================================================

/**
 * 调试UI组件基类
 * 提供常用的UI组件封装
 */
class DebugUIComponent {
public:
    virtual ~DebugUIComponent() = default;
    virtual void render() = 0;
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

protected:
    bool enabled_ = true;
};

/**
 * 简单图表组件
 */
class DebugChart : public DebugUIComponent {
public:
    DebugChart(const std::string& label, size_t max_values = 100);
    
    void add_value(float value);
    void clear_values();
    void render() override;
    
    void set_range(float min_val, float max_val) { 
        min_value_ = min_val; 
        max_value_ = max_val; 
        auto_range_ = false;
    }
    void set_auto_range(bool auto_range) { auto_range_ = auto_range; }

private:
    std::string label_;
    std::vector<float> values_;
    size_t max_values_;
    float min_value_ = 0.0f;
    float max_value_ = 1.0f;
    bool auto_range_ = true;
};

/**
 * 性能监控组件
 */
class DebugPerformanceMonitor : public DebugUIComponent {
public:
    DebugPerformanceMonitor();
    
    void add_frame_time(float frame_time_ms);
    void add_custom_metric(const std::string& name, float value);
    void render() override;

private:
    DebugChart frame_time_chart_;
    std::unordered_map<std::string, DebugChart> custom_charts_;
    float avg_frame_time_ = 0.0f;
    float min_frame_time_ = 999.0f;
    float max_frame_time_ = 0.0f;
    size_t frame_count_ = 0;
};

/**
 * 属性检查器组件
 */
class DebugPropertyInspector : public DebugUIComponent {
public:
    DebugPropertyInspector(const std::string& title = "Properties");
    
    // 添加各种类型的属性
    void add_float_property(const std::string& name, float* value, float min = 0.0f, float max = 1.0f);
    void add_int_property(const std::string& name, int* value, int min = 0, int max = 100);
    void add_bool_property(const std::string& name, bool* value);
    void add_string_property(const std::string& name, std::string* value);
    void add_vector3_property(const std::string& name, Vector3* value);
    void add_color_property(const std::string& name, ColorExtended* value);
    
    // 添加按钮
    void add_button(const std::string& name, std::function<void()> callback);
    
    void render() override;
    void clear_properties();

private:
    struct Property {
        enum Type { FLOAT, INT, BOOL, STRING, VECTOR3, COLOR, BUTTON };
        
        std::string name;
        Type type;
        void* data_ptr;
        float min_val = 0.0f;
        float max_val = 1.0f;
        std::function<void()> callback;
        
        Property(const std::string& n, Type t, void* ptr) 
            : name(n), type(t), data_ptr(ptr) {}
        Property(const std::string& n, std::function<void()> cb)
            : name(n), type(BUTTON), data_ptr(nullptr), callback(cb) {}
    };
    
    std::string title_;
    std::vector<Property> properties_;
};

/**
 * 日志查看器组件
 */
class DebugLogViewer : public DebugUIComponent {
public:
    enum LogLevel {
        LOG_DEBUG = 0,
        LOG_INFO,
        LOG_WARNING, 
        LOG_ERROR
    };
    
    DebugLogViewer(size_t max_entries = 1000);
    
    void add_log(LogLevel level, const std::string& message);
    void clear_logs();
    void render() override;
    
    void set_filter_level(LogLevel level) { filter_level_ = level; }
    void set_auto_scroll(bool auto_scroll) { auto_scroll_ = auto_scroll; }

private:
    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string timestamp;
    };
    
    std::vector<LogEntry> log_entries_;
    size_t max_entries_;
    LogLevel filter_level_ = LOG_DEBUG;
    bool auto_scroll_ = true;
    char search_buffer_[256] = "";
    
    const char* get_level_name(LogLevel level) const;
    ImVec4 get_level_color(LogLevel level) const;
};

// ==============================================================================
// 内置调试窗口
// ==============================================================================

/**
 * 系统信息窗口
 */
class SystemInfoWindow : public DebugWindow {
public:
    SystemInfoWindow();
    void render() override;

private:
    void update_system_info();
    
    std::string platform_info_;
    std::string memory_info_;
    float cpu_usage_ = 0.0f;
    bool info_updated_ = false;
};

/**
 * 性能监控窗口  
 */
class PerformanceWindow : public DebugWindow {
public:
    PerformanceWindow();
    void render() override;
    
    void update_performance_data(float frame_time_ms);

private:
    DebugPerformanceMonitor performance_monitor_;
    DebugChart render_time_chart_;
    bool show_detailed_stats_ = false;
};

/**
 * 渲染统计窗口
 */
class RenderStatsWindow : public DebugWindow {
public:
    RenderStatsWindow();
    void render() override;

private:
    void update_render_stats();
    
    struct RenderStats {
        size_t draw_calls = 0;
        size_t vertices = 0;
        size_t triangles = 0;
        size_t texture_memory_mb = 0;
        float gpu_time_ms = 0.0f;
    } render_stats_;
};

/**
 * ImGui演示窗口封装
 */
class ImGuiDemoWindow : public DebugWindow {
public:
    ImGuiDemoWindow();
    void render() override;
};

} // namespace debug
} // namespace portal_core

#else // PORTAL_DEBUG_GUI_ENABLED

// 当未启用调试GUI时的空实现
namespace portal_core {
namespace debug {

class DebugGUISystem {
public:
    static DebugGUISystem& instance() {
        static DebugGUISystem inst;
        return inst;
    }
    
    bool initialize() { return true; }
    void shutdown() {}
    void update(float) {}
    void render() {}
    void flush_to_unified_renderer() {}
    bool is_initialized() const { return false; }
    bool is_enabled() const { return false; }
    void set_enabled(bool) {}
};

} // namespace debug  
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED
