# 统一可视化调试平台实现路线

## 设计原则

1. **分层架构**: 核心层(C++)、桥接层(Godot)、接口层完全解耦
2. **统一接口**: 所有调试绘制通过统一接口，无论3D还是UI
3. **可选编译**: 除核心统一接口外，所有调试功能都可选择性编译
4. **生产就绪**: 统一接口可留在发布版本中，调试功能可完全移除

---

## 阶段0: 技术验证 ✅ **[已完成]**

**目标**: 验证统一渲染管线和桥接可行性

### 已完成组件

#### C++核心层
- `MinimalDebugDrawManager`: 最小化调试绘图管理器，收集线段数据
- `UIDebugManager`: UI调试管理器，生成ImGui风格界面元素

#### Godot桥接层  
- `MinimalDebugRenderer`: 3D世界空间线段渲染器
- `AdvancedUIRenderer`: 2D屏幕空间UI渲染器
- `DebugUIPanel`: 基础UI面板组件

#### 验证结果
- ✅ C++到Godot的完整数据流畅通
- ✅ 3D世界空间 + 2D屏幕空间同时渲染
- ✅ 复杂UI元素（窗口、按钮、图表）正常工作
- ✅ 实时数据更新和动画效果正常

### 示例代码价值
当前代码将作为：
- **技术参考**: 展示统一渲染管线实现方式
- **测试用例**: 验证后续阶段功能正确性
- **文档示例**: 为开发者提供使用范例

---

## 阶段1: 统一绘制接口和桥接 🎯 **[已完成]**

**目标**: 完成通用、生产级别的统一绘制接口，支持任何自定义渲染步骤

### 1.1 核心统一绘制接口设计

```cpp
namespace portal_core {
namespace render {

// 通用渲染命令类型
enum class RenderCommandType {
    DRAW_LINE_3D,      // 3D线段
    DRAW_MESH_3D,      // 3D网格
    DRAW_UI_RECT,      // UI矩形
    DRAW_UI_TEXT,      // UI文本
    DRAW_UI_TEXTURE,   // UI贴图
    CUSTOM_COMMAND     // 自定义命令
};

// 统一渲染命令
struct UnifiedRenderCommand {
    RenderCommandType type;
    void* data;        // 具体数据指针
    size_t data_size;  // 数据大小
    uint32_t layer;    // 渲染层级
    uint32_t flags;    // 渲染标志
};

// 统一渲染接口
class IUnifiedRenderer {
public:
    virtual ~IUnifiedRenderer() = default;
    
    // 提交渲染命令
    virtual void submit_command(const UnifiedRenderCommand& command) = 0;
    
    // 批量提交
    virtual void submit_commands(const UnifiedRenderCommand* commands, size_t count) = 0;
    
    // 清空命令队列
    virtual void clear_commands() = 0;
    
    // 获取命令统计
    virtual size_t get_command_count() const = 0;
};

}} // namespace portal_core::render
```

### 1.2 渲染命令管理器

```cpp
// 渲染命令队列管理
class UnifiedRenderManager {
private:
    std::vector<UnifiedRenderCommand> command_queue_;
    std::vector<IUnifiedRenderer*> renderers_;
    
public:
    // 注册渲染器
    void register_renderer(IUnifiedRenderer* renderer);
    void unregister_renderer(IUnifiedRenderer* renderer);
    
    // 提交命令
    void submit_command(const UnifiedRenderCommand& command);
    
    // 分发到所有渲染器
    void flush_commands();
    
    // 单例访问
    static UnifiedRenderManager& instance();
};
```

### 1.3 Godot桥接渲染器

```cpp
// Godot渲染器实现统一接口
class GodotUnifiedRenderer : public IUnifiedRenderer {
private:
    // 3D渲染组件
    Node3D* world_renderer_;
    
    // 2D渲染组件  
    Control* ui_renderer_;
    
public:
    // 实现统一接口
    void submit_command(const UnifiedRenderCommand& command) override;
    
    // 分发到对应渲染器
    void dispatch_3d_command(const UnifiedRenderCommand& command);
    void dispatch_2d_command(const UnifiedRenderCommand& command);
};
```

