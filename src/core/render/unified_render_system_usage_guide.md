# Portal Demo - 统一渲染系统使用指南

## 概述

阶段1的统一渲染系统现已完成！这个系统提供了一个统一的接口来处理3D和2D调试绘制，支持任何自定义渲染步骤，并且完全与Godot集成。

## 系统架构

### 核心组件

1. **统一渲染接口层** (`src/core/render/`)
   - `IUnifiedRenderer` - 统一渲染器接口
   - `UnifiedRenderManager` - 全局渲染命令管理器
   - `UnifiedDebugDraw` - 便利API类

2. **Godot桥接层** (`portal_demo_godot/gdextension/src/render/`)
   - `GodotUnifiedRenderer` - Godot统一渲染器实现
   - `GodotRenderer3D` - 3D世界空间渲染器
   - `GodotRendererUI` - 2D屏幕空间UI渲染器

3. **Godot集成组件** (`portal_demo_godot/gdextension/src/debug/`)
   - `UnifiedDebugRenderBridge` - Godot节点桥接器

## 使用方法

### C++代码中使用

#### 基础3D绘制

```cpp
#include "core/render/unified_debug_draw.h"

// 绘制线段
portal_core::debug::UnifiedDebugDraw::draw_line(
    Vector3(0, 0, 0), Vector3(1, 1, 1), 
    portal_core::render::Color4f::RED
);

// 绘制盒子
portal_core::debug::UnifiedDebugDraw::draw_box(
    Vector3(0, 1, 0), Vector3(0.5f, 0.5f, 0.5f), 
    portal_core::render::Color4f::YELLOW
);

// 绘制球体
portal_core::debug::UnifiedDebugDraw::draw_sphere(
    Vector3(2, 1, 0), 1.0f, 
    portal_core::render::Color4f::CYAN, 16
);

// 绘制坐标轴
portal_core::debug::UnifiedDebugDraw::draw_coordinate_axes(
    Vector3(0, 0, 0), 2.0f
);
```

#### UI绘制

```cpp
// 绘制窗口
portal_core::debug::UnifiedDebugDraw::draw_ui_window(
    Vector2(10, 10), Vector2(200, 150), 
    "调试窗口"
);

// 绘制文本
portal_core::debug::UnifiedDebugDraw::draw_ui_text(
    Vector2(20, 40), "Hello, World!", 
    portal_core::render::Color4f::WHITE, 14.0f
);

// 绘制按钮
portal_core::debug::UnifiedDebugDraw::draw_ui_button(
    Vector2(20, 60), Vector2(80, 25), 
    "按钮"
);

// 绘制进度条
portal_core::debug::UnifiedDebugDraw::draw_ui_progress_bar(
    Vector2(20, 95), Vector2(150, 20), 
    0.75f  // 75%进度
);
```

#### 高级功能

```cpp
// 定时绘制（3秒后消失）
portal_core::debug::UnifiedDebugDraw::draw_line_timed(
    Vector3(0, 2, 0), Vector3(0, 3, 0), 
    3.0f, portal_core::render::Color4f::MAGENTA
);

// 一次性绘制（仅当前帧）
portal_core::debug::UnifiedDebugDraw::draw_line_once(
    Vector3(-1, 0, 0), Vector3(1, 0, 0), 
    portal_core::render::Color4f::RED
);

// 清理命令
portal_core::debug::UnifiedDebugDraw::clear_3d();   // 清理3D命令
portal_core::debug::UnifiedDebugDraw::clear_ui();   // 清理UI命令
portal_core::debug::UnifiedDebugDraw::clear_all();  // 清理所有命令
```

### Godot中使用

#### 1. 添加桥接节点

在Godot场景中添加`UnifiedDebugRenderBridge`节点：

```gdscript
# 在场景中创建节点
var debug_bridge = UnifiedDebugRenderBridge.new()
add_child(debug_bridge)

# 配置节点
debug_bridge.world_node = $"World3D"  # 可选，默认使用自身
debug_bridge.ui_node = $"UILayer"     # 可选，自动查找
debug_bridge.auto_register = true     # 自动初始化
```

#### 2. 绘制测试内容

