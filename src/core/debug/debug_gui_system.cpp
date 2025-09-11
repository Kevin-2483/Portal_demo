#include "debug_gui_system.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "../render/unified_render_manager.h"
#include "../render/unified_debug_draw.h"
#include "../render/unified_render_types.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace portal_core {
namespace debug {

// ==============================================================================
// DebugGUISystem 实现
// ==============================================================================

DebugGUISystem& DebugGUISystem::instance() {
    static DebugGUISystem instance;
    return instance;
}

bool DebugGUISystem::initialize() {
    if (initialized_) {
        std::cout << "DebugGUISystem: Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "DebugGUISystem: Initializing..." << std::endl;
    
    if (!initialize_imgui()) {
        std::cerr << "DebugGUISystem: Failed to initialize ImGui" << std::endl;
        return false;
    }
    
    setup_imgui_style();
    
    // 阶段2不再创建预设窗口，保持纯粹的基础设施
    // 具体窗口由阶段3的IDebuggable系统按需创建
    
    initialized_ = true;
    std::cout << "DebugGUISystem: Initialization completed" << std::endl;
    return true;
}

void DebugGUISystem::shutdown() {
    if (!initialized_) return;
    
    std::cout << "DebugGUISystem: Shutting down..." << std::endl;
    
    // 清理调试对象注册表（阶段3集成）
#ifdef PORTAL_DEBUG_ENABLED
    auto& registry = DebuggableRegistry::instance();
    registry.clear_all();
#endif
    
    // 清理窗口
    windows_.clear();
    window_map_.clear();
    
    // 清理ImGui
    if (imgui_context_) {
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }
    
    initialized_ = false;
    std::cout << "DebugGUISystem: Shutdown completed" << std::endl;
}

bool DebugGUISystem::initialize_imgui() {
    // 创建ImGui上下文
    imgui_context_ = ImGui::CreateContext();
    if (!imgui_context_) {
        std::cerr << "DebugGUISystem: Failed to create ImGui context" << std::endl;
        return false;
    }
    
    ImGui::SetCurrentContext(imgui_context_);
    
    // 配置ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 注意：当前 ImGui 版本可能不支持 docking 功能
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // 设置字体（可选，使用默认字体）
    io.Fonts->AddFontDefault();
    
    std::cout << "DebugGUISystem: ImGui context created successfully" << std::endl;
    return true;
}

void DebugGUISystem::setup_imgui_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 设置深色主题
    ImGui::StyleColorsDark(&style);
    
    // 自定义样式调整
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    
    // 自定义颜色
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.15f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    
    std::cout << "DebugGUISystem: ImGui style configured" << std::endl;
}

void DebugGUISystem::update(float delta_time) {
    if (!initialized_ || !enabled_) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ImGui::SetCurrentContext(imgui_context_);
    
    // 更新ImGui帧
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = delta_time;
    io.DisplaySize = ImVec2(1920, 1080); // 默认分辨率，应该从渲染系统获取
    
    ImGui::NewFrame();
    
    // 更新统计信息
    stats_.window_count = windows_.size();
    stats_.visible_window_count = 0;
    
    for (auto& window : windows_) {
        if (window->is_visible()) {
            stats_.visible_window_count++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats_.frame_time_ms = duration.count() / 1000.0f;
    
    frame_timer_ += delta_time;
}

void DebugGUISystem::render() {
    if (!initialized_ || !enabled_) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ImGui::SetCurrentContext(imgui_context_);
    
    // 渲染所有注册的窗口
    for (auto& window : windows_) {
        if (window->is_visible()) {
            window->render();
        }
    }
    
    // 渲染所有IDebuggable对象的GUI（阶段3集成）
#ifdef PORTAL_DEBUG_ENABLED
    auto& registry = DebuggableRegistry::instance();
    registry.render_all_gui();
#endif
    
    // 主菜单栏（集成窗口管理和调试对象管理）
    static bool show_debuggable_list = false;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("调试")) {
            // 注册的窗口控制
            if (!windows_.empty()) {
                ImGui::Text("调试窗口:");
                for (auto& window : windows_) {
                    bool visible = window->is_visible();
                    if (ImGui::MenuItem(window->get_title().c_str(), nullptr, &visible)) {
                        window->set_visible(visible);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("关闭所有窗口")) {
                    for (auto& window : windows_) {
                        window->set_visible(false);
                    }
                }
                if (ImGui::MenuItem("显示所有窗口")) {
                    for (auto& window : windows_) {
                        window->set_visible(true);
                    }
                }
                ImGui::Separator();
            }
            
            // 调试对象管理
#ifdef PORTAL_DEBUG_ENABLED
            ImGui::MenuItem("调试对象列表", nullptr, &show_debuggable_list);
#endif
            
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
#ifdef PORTAL_DEBUG_ENABLED
    if (show_debuggable_list) {
        registry.render_debuggable_list();
    }
#endif
    
    ImGui::Render();;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats_.render_time_ms = duration.count() / 1000.0f;
}

void DebugGUISystem::flush_to_unified_renderer() {
    if (!initialized_ || !enabled_) return;
    
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (!draw_data || draw_data->CmdListsCount == 0) return;
    
    // 将ImGui绘制数据转换为统一渲染命令
    for (int cmd_list_idx = 0; cmd_list_idx < draw_data->CmdListsCount; cmd_list_idx++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[cmd_list_idx];
        
        for (int cmd_idx = 0; cmd_idx < cmd_list->CmdBuffer.Size; cmd_idx++) {
            const ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_idx];
            
            if (cmd->UserCallback) {
                // 处理用户回调
                cmd->UserCallback(cmd_list, cmd);
            } else {
                // 转换为UI渲染命令
                // 这里简化处理，实际应该转换顶点数据和纹理
                Vector2 pos(cmd->ClipRect.x, cmd->ClipRect.y);
                Vector2 size(cmd->ClipRect.z - cmd->ClipRect.x, cmd->ClipRect.w - cmd->ClipRect.y);
                
                portal_core::debug::UnifiedDebugDraw::draw_ui_rect(
                    pos, size, 
                    render::Color4f(1.0f, 1.0f, 1.0f, 0.1f), // 半透明白色
                    true, 1.0f
                );
            }
        }
    }
}

void DebugGUISystem::register_window(std::unique_ptr<DebugWindow> window) {
    if (!window) return;
    
    const std::string& id = window->get_id();
    
    // 检查是否已存在
    auto it = window_map_.find(id);
    if (it != window_map_.end()) {
        std::cerr << "DebugGUISystem: Window with ID '" << id << "' already exists" << std::endl;
        return;
    }
    
    DebugWindow* window_ptr = window.get();
    windows_.push_back(std::move(window));
    window_map_[id] = window_ptr;
    
    std::cout << "DebugGUISystem: Registered window '" << id << "'" << std::endl;
}

void DebugGUISystem::unregister_window(const std::string& window_id) {
    auto it = window_map_.find(window_id);
    if (it == window_map_.end()) {
        std::cerr << "DebugGUISystem: Window '" << window_id << "' not found" << std::endl;
        return;
    }
    
    // 从vector中移除
    auto vec_it = std::find_if(windows_.begin(), windows_.end(),
        [&window_id](const std::unique_ptr<DebugWindow>& window) {
            return window->get_id() == window_id;
        });
    
    if (vec_it != windows_.end()) {
        windows_.erase(vec_it);
    }
    
    window_map_.erase(it);
    std::cout << "DebugGUISystem: Unregistered window '" << window_id << "'" << std::endl;
}

DebugWindow* DebugGUISystem::find_window(const std::string& window_id) {
    auto it = window_map_.find(window_id);
    return (it != window_map_.end()) ? it->second : nullptr;
}

void DebugGUISystem::print_stats() const {
    std::cout << "=== DebugGUISystem Statistics ===" << std::endl;
    std::cout << "Windows: " << stats_.window_count 
              << " (Visible: " << stats_.visible_window_count << ")" << std::endl;
    std::cout << "Frame time: " << std::fixed << std::setprecision(3) 
              << stats_.frame_time_ms << "ms" << std::endl;
    std::cout << "Render time: " << std::fixed << std::setprecision(3) 
              << stats_.render_time_ms << "ms" << std::endl;
}

// ==============================================================================
// DebugWindow 实现
// ==============================================================================

DebugWindow::DebugWindow(const std::string& id, const std::string& title)
    : window_id_(id), title_(title) {
}

void DebugWindow::begin_window() {
    if (position_set_) {
        ImGui::SetNextWindowPos(ImVec2(position_.x, position_.y), ImGuiCond_FirstUseEver);
    }
    if (size_set_) {
        ImGui::SetNextWindowSize(ImVec2(size_.x, size_.y), ImGuiCond_FirstUseEver);
    }
    
    ImGui::Begin(title_.c_str(), &visible_, window_flags_);
}

void DebugWindow::end_window() {
    // 更新窗口位置和大小
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    position_ = Vector2(pos.x, pos.y);
    size_ = Vector2(size.x, size.y);
    
    ImGui::End();
}

// ==============================================================================
// DebugChart 实现
// ==============================================================================

DebugChart::DebugChart(const std::string& label, size_t max_values)
    : label_(label), max_values_(max_values) {
    values_.reserve(max_values_);
}

void DebugChart::add_value(float value) {
    values_.push_back(value);
    
    if (values_.size() > max_values_) {
        values_.erase(values_.begin());
    }
    
    if (auto_range_ && !values_.empty()) {
        auto minmax = std::minmax_element(values_.begin(), values_.end());
        min_value_ = *minmax.first;
        max_value_ = *minmax.second;
        
        // 添加一些边距
        float range = max_value_ - min_value_;
        if (range > 0) {
            min_value_ -= range * 0.1f;
            max_value_ += range * 0.1f;
        }
    }
}

void DebugChart::clear_values() {
    values_.clear();
}

void DebugChart::render() {
    if (!enabled_ || values_.empty()) return;
    
    float scale_min = min_value_;
    float scale_max = max_value_;
    
    ImGui::PlotLines(
        label_.c_str(),
        values_.data(),
        static_cast<int>(values_.size()),
        0,
        nullptr,
        scale_min,
        scale_max,
        ImVec2(0, 80)
    );
    
    // 显示当前值和统计信息
    if (!values_.empty()) {
        ImGui::Text("当前: %.3f", values_.back());
        ImGui::SameLine();
        ImGui::Text("范围: [%.3f, %.3f]", scale_min, scale_max);
    }
}

// ==============================================================================
// DebugPerformanceMonitor 实现
// ==============================================================================

DebugPerformanceMonitor::DebugPerformanceMonitor()
    : frame_time_chart_("帧时间 (ms)", 120) {
    frame_time_chart_.set_range(0.0f, 33.33f); // 30-60 FPS 范围
}

void DebugPerformanceMonitor::add_frame_time(float frame_time_ms) {
    frame_time_chart_.add_value(frame_time_ms);
    
    // 更新统计信息
    frame_count_++;
    avg_frame_time_ = (avg_frame_time_ * (frame_count_ - 1) + frame_time_ms) / frame_count_;
    min_frame_time_ = std::min(min_frame_time_, frame_time_ms);
    max_frame_time_ = std::max(max_frame_time_, frame_time_ms);
}

void DebugPerformanceMonitor::add_custom_metric(const std::string& name, float value) {
    auto it = custom_charts_.find(name);
    if (it == custom_charts_.end()) {
        custom_charts_.emplace(name, DebugChart(name, 60));
        it = custom_charts_.find(name);
    }
    it->second.add_value(value);
}

void DebugPerformanceMonitor::render() {
    if (!enabled_) return;
    
    ImGui::Text("性能统计");
    ImGui::Separator();
    
    // 显示帧时间统计
    ImGui::Text("平均帧时间: %.3f ms (%.1f FPS)", avg_frame_time_, 1000.0f / avg_frame_time_);
    ImGui::Text("最小帧时间: %.3f ms", min_frame_time_);
    ImGui::Text("最大帧时间: %.3f ms", max_frame_time_);
    
    // 显示帧时间图表
    frame_time_chart_.render();
    
    // 显示自定义指标
    if (!custom_charts_.empty()) {
        ImGui::Separator();
        ImGui::Text("自定义指标");
        for (auto& [name, chart] : custom_charts_) {
            chart.render();
        }
    }
}

// ==============================================================================
// DebugPropertyInspector 实现
// ==============================================================================

DebugPropertyInspector::DebugPropertyInspector(const std::string& title)
    : title_(title) {
}

void DebugPropertyInspector::add_float_property(const std::string& name, float* value, float min, float max) {
    Property prop(name, Property::FLOAT, value);
    prop.min_val = min;
    prop.max_val = max;
    properties_.push_back(prop);
}

void DebugPropertyInspector::add_int_property(const std::string& name, int* value, int min, int max) {
    Property prop(name, Property::INT, value);
    prop.min_val = static_cast<float>(min);
    prop.max_val = static_cast<float>(max);
    properties_.push_back(prop);
}

void DebugPropertyInspector::add_bool_property(const std::string& name, bool* value) {
    properties_.emplace_back(name, Property::BOOL, value);
}

void DebugPropertyInspector::add_string_property(const std::string& name, std::string* value) {
    properties_.emplace_back(name, Property::STRING, value);
}

void DebugPropertyInspector::add_vector3_property(const std::string& name, Vector3* value) {
    properties_.emplace_back(name, Property::VECTOR3, value);
}

void DebugPropertyInspector::add_color_property(const std::string& name, ColorExtended* value) {
    properties_.emplace_back(name, Property::COLOR, value);
}

void DebugPropertyInspector::add_button(const std::string& name, std::function<void()> callback) {
    properties_.emplace_back(name, callback);
}

void DebugPropertyInspector::render() {
    if (!enabled_) return;
    
    ImGui::Text("%s", title_.c_str());
    ImGui::Separator();
    
    for (auto& prop : properties_) {
        switch (prop.type) {
            case Property::FLOAT: {
                float* value = static_cast<float*>(prop.data_ptr);
                ImGui::SliderFloat(prop.name.c_str(), value, prop.min_val, prop.max_val);
                break;
            }
            case Property::INT: {
                int* value = static_cast<int*>(prop.data_ptr);
                int min_val = static_cast<int>(prop.min_val);
                int max_val = static_cast<int>(prop.max_val);
                ImGui::SliderInt(prop.name.c_str(), value, min_val, max_val);
                break;
            }
            case Property::BOOL: {
                bool* value = static_cast<bool*>(prop.data_ptr);
                ImGui::Checkbox(prop.name.c_str(), value);
                break;
            }
            case Property::STRING: {
                std::string* value = static_cast<std::string*>(prop.data_ptr);
                char buffer[256];
                strncpy(buffer, value->c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                if (ImGui::InputText(prop.name.c_str(), buffer, sizeof(buffer))) {
                    *value = buffer;
                }
                break;
            }
            case Property::VECTOR3: {
                Vector3* value = static_cast<Vector3*>(prop.data_ptr);
                float vec[3] = { value->GetX(), value->GetY(), value->GetZ() };
                if (ImGui::DragFloat3(prop.name.c_str(), vec, 0.1f)) {
                    *value = Vector3(vec[0], vec[1], vec[2]);
                }
                break;
            }
            case Property::COLOR: {
                ColorExtended* value = static_cast<ColorExtended*>(prop.data_ptr);
                float color[4] = { value->r, value->g, value->b, value->a };
                if (ImGui::ColorEdit4(prop.name.c_str(), color)) {
                    value->r = color[0];
                    value->g = color[1];
                    value->b = color[2];
                    value->a = color[3];
                }
                break;
            }
            case Property::BUTTON: {
                if (ImGui::Button(prop.name.c_str()) && prop.callback) {
                    prop.callback();
                }
                break;
            }
        }
    }
}

void DebugPropertyInspector::clear_properties() {
    properties_.clear();
}

// ==============================================================================
// DebugLogViewer 实现
// ==============================================================================

DebugLogViewer::DebugLogViewer(size_t max_entries) : max_entries_(max_entries) {
}

void DebugLogViewer::add_log(LogLevel level, const std::string& message) {
    // 创建时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.timestamp = oss.str();
    
    log_entries_.push_back(entry);
    
    // 限制日志条目数量
    if (log_entries_.size() > max_entries_) {
        log_entries_.erase(log_entries_.begin());
    }
}

void DebugLogViewer::clear_logs() {
    log_entries_.clear();
}

void DebugLogViewer::render() {
    if (!enabled_) return;
    
    // 控制栏
    ImGui::AlignTextToFramePadding();
    ImGui::Text("过滤等级:");
    ImGui::SameLine();
    
    const char* level_names[] = { "DEBUG", "INFO", "WARNING", "ERROR" };
    int current_level = static_cast<int>(filter_level_);
    if (ImGui::Combo("##FilterLevel", &current_level, level_names, 4)) {
        filter_level_ = static_cast<LogLevel>(current_level);
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("自动滚动", &auto_scroll_);
    
    ImGui::SameLine();
    if (ImGui::Button("清空")) {
        clear_logs();
    }
    
    // 搜索框
    ImGui::Text("搜索:");
    ImGui::SameLine();
    ImGui::InputText("##Search", search_buffer_, sizeof(search_buffer_));
    
    ImGui::Separator();
    
    // 日志列表
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    if (ImGui::BeginTable("LogTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("等级", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("消息", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto& entry : log_entries_) {
            // 过滤等级
            if (entry.level < filter_level_) continue;
            
            // 搜索过滤
            std::string search_str(search_buffer_);
            if (!search_str.empty() && 
                entry.message.find(search_str) == std::string::npos) {
                continue;
            }
            
            ImGui::TableNextRow();
            
            // 时间戳
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", entry.timestamp.c_str());
            
            // 等级
            ImGui::TableSetColumnIndex(1);
            ImVec4 level_color = get_level_color(entry.level);
            ImGui::TextColored(level_color, "%s", get_level_name(entry.level));
            
            // 消息
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entry.message.c_str());
        }
        
        ImGui::EndTable();
    }
    
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
}

const char* DebugLogViewer::get_level_name(LogLevel level) const {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

ImVec4 DebugLogViewer::get_level_color(LogLevel level) const {
    switch (level) {
        case LOG_DEBUG: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        case LOG_INFO: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        case LOG_WARNING: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        case LOG_ERROR: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

// ==============================================================================
// 内置窗口实现
// ==============================================================================

SystemInfoWindow::SystemInfoWindow() 
    : DebugWindow("system_info", "系统信息") {
    set_size(Vector2(400, 300));
    update_system_info();
}

void SystemInfoWindow::render() {
    begin_window();
    
    if (!info_updated_ || ImGui::Button("刷新")) {
        update_system_info();
    }
    
    ImGui::Separator();
    
    ImGui::Text("平台信息:");
    ImGui::TextWrapped("%s", platform_info_.c_str());
    
    ImGui::Separator();
    
    ImGui::Text("内存信息:");
    ImGui::TextWrapped("%s", memory_info_.c_str());
    
    ImGui::Separator();
    
    ImGui::Text("CPU 使用率: %.1f%%", cpu_usage_);
    
    end_window();
}

void SystemInfoWindow::update_system_info() {
    // 简化的系统信息收集
    platform_info_ = "Portal Demo Debug System\n";
    platform_info_ += "编译时间: " __DATE__ " " __TIME__ "\n";
    
#ifdef _WIN32
    platform_info_ += "平台: Windows\n";
#elif defined(__APPLE__)
    platform_info_ += "平台: macOS\n";
#elif defined(__linux__)
    platform_info_ += "平台: Linux\n";
#else
    platform_info_ += "平台: Unknown\n";
#endif
    
    memory_info_ = "内存信息暂不可用";
    cpu_usage_ = 0.0f; // 简化，实际需要系统调用
    
    info_updated_ = true;
}

PerformanceWindow::PerformanceWindow() 
    : DebugWindow("performance", "性能监控"),
      render_time_chart_("渲染时间 (ms)", 120) {
    set_size(Vector2(500, 400));
}

void PerformanceWindow::render() {
    begin_window();
    
    performance_monitor_.render();
    
    ImGui::Separator();
    
    // 显示调试GUI系统自身的性能
    const auto& gui_stats = DebugGUISystem::instance().get_stats();
    ImGui::Text("GUI系统性能:");
    ImGui::Text("窗口数量: %zu (%zu 可见)", gui_stats.window_count, gui_stats.visible_window_count);
    ImGui::Text("GUI帧时间: %.3f ms", gui_stats.frame_time_ms);
    ImGui::Text("GUI渲染时间: %.3f ms", gui_stats.render_time_ms);
    
    ImGui::Checkbox("显示详细统计", &show_detailed_stats_);
    
    if (show_detailed_stats_) {
        ImGui::Separator();
        render_time_chart_.render();
    }
    
    end_window();
}

void PerformanceWindow::update_performance_data(float frame_time_ms) {
    performance_monitor_.add_frame_time(frame_time_ms);
    render_time_chart_.add_value(frame_time_ms);
}

RenderStatsWindow::RenderStatsWindow() 
    : DebugWindow("render_stats", "渲染统计") {
    set_size(Vector2(350, 250));
}

void RenderStatsWindow::render() {
    begin_window();
    
    update_render_stats();
    
    ImGui::Text("渲染统计信息");
    ImGui::Separator();
    
    ImGui::Text("绘制调用: %zu", render_stats_.draw_calls);
    ImGui::Text("顶点数: %zu", render_stats_.vertices);
    ImGui::Text("三角形数: %zu", render_stats_.triangles);
    ImGui::Text("纹理内存: %zu MB", render_stats_.texture_memory_mb);
    ImGui::Text("GPU 时间: %.3f ms", render_stats_.gpu_time_ms);
    
    ImGui::Separator();
    
    // 显示统一渲染系统统计
    auto unified_stats = portal_core::debug::UnifiedDebugDraw::get_stats();
    ImGui::Text("统一渲染系统:");
    ImGui::Text("总命令数: %zu", unified_stats.total_commands);
    ImGui::Text("3D命令数: %zu", unified_stats.commands_3d);
    ImGui::Text("UI命令数: %zu", unified_stats.commands_ui);
    ImGui::Text("自定义命令数: %zu", unified_stats.commands_custom);
    
    end_window();
}

void RenderStatsWindow::update_render_stats() {
    // 从统一渲染管理器获取统计信息
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    auto stats = render_manager.get_render_stats();
    
    render_stats_.draw_calls = stats.total_commands;
    render_stats_.vertices = 0; // 需要从实际渲染器获取
    render_stats_.triangles = 0;
    render_stats_.texture_memory_mb = 0;
    render_stats_.gpu_time_ms = stats.frame_time_ms;
}

ImGuiDemoWindow::ImGuiDemoWindow() 
    : DebugWindow("imgui_demo", "ImGui 演示") {
    set_size(Vector2(600, 500));
    set_visible(false); // 默认隐藏
}

void ImGuiDemoWindow::render() {
    if (should_render()) {
        ImGui::ShowDemoWindow(&visible_);
    }
}

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED
