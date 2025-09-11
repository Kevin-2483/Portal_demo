# 统一调试渲染桥接器使用指南

## 概述

`UnifiedDebugRenderBridge` 是项目的核心调试渲染组件，将所有调试功能（3D渲染、UI绘制、ImGui系统）整合到统一的渲染管线中。

## 架构特点

### 统一渲染流程
```
[DebugGUISystem (ImGui)] ─┐
[3D调试渲染命令]         ├─→ [UnifiedDebugRenderBridge] ─→ [UnifiedRenderManager] ─→ [GodotUnifiedRenderer] ─→ [Godot]
[UI调试命令]            ─┘
```

### 核心优势
- **单一桥接器**：避免多个生命周期管理器冲突
- **统一渲染循环**：所有调试命令在同一个 `_process()` 中处理
- **条件编译**：ImGui 功能可通过 `PORTAL_DEBUG_GUI_ENABLED` 宏控制
- **自动初始化**：支持自动设置和管理

## 使用方法

### 在 GDScript 中使用

```gdscript
# 创建统一调试渲染桥接器
var debug_bridge = UnifiedDebugRenderBridge.new()
add_child(debug_bridge)

# 设置节点（可选，会自动检测）
debug_bridge.world_node = get_node("3DWorld")
debug_bridge.ui_node = get_node("UILayer")

# 启用自动初始化
debug_bridge.auto_register = true

# 控制Debug GUI（如果编译时启用）
debug_bridge.debug_gui_enabled = true
```

### 调试命令

#### 3D调试绘制
```gdscript
# 绘制测试内容（包含3D和UI）
debug_bridge.draw_test_content()

# 清除所有调试内容
debug_bridge.clear_all_debug()

# 切换渲染器状态
debug_bridge.toggle_renderer(true)
```

#### Debug GUI 控制（启用 PORTAL_DEBUG_GUI_ENABLED 时）
```gdscript
# 初始化 Debug GUI
debug_bridge.initialize_debug_gui()

# 显示/隐藏所有窗口
debug_bridge.show_all_gui_windows()
debug_bridge.hide_all_gui_windows()

# 切换特定窗口
debug_bridge.toggle_gui_window("performance")
debug_bridge.toggle_gui_window("system_info")

# 打印统计信息
debug_bridge.print_gui_stats()

# 创建测试数据
debug_bridge.create_test_gui_data()

# 添加性能采样
debug_bridge.add_performance_sample(16.67)
```

### 属性配置

```gdscript
# 配置属性
debug_bridge.world_node = my_3d_node       # 3D世界节点
debug_bridge.ui_node = my_ui_control       # UI容器节点
debug_bridge.auto_register = true          # 自动注册到渲染管理器
debug_bridge.debug_gui_enabled = true      # 启用Debug GUI（编译时支持）
```

## C++ 集成

### 获取调试系统实例

```cpp
// 在C++代码中获取Debug GUI系统
auto* bridge = get_node<UnifiedDebugRenderBridge>("DebugBridge");
if (bridge && bridge->is_debug_gui_initialized()) {
    auto* gui_system = bridge->get_debug_gui_system();
    // 使用 gui_system 进行操作
}
```

### 直接调用统一渲染API

```cpp
// 直接使用统一调试绘制API
#include "core/render/unified_debug_draw.h"

// 3D绘制
portal_core::debug::UnifiedDebugDraw::draw_line(
    portal_core::Vector3(0, 0, 0), portal_core::Vector3(1, 1, 1), 
    portal_core::render::Color4f::RED
);

// UI绘制
portal_core::debug::UnifiedDebugDraw::draw_ui_text(
    portal_core::Vector2(10, 10), "调试信息", 
    portal_core::render::Color4f::WHITE, 12.0f
);
```

## 编译配置

### 启用 Debug GUI 支持

在 `SConstruct` 或编译环境中确保定义：
```python
env.Append(CPPDEFINES=['PORTAL_DEBUG_ENABLED', 'PORTAL_DEBUG_GUI_ENABLED'])
```

### 条件编译说明

- `PORTAL_DEBUG_ENABLED`：启用基础调试功能
- `PORTAL_DEBUG_GUI_ENABLED`：启用 ImGui 调试界面支持

当未启用 `PORTAL_DEBUG_GUI_ENABLED` 时，Debug GUI 相关方法会变成空实现，不影响编译。

## 窗口管理

### 默认窗口

系统提供以下预设窗口：
- `"system_info"`：系统信息窗口
- `"performance"`：性能监控窗口
- `"render_stats"`：渲染统计窗口
- `"imgui_demo"`：ImGui 演示窗口

### 窗口操作示例

```gdscript
# 显示性能窗口
debug_bridge.toggle_gui_window("performance")

# 隐藏所有窗口
debug_bridge.hide_all_gui_windows()

# 检查初始化状态
if debug_bridge.is_debug_gui_initialized():
    print("Debug GUI 已初始化")
```

## 性能监控

```gdscript
# 添加帧时间数据（毫秒）
debug_bridge.add_performance_sample(get_process_delta_time() * 1000.0)

# 创建测试性能数据
debug_bridge.create_test_gui_data()
```

## 故障排除

### 常见问题

1. **Debug GUI 功能不可用**
   - 检查是否启用了 `PORTAL_DEBUG_GUI_ENABLED` 宏
   - 确认 ImGui 库正确链接

2. **渲染命令不显示**
   - 确保 `auto_register = true`
   - 检查 world_node 和 ui_node 设置
   - 调用 `initialize_renderer()` 手动初始化

3. **窗口不显示**
   - 确保调用了 `initialize_debug_gui()`
   - 检查窗口是否被隐藏或在屏幕外

### 调试步骤

```gdscript
# 检查状态
print("渲染器已初始化: ", debug_bridge.is_initialized())
print("Debug GUI已初始化: ", debug_bridge.is_debug_gui_initialized())
print("Debug GUI已启用: ", debug_bridge.debug_gui_enabled)

# 打印统计信息
debug_bridge.print_gui_stats()
```

## 与旧系统的差异

### 移除的组件
- `DebugGUIBridge`：功能已整合到 `UnifiedDebugRenderBridge`
- `SimpleDebugManager`：简化的管理功能直接内置

### 新增功能
- 统一的生命周期管理
- 自动节点检测
- 条件编译支持
- 性能监控集成

## 最佳实践

1. **场景设置**：在场景根节点添加一个 `UnifiedDebugRenderBridge`
2. **自动初始化**：启用 `auto_register` 进行自动配置
3. **性能监控**：定期调用 `add_performance_sample()` 提供实时性能数据
4. **条件启用**：在发布版本中禁用调试宏以减小体积

## 示例场景配置

```gdscript
# 主场景的 _ready() 函数
func _ready():
    var debug_bridge = UnifiedDebugRenderBridge.new()
    debug_bridge.name = "DebugRenderBridge"
    debug_bridge.auto_register = true
    debug_bridge.debug_gui_enabled = true
    add_child(debug_bridge)
    
    # 绘制一些测试内容
    debug_bridge.draw_test_content()
    
    # 显示性能窗口
    if debug_bridge.is_debug_gui_initialized():
        debug_bridge.toggle_gui_window("performance")
```

通过这种统一的架构，所有调试功能都通过一个简洁的接口进行管理，避免了多桥接器的复杂性和潜在冲突。
