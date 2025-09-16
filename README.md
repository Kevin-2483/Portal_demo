# c++游戏引擎demo

这是一个c++游戏引擎demo,目前使用godot引擎作为编辑器,使用c++开发游戏逻辑.

仍在开发,随着进展的深入与持续的实现验证,更多部分将不再依赖游戏引擎,而是独立实现.

## 详细文档

项目包含完整的技术文档：

- [**核心架构分析**](src/core/ARCHITECTURE_ANALYSIS.md) - 系统架构深度解析
- [**Portal 核心库**](src/core/portal_core/README.md) - 传送门系统详细说明
- [**事件系统文档**](src/core/EVENT_SYSTEM_DOCUMENTATION.md) - 事件管理系统指南
- [**ECS 组件预设系统**](portal_demo_godot/gdextension/ecs-components/ecs_component_preset_system_guide.md) - 组件预设使用指南
- [**统一渲染系统**](src/core/render/unified_render_system_usage_guide.md) - 调试渲染系统使用
- [**编辑器插件系统**](portal_demo_godot/gdextension/Portal_Demo_编辑器插件系统详细实现文档.md) - 编辑器集成详解

## 下一步

1. 错误处理
- **异常处理**: 详细的异常处理机制，确保系统稳定性
- **错误码**: 统一的错误码系统，方便错误定位和处理
- **日志系统**: 详细的运行时日志记录

2. 集成传送门

3. 音频计算
- **音频计算**: 基于物理的音频计算

4. 整合渲染代理组件
移动到其他游戏引擎,或是自定义渲染器

## 核心功能
- **无缝传送门系统**: 基于质心检测的精确传送，支持幽灵实体和角色互换
- **高性能 ECS 架构**: 基于 EnTT 的现代化实体组件系统
- **物理引擎集成**: 深度集成 Jolt Physics，提供高精度物理模拟
- **统一事件管理**: 外观模式统一的事件处理系统，支持多种处理策略
- **可视化调试系统**: 统一的 3D/2D 调试渲染管线

## 编辑器集成
- **ECS 编辑器插件**: 完整的编辑器内 ECS 开发体验
- **组件预设系统**: 智能的组件配置管理和自动填充功能
- **实时约束检查**: 组件属性的实时验证和警告系统
- **可视化管理面板**: 直观的 ECS 实体和系统管理界面

## 项目结构

```
Portal_demo/
├── src/                              # C++ 核心代码
│   ├── core/                         # ECS 核心架构
│   │   ├── components/               # ECS 组件定义
│   │   ├── systems/                  # ECS 系统实现
│   │   ├── portal_core/              # 传送门核心库
│   │   ├── debug/                    # 调试系统
│   │   ├── render/                   # 统一渲染系统
│   │   └── physics_events/           # 物理事件系统
│   └── vendor/                       # 第三方库
│       ├── entt/                     # EnTT ECS 库
│       ├── jolt/                     # Jolt Physics 引擎
│       └── imgui/                    # ImGui UI 库
├── portal_demo_godot/                # Godot 项目
│   ├── gdextension/                  # GDExtension 桥接层
│   │   ├── src/                      # C++ 源码
│   │   ├── include/                  # 头文件
│   │   └── ecs-components/           # ECS 组件资源
│   ├── addons/                       # Godot 插件
│   │   └── ecs_editor_plugin/        # ECS 编辑器插件
│   ├── component_presets/            # 组件预设文件
│   └── templates/                    # 实体模板
├── docs/                             # 详细文档
└── SConstruct                        # 统一构建脚本
```

## 技术栈

### 核心技术
- **游戏引擎**: Godot 4.4
- **编程语言**: C++17/20, GDScript, Python
- **ECS 框架**: EnTT (Entity Component System)
- **物理引擎**: Jolt Physics
- **构建系统**: SCons
- **UI 库**: ImGui (调试界面)

### 架构模式
- **ECS 架构**: 高性能的实体组件系统
- **外观模式**: 统一的事件管理接口
- **策略模式**: 多种事件处理策略
- **观察者模式**: 事件发布-订阅机制
- **工厂模式**: 动态组件创建和管理

## 快速开始

### 环境要求
- **Godot**: 4.4 或更高版本
- **Python**: 3.8+ (用于构建系统)
- **编译器**: 
  - Windows: MSVC 2019+ 或 MinGW-w64
  - Linux: GCC 9+ 或 Clang 10+
  - macOS: Xcode 12+