### 1.4 便利API层

```cpp
// 高级便利API，简化常用操作
namespace portal_core {
namespace debug {

class UnifiedDebugDraw {
public:
    // 3D绘制API
    static void draw_line(const Vector3& start, const Vector3& end, const Color& color);
    static void draw_box(const Vector3& center, const Vector3& size, const Color& color);
    static void draw_sphere(const Vector3& center, float radius, const Color& color);
    
    // UI绘制API
    static void draw_ui_rect(const Vector2& pos, const Vector2& size, const Color& color);
    static void draw_ui_text(const Vector2& pos, const std::string& text, const Color& color);
    static void draw_ui_image(const Vector2& pos, const std::string& texture_path);
    
    // 自定义命令
    static void submit_custom_command(void* data, size_t size, uint32_t type);
};

}} // namespace portal_core::debug
```

### 1.5 阶段1交付物

- ✅ 生产级统一渲染接口
- ✅ 命令队列管理系统
- ✅ Godot桥接渲染器
- ✅ 便利API层
- ✅ 完整文档和使用示例
- ✅ 性能测试和基准测试

---

## 阶段2: DebugGUISystem系统 🔄 **[已完成]**

**目标**: 基于ImGui的专业调试GUI系统，可选择性编译

### 2.1 ImGui集成架构

```cpp
#ifdef PORTAL_DEBUG_GUI_ENABLED

namespace portal_core {
namespace debug {

class DebugGUISystem {
private:
    bool enabled_;
    std::vector<std::unique_ptr<DebugWindow>> windows_;
    
public:
    // 初始化ImGui上下文
    bool initialize();
    void shutdown();
    
    // 窗口管理
    void register_window(std::unique_ptr<DebugWindow> window);
    void unregister_window(const std::string& window_id);
    
    // 渲染循环
    void update(float delta_time);
    void render();
    
    // 通过统一接口输出
    void flush_to_unified_renderer();
    
    // 单例访问
    static DebugGUISystem& instance();
};

}} // namespace portal_core::debug

#endif // PORTAL_DEBUG_GUI_ENABLED
```

### 2.2 调试窗口基类

```cpp
#ifdef PORTAL_DEBUG_GUI_ENABLED

// 调试窗口抽象基类
class DebugWindow {
protected:
    std::string window_id_;
    std::string title_;
    bool visible_;
    
public:
    DebugWindow(const std::string& id, const std::string& title);
    virtual ~DebugWindow() = default;
    
    // 渲染接口
    virtual void render() = 0;
    
    // 窗口控制
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    const std::string& get_id() const { return window_id_; }
};

#endif // PORTAL_DEBUG_GUI_ENABLED
```

### 2.3 内置调试窗口

```cpp
#ifdef PORTAL_DEBUG_GUI_ENABLED

// 性能监控窗口
class PerformanceWindow : public DebugWindow {
public:
    void render() override;
    // 显示FPS、帧时间、内存使用等
};

// 渲染统计窗口  
class RenderStatsWindow : public DebugWindow {
public:
    void render() override;
    // 显示绘制调用、顶点数、纹理使用等
};

// 日志窗口
class LogWindow : public DebugWindow {
public:
    void render() override;
    // 显示系统日志、错误信息等
};

#endif // PORTAL_DEBUG_GUI_ENABLED
```

### 2.4 阶段2交付物

- ✅ ImGui集成到构建系统
- ✅ DebugGUISystem核心框架
- ✅ 可选编译宏定义
- ✅ 内置调试窗口集合
- ✅ 与统一接口的完整集成
- ✅ 文档和使用指南

---

## 阶段3: IDebuggable接口 🔄 **[已完成]**

**目标**: 模块化调试接口，各系统可自主绘制调试信息

### 3.1 IDebuggable接口设计

