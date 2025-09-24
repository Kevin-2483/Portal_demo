#include "debug_gui_system.h"

#ifdef PORTAL_DEBUG_GUI_ENABLED

#include "../render/unified_render_manager.h"
#include "../render/unified_debug_draw.h"
#include "../render/unified_render_types.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include "portal_debug_logging.h"

namespace portal_core {
namespace debug {

// ==============================================================================
// DebugGUISystem 实现
// ==============================================================================

DebugGUISystem& DebugGUISystem::instance() {
    static DebugGUISystem instance;
    return instance;
}

bool DebugGUISystem::initialize(const std::string& font_path) {
    if (initialized_) {
        PORTAL_DEBUG_LOG("DebugGUISystem: Already initialized");
        return true;
    }
    
    PORTAL_DEBUG_LOG("DebugGUISystem: Initializing...");
    
    if (!initialize_imgui(font_path)) {
        PORTAL_DEBUG_ERROR("DebugGUISystem: Failed to initialize ImGui");
        return false;
    }
    
    setup_imgui_style();
    
    // 阶段2不再创建预设窗口，保持纯粹的基础设施
    // 具体窗口由阶段3的IDebuggable系统按需创建
    
    initialized_ = true;
    PORTAL_DEBUG_LOG("DebugGUISystem: Initialization completed");
    return true;
}

void DebugGUISystem::shutdown() {
    if (!initialized_) return;
    
    PORTAL_DEBUG_LOG("DebugGUISystem: Shutting down...");
    
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
    PORTAL_DEBUG_LOG("DebugGUISystem: Shutdown completed");
}

bool DebugGUISystem::initialize_imgui(const std::string& font_path) {
    // 创建ImGui上下文
    imgui_context_ = ImGui::CreateContext();
    if (!imgui_context_) {
        PORTAL_DEBUG_ERROR("DebugGUISystem: Failed to create ImGui context");
        return false;
    }
    
    ImGui::SetCurrentContext(imgui_context_);
    
    // 配置ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 注意：当前 ImGui 版本可能不支持 docking 功能
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // 设置后端标志，表明我们有纹理支持
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    
    // 设置字体 - 添加中文字体支持
    ImFontConfig font_config;
    font_config.MergeMode = false;
    
    // 添加默认字体
    io.Fonts->AddFontDefault(&font_config);
    
    // 尝试加载中文字体
    std::string chinese_font_path = font_path.empty() ? "src/assets/fonts/SourceHanSerifSC-Regular.otf" : font_path;
    if (std::ifstream(chinese_font_path).good()) {
        // 配置中文字体
        ImFontConfig chinese_font_config;
        chinese_font_config.MergeMode = true;  // 合并到默认字体
        chinese_font_config.FontDataOwnedByAtlas = false;  // 不让ImGui管理字体数据
        
        // 添加中文字符范围
        static const ImWchar chinese_ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x2000, 0x206F, // General Punctuation
            0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
            0x31F0, 0x31FF, // Katakana Phonetic Extensions
            0xFF00, 0xFFEF, // Half-width characters
            0x4e00, 0x9FAF, // CJK Ideograms
            0,
        };
        
        ImFont* chinese_font = io.Fonts->AddFontFromFileTTF(
            chinese_font_path.c_str(), 
            16.0f, 
            &chinese_font_config, 
            chinese_ranges
        );
        
        if (chinese_font) {
            PORTAL_DEBUG_LOG("DebugGUISystem: Chinese font loaded successfully from " << chinese_font_path);
        } else {
            PORTAL_DEBUG_LOG("DebugGUISystem: Failed to load Chinese font, using default font only");
        }
    } else {
        PORTAL_DEBUG_LOG("DebugGUISystem: Chinese font file not found at " << chinese_font_path << ", using default font only");
    }
    
    // 构建字体图集 - 这是关键步骤！
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    // 设置字体纹理ID（这里使用一个虚拟ID，实际渲染时会处理）
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1);
    
    PORTAL_DEBUG_LOG("DebugGUISystem: ImGui context created successfully");
    PORTAL_DEBUG_LOG("DebugGUISystem: Font atlas built - size: " << width << "x" << height);
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
    
    PORTAL_DEBUG_LOG("DebugGUISystem: ImGui style configured");
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
    
    // 获取统一渲染管理器
    auto& render_manager = portal_core::render::UnifiedRenderManager::instance();
    
    // 遍历所有命令列表
    for (int cmd_list_idx = 0; cmd_list_idx < draw_data->CmdListsCount; cmd_list_idx++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[cmd_list_idx];
        
        // 转换顶点数据
        std::vector<portal_core::render::ImGuiVertex> vertices;
        vertices.reserve(cmd_list->VtxBuffer.Size);
        
        for (int vtx_idx = 0; vtx_idx < cmd_list->VtxBuffer.Size; vtx_idx++) {
            const ImDrawVert& imgui_vtx = cmd_list->VtxBuffer[vtx_idx];
            portal_core::render::ImGuiVertex vertex;
            vertex.pos = portal_core::Vector2(imgui_vtx.pos.x, imgui_vtx.pos.y);
            vertex.uv = portal_core::Vector2(imgui_vtx.uv.x, imgui_vtx.uv.y);
            vertex.col = imgui_vtx.col;
            vertices.push_back(vertex);
        }
        
        // 处理每个绘制命令
        uint32_t idx_offset = 0;
        for (int cmd_idx = 0; cmd_idx < cmd_list->CmdBuffer.Size; cmd_idx++) {
            const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_idx];
            
            // 跳过用户回调命令
            if (pcmd.UserCallback != nullptr) {
                idx_offset += pcmd.ElemCount;
                continue;
            }
            
            // 计算需要的顶点和索引数据大小
            size_t vertex_data_size = vertices.size() * sizeof(portal_core::render::ImGuiVertex);
            size_t index_data_size = pcmd.ElemCount * sizeof(ImDrawIdx);
            size_t total_data_size = sizeof(portal_core::render::ImGuiMeshData) + vertex_data_size + index_data_size;
            
            // 创建包含所有数据的缓冲区 - 使用静态存储确保生命周期
            static std::vector<std::unique_ptr<uint8_t[]>> static_buffers;
            auto data_buffer = std::make_unique<uint8_t[]>(total_data_size);
            uint8_t* current_ptr = data_buffer.get();
            
            // 在缓冲区开头放置ImGuiMeshData
            portal_core::render::ImGuiMeshData* mesh_data = 
                reinterpret_cast<portal_core::render::ImGuiMeshData*>(current_ptr);
            current_ptr += sizeof(portal_core::render::ImGuiMeshData);
            
            // 拷贝顶点数据
            portal_core::render::ImGuiVertex* vertex_data = 
                reinterpret_cast<portal_core::render::ImGuiVertex*>(current_ptr);
            std::memcpy(vertex_data, vertices.data(), vertex_data_size);
            current_ptr += vertex_data_size;
            
            // 拷贝索引数据
            ImDrawIdx* index_data = reinterpret_cast<ImDrawIdx*>(current_ptr);
            std::memcpy(index_data, &cmd_list->IdxBuffer[idx_offset], index_data_size);
            
            // 设置mesh_data的字段，指向缓冲区内的数据
            mesh_data->vertices = vertex_data;
            mesh_data->indices = index_data;
            mesh_data->vertex_count = static_cast<uint32_t>(vertices.size());
            mesh_data->index_count = pcmd.ElemCount;
            mesh_data->texture_id = pcmd.GetTexID();
            mesh_data->use_clipping = false;
            
            // 设置裁剪区域
            if (pcmd.ClipRect.x < pcmd.ClipRect.z && pcmd.ClipRect.y < pcmd.ClipRect.w) {
                mesh_data->use_clipping = true;
                mesh_data->clip_rect_min = portal_core::Vector2(pcmd.ClipRect.x, pcmd.ClipRect.y);
                mesh_data->clip_rect_max = portal_core::Vector2(pcmd.ClipRect.z, pcmd.ClipRect.w);
            }
            
            // 创建渲染命令，直接传递数据缓冲区
            portal_core::render::UnifiedRenderCommand command;
            command.type = portal_core::render::RenderCommandType::DRAW_IMGUI_VERTICES;
            command.data = data_buffer.get();
            command.data_size = total_data_size;
            command.layer = static_cast<uint32_t>(portal_core::render::RenderLayer::UI_OVERLAY);
            command.flags = portal_core::render::RENDER_FLAG_ALPHA_BLEND | portal_core::render::RENDER_FLAG_ONE_FRAME;
            command.duration = -1.0f;  // 使用负值表示不基于时间清理，而是基于帧标志
            
            // 将缓冲区移动到静态存储中，确保生命周期
            static_buffers.push_back(std::move(data_buffer));
            
            // 提交命令（UnifiedRenderManager会创建数据的深拷贝）
            render_manager.submit_command(command);
            
            idx_offset += pcmd.ElemCount;
        }
    }
}

void DebugGUISystem::register_window(std::unique_ptr<DebugWindow> window) {
    if (!window) return;
    
    const std::string& id = window->get_id();
    
    // 检查是否已存在
    auto it = window_map_.find(id);
    if (it != window_map_.end()) {
        PORTAL_DEBUG_ERROR("DebugGUISystem: Window with ID '" << id << "' already exists");
        return;
    }
    
    DebugWindow* window_ptr = window.get();
    windows_.push_back(std::move(window));
    window_map_[id] = window_ptr;
    

}

void DebugGUISystem::unregister_window(const std::string& window_id) {
    auto it = window_map_.find(window_id);
    if (it == window_map_.end()) {
        PORTAL_DEBUG_ERROR("DebugGUISystem: Window '" << window_id << "' not found");
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

}

DebugWindow* DebugGUISystem::find_window(const std::string& window_id) {
    auto it = window_map_.find(window_id);
    return (it != window_map_.end()) ? it->second : nullptr;
}

void DebugGUISystem::print_stats() const {
    // Statistics available but not logged to reduce console output
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
// 内置窗口实现已移除 - 保持纯净的调试接口
// ==============================================================================

} // namespace debug
} // namespace portal_core

#endif // PORTAL_DEBUG_GUI_ENABLED