### 构建步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/your-repo/Portal_demo.git
   cd Portal_demo
   git submodule update --init --recursive
   ```

2. **编译 C++ 核心**
   ```bash
   # 调试版本
   python SConstruct mode=debug
   
   # 发布版本
   python SConstruct mode=release
   
   # 包含测试
   python SConstruct tests=yes
   ```

3. **打开 Godot 项目**
   ```bash
   # 使用 Godot 编辑器打开
   godot portal_demo_godot/project.godot
   ```

4. **启用插件**
   - 在 Godot 编辑器中进入 `项目 -> 项目设置 -> 插件`
   - 启用 "ECS Editor Plugin"

## 核心系统介绍

### 传送门系统 (Portal Core)

Portal Core 是项目的核心特性，提供无缝的传送门效果：

- **事件驱动架构**: 响应外部物理引擎事件，无轮询开销
- **质心检测**: 基于实体质心的精确传送触发
- **幽灵实体管理**: 自动管理跨传送门的分身实体
- **角色互换**: 平滑的身份切换，完全无感体验
- **多段裁切**: 智能的模型分段渲染支持

```cpp
// 传送门事件处理示例
manager.on_entity_intersect_portal_start(entity_id, portal_id);
manager.on_entity_center_crossed_portal(entity_id, portal_id, Portal::PortalFace::A);
manager.on_entity_exit_portal(entity_id, portal_id);
```

### ECS 架构系统

基于 EnTT 的高性能 ECS 实现：

- **系统注册机制**: 声明式的系统依赖管理
- **并行执行**: 自动分析系统依赖，支持并行优化
- **组件安全**: 实时的组件数据验证和修正
- **生命周期管理**: 完整的系统和实体生命周期控制

```cpp
// 系统注册示例
REGISTER_SYSTEM(PhysicsSystem, {"PhysicsCommandSystem"}, {}, 20);
REGISTER_SYSTEM_SIMPLE(RotationSystem, 100);
```

### 渲染系统

支持 3D 世界空间和 2D 屏幕空间的统一调试渲染：

- **统一接口**: 一套 API 处理所有渲染需求
- **多层渲染**: 支持渲染层级和优先级
- **实时调试**: 3D 线段、网格、UI 元素的实时绘制
- **性能优化**: 批量提交和命令队列优化

```cpp
// 调试绘制示例
UnifiedDebugDraw::draw_line(Vector3(0,0,0), Vector3(1,1,1), Color4f::RED);
UnifiedDebugDraw::draw_sphere(Vector3(2,1,0), 1.0f, Color4f::CYAN, 16);
UnifiedDebugDraw::draw_ui_window(Vector2(10,10), Vector2(200,150), "调试窗口");
```

### 事件管理系统

外观模式统一的事件处理系统：

- **多种处理策略**: 立即处理、队列处理、延迟处理
- **实体事件管理**: 基于 ECS 的事件实体生命周期
- **临时标记系统**: 自动清理的临时组件标记
- **并发安全**: 多线程环境下的事件处理支持

```cpp
// 事件处理示例
event_manager.publish_immediate(collision_event, EventMetadata{EventPriority::HIGH});
event_manager.enqueue(sync_event);
event_manager.add_temporary_marker(entity, HitMarkerComponent(direction, force), 3);
```

## 开发指南

### 创建新的 ECS 组件

1. **定义组件资源**
   ```cpp
   class YourComponentResource : public ECSComponentResource, public IPresettableResource {
       GDCLASS(YourComponentResource, ECSComponentResource)
   public:
       virtual bool apply_to_entity(entt::registry&, entt::entity) override;
       virtual String get_constraint_warnings() const override;
       // ... 其他接口实现
   };
   ```

2. **实现约束检查**
   ```cpp
   String YourComponentResource::get_constraint_warnings() const {
       PackedStringArray warnings;
       if (property <= 0.0f) warnings.append("Property must be positive");
       return String("\n").join(warnings);
   }
   ```

3. **注册组件**
   ```cpp
   REGISTER_COMPONENT_RESOURCE(YourComponentResource)
   ```

### 创建新的 ECS 系统

1. **继承系统基类**
   ```cpp
   class YourSystem : public ISystem {
   public:
       void update(entt::registry &registry, float delta_time) override;
       const char *get_name() const override { return "YourSystem"; }
       std::vector<std::string> get_dependencies() const override;
   };
   ```

2. **注册系统**
   ```cpp
   REGISTER_SYSTEM(YourSystem, {"DependencySystem"}, {}, 50);
   ```

## 详细文档

项目包含完整的技术文档：

- [**核心架构分析**](src/core/ARCHITECTURE_ANALYSIS.md) - 系统架构深度解析
- [**Portal 核心库**](src/core/portal_core/README.md) - 传送门系统详细说明
- [**事件系统文档**](src/core/EVENT_SYSTEM_DOCUMENTATION.md) - 事件管理系统指南
- [**ECS 组件预设系统**](portal_demo_godot/gdextension/ecs-components/ecs_component_preset_system_guide.md) - 组件预设使用指南
- [**统一渲染系统**](src/core/render/unified_render_system_usage_guide.md) - 调试渲染系统使用
- [**编辑器插件系统**](portal_demo_godot/gdextension/Portal_Demo_编辑器插件系统详细实现文档.md) - 编辑器集成详解

## 使用示例

### 在编辑器中创建 ECS 实体

1. 在场景中添加 `ECSNode`
2. 在 `components` 数组中添加所需组件资源
3. 设置 `target_node_path` 指向要控制的 Node3D
4. 系统自动创建 ECS 实体并应用组件

### 使用组件预设

1. 配置组件属性
2. 点击 "保存预设" 保存预设
3. 使用 "加载预设" 快速加载配置
4. 使用 "自动填充" 从现有节点自动提取属性


## 致谢

- [Godot Engine](https://godotengine.org/) - 优秀的开源游戏引擎
- [EnTT](https://github.com/skypjack/entt) - 高性能的 ECS 库
- [Jolt Physics](https://github.com/jrouwe/JoltPhysics) - 现代化的物理引擎
- [ImGui](https://github.com/ocornut/imgui) - 即时模式 GUI 库

---

**Portal Demo** - 展示现代游戏开发技术的最佳实践