```cpp
#ifdef PORTAL_DEBUG_ENABLED

namespace portal_core {
namespace debug {

// 可调试模块接口
class IDebuggable {
public:
    virtual ~IDebuggable() = default;
    
    // GUI调试绘制
    virtual void render_debug_gui() {}
    
    // 世界空间调试绘制
    virtual void render_debug_world() {}
    
    // 获取调试信息
    virtual std::string get_debug_name() const = 0;
    virtual bool is_debug_enabled() const { return true; }
    
    // 调试菜单项
    virtual void render_debug_menu() {}
};

}} // namespace portal_core::debug

#endif // PORTAL_DEBUG_ENABLED
```

### 3.2 调试注册管理器

```cpp
#ifdef PORTAL_DEBUG_ENABLED

class DebuggableRegistry {
private:
    std::vector<IDebuggable*> registered_objects_;
    std::unordered_map<std::string, IDebuggable*> named_objects_;
    
public:
    // 注册/注销
    void register_debuggable(IDebuggable* debuggable);
    void unregister_debuggable(IDebuggable* debuggable);
    
    // 批量渲染
    void render_all_gui();
    void render_all_world();
    
    // 查找
    IDebuggable* find_by_name(const std::string& name);
    
    // 单例访问
    static DebuggableRegistry& instance();
};

#endif // PORTAL_DEBUG_ENABLED
```

### 3.3 便利宏定义

```cpp
#ifdef PORTAL_DEBUG_ENABLED
    #define PORTAL_DECLARE_DEBUGGABLE() \
        void render_debug_gui() override; \
        void render_debug_world() override; \
        std::string get_debug_name() const override;
        
    #define PORTAL_REGISTER_DEBUGGABLE(obj) \
        portal_core::debug::DebuggableRegistry::instance().register_debuggable(obj)
        
    #define PORTAL_UNREGISTER_DEBUGGABLE(obj) \
        portal_core::debug::DebuggableRegistry::instance().unregister_debuggable(obj)
#else
    #define PORTAL_DECLARE_DEBUGGABLE()
    #define PORTAL_REGISTER_DEBUGGABLE(obj)
    #define PORTAL_UNREGISTER_DEBUGGABLE(obj)
#endif
```

### 3.4 阶段3交付物

- ✅ IDebuggable接口定义
- ✅ 调试注册管理系统
- ✅ 便利宏和工具函数
- ✅ 与DebugGUISystem集成
- ✅ 示例实现和最佳实践

---

## 阶段4: 现有系统集成 🔄 **[最终阶段]**

**目标**: 让现有系统和管理器实现IDebuggable，绘制调试数据

### 4.1 物理系统调试

```cpp
#ifdef PORTAL_DEBUG_ENABLED

class PhysicsSystem : public IDebuggable {
public:
    PORTAL_DECLARE_DEBUGGABLE()
    
    void render_debug_gui() override {
        if (ImGui::Begin("Physics Debug")) {
            ImGui::Text("Active Bodies: %d", active_body_count_);
            ImGui::Text("Collision Checks: %d", collision_check_count_);
            ImGui::Checkbox("Show Collision Shapes", &show_collision_shapes_);
            ImGui::Checkbox("Show Contact Points", &show_contact_points_);
        }
        ImGui::End();
    }
    
    void render_debug_world() override {
        if (show_collision_shapes_) {
            // 绘制碰撞形状轮廓
            for (auto& body : physics_bodies_) {
                UnifiedDebugDraw::draw_box(body.position, body.size, Color::GREEN);
            }
        }
        
        if (show_contact_points_) {
            // 绘制碰撞点
            for (auto& contact : contact_points_) {
                UnifiedDebugDraw::draw_sphere(contact.position, 0.1f, Color::RED);
            }
        }
    }
};

#endif // PORTAL_DEBUG_ENABLED
```

### 4.2 ECS系统调试