```gdscript
# 绘制测试内容
debug_bridge.draw_test_content()

# 清理所有调试内容
debug_bridge.clear_all_debug()

# 启用/禁用渲染器
debug_bridge.toggle_renderer(true)
```

#### 3. 检查状态

```gdscript
# 检查是否已初始化
if debug_bridge.is_initialized():
    print("渲染系统已就绪")

# 获取统计信息
var stats = debug_bridge.get_unified_renderer().get_render_stats()
print("总命令数: ", stats.total_commands)
```

## 测试验证

### 编译测试程序

```bash
# 编译项目（包括测试程序）
scons

# 运行统一渲染系统测试
./build/test_unified_render_system.exe
```

### Godot场景测试

1. 打开`portal_demo_godot`项目
2. 创建新场景，添加`UnifiedDebugRenderBridge`节点
3. 运行场景，调用`draw_test_content()`方法
4. 观察3D世界空间和2D UI空间的调试内容

## 性能特点

### 优化特性

- **命令队列缓存** - 避免重复数据拷贝
- **分层渲染** - 支持不同优先级的渲染层
- **批量处理** - 一次性处理多个渲染命令
- **定时清理** - 自动清理过期的调试内容

### 统计信息

```cpp
// 获取渲染统计
auto stats = portal_core::debug::UnifiedDebugDraw::get_stats();
std::cout << "总命令数: " << stats.total_commands << std::endl;
std::cout << "3D命令数: " << stats.commands_3d << std::endl;
std::cout << "UI命令数: " << stats.commands_ui << std::endl;
std::cout << "帧时间: " << stats.frame_time_ms << "ms" << std::endl;
```

## 扩展功能

### 自定义渲染命令

```cpp
// 定义自定义数据结构
struct CustomDebugData {
    Vector3 position;
    float intensity;
    int type;
};

// 提交自定义命令
CustomDebugData data = {Vector3(1, 2, 3), 0.8f, 42};
portal_core::debug::UnifiedDebugDraw::submit_custom_command(
    data, 0x9000, // 自定义类型ID
    static_cast<uint32_t>(portal_core::render::RenderLayer::WORLD_DEBUG)
);
```

### 实现自定义渲染器

```cpp
class MyCustomRenderer : public portal_core::render::IUnifiedRenderer {
public:
    void submit_command(const portal_core::render::UnifiedRenderCommand& command) override {
        // 处理自定义命令
    }
    
    bool supports_command_type(portal_core::render::RenderCommandType type) const override {
        return static_cast<uint32_t>(type) >= 0x9000; // 支持自定义命令
    }
    
    // ... 实现其他接口方法
};

// 注册自定义渲染器
auto& manager = portal_core::render::UnifiedRenderManager::instance();
manager.register_renderer(new MyCustomRenderer());
```

## 调试和故障排除

### 常见问题

1. **渲染内容不显示**
   - 检查`UnifiedDebugRenderBridge`是否正确初始化
   - 确认`world_node`和`ui_node`设置正确
   - 验证渲染器是否启用：`debug_bridge.toggle_renderer(true)`

2. **性能问题**
   - 定期清理不需要的调试内容：`clear_all_debug()`
   - 使用定时绘制避免累积过多永久内容
   - 检查命令数量：`get_stats().total_commands`

3. **编译错误**
   - 确保包含正确的头文件路径
   - 检查是否添加了新的源文件到构建系统
   - 验证Godot-cpp版本兼容性

### 调试输出

```cpp
// 打印详细统计信息
portal_core::debug::UnifiedDebugDraw::print_stats();

// 打印渲染器信息
auto& manager = portal_core::render::UnifiedRenderManager::instance();
manager.print_renderers();
```

## 下一步

阶段1已完成，接下来可以进行：

1. **阶段2** - 实现基于ImGui的DebugGUISystem
2. **阶段3** - 实现IDebuggable接口系统
3. **阶段4** - 集成现有系统（物理、ECS、事件系统）

当前的统一渲染接口为后续阶段提供了坚实的基础！

## 示例场景

参考以下文件获取完整的使用示例：

- `src/core/tests/test_unified_render_system.cpp` - C++测试示例
- `portal_demo_godot/gdextension/src/debug/unified_debug_render_bridge.cpp` - Godot集成示例

---

*文档最后更新: 2025年9月9日*