```cpp
#ifdef PORTAL_DEBUG_ENABLED

class ECSManager : public IDebuggable {
public:
    PORTAL_DECLARE_DEBUGGABLE()
    
    void render_debug_gui() override {
        if (ImGui::Begin("ECS Debug")) {
            ImGui::Text("Total Entities: %d", entity_count_);
            ImGui::Text("Active Systems: %d", active_system_count_);
            
            if (ImGui::TreeNode("Component Types")) {
                for (auto& [type_id, count] : component_counts_) {
                    ImGui::Text("%s: %d", type_id.name(), count);
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("System Performance")) {
                for (auto& [system_name, time] : system_times_) {
                    ImGui::Text("%s: %.3fms", system_name.c_str(), time);
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
    
    void render_debug_world() override {
        // 绘制实体位置、组件关系等
        if (show_entity_ids_) {
            for (auto& entity : entities_) {
                auto pos = entity.get_component<PositionComponent>();
                if (pos) {
                    UnifiedDebugDraw::draw_ui_text(
                        Vector2(pos->x, pos->y), 
                        "ID:" + std::to_string(entity.id()),
                        Color::WHITE
                    );
                }
            }
        }
    }
};

#endif // PORTAL_DEBUG_ENABLED
```

### 4.3 事件系统调试

```cpp
#ifdef PORTAL_DEBUG_ENABLED

class EventManager : public IDebuggable {
public:
    PORTAL_DECLARE_DEBUGGABLE()
    
    void render_debug_gui() override {
        if (ImGui::Begin("Event System Debug")) {
            ImGui::Text("Events This Frame: %d", events_this_frame_);
            ImGui::Text("Total Listeners: %d", total_listeners_);
            
            if (ImGui::TreeNode("Event Types")) {
                for (auto& [type, count] : event_type_counts_) {
                    ImGui::Text("%s: %d", type.c_str(), count);
                }
                ImGui::TreePop();
            }
            
            ImGui::Checkbox("Show Event Flow", &show_event_flow_);
        }
        ImGui::End();
    }
    
    void render_debug_world() override {
        if (show_event_flow_) {
            // 可视化事件传递路径
            for (auto& event_trace : recent_event_traces_) {
                for (size_t i = 1; i < event_trace.path.size(); ++i) {
                    UnifiedDebugDraw::draw_line(
                        event_trace.path[i-1],
                        event_trace.path[i],
                        Color::YELLOW
                    );
                }
            }
        }
    }
};

#endif // PORTAL_DEBUG_ENABLED
```

### 4.4 阶段4交付物

- ✅ 所有核心系统实现IDebuggable
- ✅ 物理、ECS、事件、渲染系统调试界面
- ✅ 性能分析和优化建议
- ✅ 完整的调试工作流文档
- ✅ 发布版本编译验证

---

## 构建系统集成

### 编译选项

```cmake
# CMake选项定义
option(PORTAL_DEBUG_ENABLED "启用调试系统" ON)
option(PORTAL_DEBUG_GUI_ENABLED "启用ImGui调试界面" ON)
option(PORTAL_DEBUG_WORLD_ENABLED "启用世界空间调试绘制" ON)
```

### 条件编译

```cpp
// 编译时配置
#ifdef PORTAL_DEBUG_ENABLED
    #define PORTAL_DEBUG_GUI_ENABLED
    #define PORTAL_DEBUG_WORLD_ENABLED
#endif

// 发布版本自动禁用
#ifdef NDEBUG
    #undef PORTAL_DEBUG_ENABLED
    #undef PORTAL_DEBUG_GUI_ENABLED
    #undef PORTAL_DEBUG_WORLD_ENABLED
#endif
```

---

## 总结

这个实现路线确保了：

1. **技术可行性**: 阶段0已验证核心技术
2. **生产就绪**: 阶段1提供稳定的统一接口
3. **功能完备**: 阶段2-4逐步添加完整调试功能
4. **可选编译**: 调试功能不影响发布版本性能
5. **模块化设计**: 各阶段独立，可按需实现

当前所有示例代码将作为技术参考和测试用例，为后续阶段提供坚实基础。

---

*最后更新: 2025年9月9日*
