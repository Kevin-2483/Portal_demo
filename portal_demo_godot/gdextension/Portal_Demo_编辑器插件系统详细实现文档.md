# Portal Demo 编辑器插件系统详细实现文档

## 插件系统概览

Portal Demo 项目包含两套协同工作的插件系统：
1. **ECS Editor Plugin** - 核心 ECS 系统管理插件
2. **预设系统插件** - 组件预设管理和自动填充功能

这两套插件通过精心设计的架构，为开发者提供了强大的编辑器内 ECS 开发体验。

## 插件架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                    Godot 编辑器插件层                            │
├─────────────────────────────────────────────────────────────────┤
│  ECS Editor Plugin (主插件)                                     │
│  ├─ plugin.gd (插件入口)                                        │
│  ├─ ecs_dock.gd (可视化管理面板)                                │
│  ├─ ecs_instance_manager.gd (实例生命周期管理)                  │
│  ├─ ecs_event_bus.gd (编辑器事件总线)                           │
│  └─ preset_ui/ (预设系统UI组件)                                 │
├─────────────────────────────────────────────────────────────────┤
│  C++ Inspector Plugin (预设检查器插件)                          │
│  └─ UniversalPresetInspectorPlugin (C++实现)                   │
├─────────────────────────────────────────────────────────────────┤
│             事件流和数据流                                       │
│  编辑器信号 ←→ ECS事件总线 ←→ GameCoreManager                   │
│       ↓              ↓              ↓                          │
│   UI更新        状态同步        ECS生命周期                      │
└─────────────────────────────────────────────────────────────────┘
```

## ECS Editor Plugin 详细实现

### 1. 插件入口 (plugin.gd)

#### 核心职责
- 管理插件的完整生命周期
- 协调所有子系统的初始化和清理
- 桥接 GDScript 和 C++ 插件系统

#### 关键设计模式
```gdscript
# 🎯 组合模式：管理多个子系统
class_name ECSEditorPlugin extends EditorPlugin

var dock_instance           # UI管理
var instance_manager        # 实例管理  
var event_bus              # 事件总线
var preset_inspector_plugin # C++预设插件
```

#### 初始化序列
```
_enter_tree()
    ↓
创建 ECS 事件总线
    ↓
创建实例管理器
    ↓
创建 Dock UI 面板
    ↓
设置预设系统 (_setup_preset_system)
    ↓
连接编辑器信号
    ↓
完成初始化
```

#### 预设系统集成
```gdscript
func _setup_preset_system():
    # 实例化C++预设检查器插件
    preset_inspector_plugin = UniversalPresetInspectorPlugin.new()
    
    # 传递EditorInterface给检查器插件
    if preset_inspector_plugin.has_method("set_editor_interface"):
        preset_inspector_plugin.call("set_editor_interface", get_editor_interface())
    
    # 注册到编辑器
    add_inspector_plugin(preset_inspector_plugin)
```

### 2. ECS 实例管理器 (ecs_instance_manager.gd)

#### 设计目标
- 提供 GameCoreManager 实例的完整生命周期管理
- 支持编辑器持久化模式，避免意外重启
- 实现状态机模式，确保状态转换的可靠性

#### 状态机设计
```gdscript
enum Status {
    STOPPED,     # 无实例运行
    STARTING,    # 实例启动中
    RUNNING,     # 实例正常运行
    STOPPING,    # 实例停止中
    ERROR        # 错误状态
}
```

#### 实例创建流程
```
start_instance()
    ↓
设置状态 → STARTING
    ↓
创建 GameCoreManager 实例
    ↓
设置编辑器持久化模式
    ↓
连接生命周期信号
    ↓
添加到场景树
    ↓
注册到事件总线
    ↓
等待 core_initialized 信号
    ↓
设置状态 → RUNNING
```

#### 编辑器持久化机制
```gdscript
func start_instance() -> bool:
    current_instance = GameCoreManager.new()
    current_instance.name = "EditorGameCoreManager"
    
    # 🔑 关键：启用编辑器持久化模式
    if current_instance.has_method("set_editor_persistent"):
        current_instance.set_editor_persistent(true)
    
    # 添加到场景树，确保不被 GC 回收
    add_child(current_instance)
```

### 3. ECS 事件总线 (ecs_event_bus.gd)

#### 设计理念
- 解耦 UI 组件和 ECS 系统
- 提供编辑器级别的全局通信机制
- 管理多个 ECSNode 的协调工作

#### 核心功能架构
```gdscript
class_name ECSEventBus extends Node

# 全局信号定义
signal game_core_initialized()     # ECS系统就绪
signal game_core_shutdown()        # ECS系统关闭
signal instance_changed(instance)  # 实例切换
signal reset_ecs_nodes()          # 重置所有ECSNode
signal clear_ecs_nodes()          # 清除所有ECSNode实体

# 核心状态管理
var _current_game_core: GameCoreManager = null
var _registered_ecs_nodes: Array[Node] = []
```

#### ECSNode 协调机制
```gdscript
# ECSNode 注册/注销
func register_ecs_node(ecs_node: Node):
    if ecs_node and not ecs_node in _registered_ecs_nodes:
        _registered_ecs_nodes.append(ecs_node)

# 批量操作：重置所有ECSNode状态
func reset_all_ecs_nodes():
    _cleanup_invalid_nodes()
    reset_ecs_nodes.emit()  # 广播重置信号

# 批量操作：清除所有ECSNode实体
func clear_all_ecs_nodes():
    _cleanup_invalid_nodes()
    clear_ecs_nodes.emit()  # 广播清除信号
```

#### 状态广播机制
```gdscript
func set_current_game_core(game_core: GameCoreManager):
    # 🔄 实例切换时的协调逻辑
    if _current_game_core != game_core:
        clear_all_ecs_nodes()  # 清理旧实体
    
    _current_game_core = game_core
    
    # 连接新实例信号
    if _current_game_core:
        _current_game_core.connect("core_initialized", _on_game_core_initialized)
        _current_game_core.connect("core_shutdown", _on_game_core_shutdown)
        
        # 立即广播当前状态
        if _current_game_core.is_core_initialized():
            call_deferred("_on_game_core_initialized")
```

### 4. ECS 管理面板 (ecs_dock.gd)

#### UI 组件设计
- **状态显示区域**：实时显示 ECS 系统状态
- **控制按钮区域**：启动/停止/重启实例
- **配置选项区域**：持久化模式、自动启动设置
- **信息显示区域**：详细的实例信息和 ECSNode 统计
- **ECSNode 控制区域**：批量重置/清除 ECSNode

#### 实时状态更新
```gdscript
func _update_ui_state():
    var is_running = instance_manager.is_running()
    var has_instance = instance_manager.current_instance != null
    
    # 动态更新状态标签颜色
    match instance_manager.get_status():
        instance_manager.Status.RUNNING:
            status_label.add_theme_color_override("font_color", Color.GREEN)
        instance_manager.Status.STARTING:
            status_label.add_theme_color_override("font_color", Color.YELLOW)
        instance_manager.Status.ERROR:
            status_label.add_theme_color_override("font_color", Color.PURPLE)
        _:
            status_label.add_theme_color_override("font_color", Color.RED)
    
    # 动态启用/禁用按钮
    start_button.disabled = has_instance
    stop_button.disabled = not has_instance
    restart_button.disabled = not has_instance
```

#### ECSNode 批量控制功能
```gdscript
func _on_reset_nodes_pressed():
    # 通过事件总线重置所有 ECSNode 状态
    var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
    if event_bus:
        event_bus.call("reset_all_ecs_nodes")

func _on_clear_nodes_pressed():
    # 通过事件总线清除所有 ECSNode 实体
    var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
    if event_bus:
        event_bus.call("clear_all_ecs_nodes")
```

## 预设系统插件详细实现

### 1. C++ 检查器插件集成

#### 架构设计
```cpp
// UniversalPresetInspectorPlugin (C++实现)
class UniversalPresetInspectorPlugin : public EditorInspectorPlugin {
    // 检测 IPresettableResource 类型
    virtual bool can_handle(Object* p_object) override;
    
    // 为支持的资源创建自定义UI
    virtual void parse_begin(Object* p_object) override;
    
    // 处理编辑器接口传递
    void set_editor_interface(EditorInterface* p_interface);
};
```

#### 与 GDScript 的桥接
```gdscript
# 在 plugin.gd 中
func _setup_preset_system():
    preset_inspector_plugin = UniversalPresetInspectorPlugin.new()
    
    # 🔑 关键：传递 EditorInterface 到 C++ 插件
    if preset_inspector_plugin.has_method("set_editor_interface"):
        preset_inspector_plugin.call("set_editor_interface", get_editor_interface())
```

### 2. 通用预设 UI (universal_preset_ui.gd)

#### 多态设计理念
这个 UI 组件可以为任何继承自 `IPresettableResource` 的资源提供预设功能，无需为每种资源类型重复编写代码。

#### 核心功能模块

##### 预设管理功能
```gdscript
# 预设目录结构：res://component_presets/ClassName/preset_name.tres
const PRESET_ROOT_DIR = "res://component_presets/"

func setup_for_resource(resource: Resource, resource_class: String, display_name: String):
    inspected_resource = resource
    resource_class_name = resource_class
    preset_dir = PRESET_ROOT_DIR.path_join(resource_class_name) + "/"
    
    # 连接资源变化信号
    if inspected_resource.changed.is_connected(_on_resource_changed):
        inspected_resource.changed.connect(_on_resource_changed)
```

##### 约束验证系统
```gdscript
# 实时约束检查 - 防抖机制
var constraint_update_timer: Timer

func _on_resource_changed():
    # 使用防抖定时器，避免频繁刷新
    if constraint_update_timer:
        constraint_update_timer.start()

func update_constraint_warnings():
    if inspected_resource.has_method("get_constraint_warnings"):
        var warnings = inspected_resource.call("get_constraint_warnings")
        if warnings.is_empty():
            warning_label.visible = false
        else:
            warning_label.text = "[color=orange]" + warnings + "[/color]"
            warning_label.visible = true
```

##### 自动填充系统
```gdscript
func setup_auto_fill_ui():
    # 检查资源是否支持自动填充
    if not inspected_resource.has_method("get_auto_fill_capabilities"):
        auto_fill_container.visible = false
        return
    
    var capabilities = inspected_resource.call("get_auto_fill_capabilities")
    
    # 为每个能力创建按钮
    for cap_dict in capabilities:
        var capability_name = cap_dict.get("capability_name", "")
        var description = cap_dict.get("description", "")
        var source_node_type = cap_dict.get("source_node_type", "")
        
        var button = Button.new()
        button.text = capability_name
        button.tooltip_text = "%s\nSource: %s\n%s" % [capability_name, source_node_type, description]
        button.pressed.connect(_on_auto_fill_button_pressed.bind(capability_name))
        
        auto_fill_button_container.add_child(button)
```

##### 智能目标节点检测
```gdscript
func _get_target_node() -> Node:
    # 1. 优先使用资源中指定的 target_node_path
    var target_path = ""
    if inspected_resource.has_method("get"):
        target_path = inspected_resource.call("get", "target_node_path")
    
    if target_path and not target_path.is_empty():
        var current_scene = editor_interface.get_edited_scene_root()
        var target_node = current_scene.get_node_or_null(target_path)
        if target_node:
            return target_node
    
    # 2. 回退到编辑器选中的节点
    var editor_selection = editor_interface.get_selection()
    var selected_nodes = editor_selection.get_selected_nodes()
    
    if selected_nodes.size() > 0:
        var selected_node = selected_nodes[0]
        
        # 特殊处理：如果选中的是ECSNode，使用其父节点
        if selected_node.get_class() == "ECSNode":
            return selected_node.get_parent()
        else:
            return selected_node
    
    return null
```

## 插件系统工作流程

### 编辑器启动流程
```
编辑器启动
    ↓
加载 GDExtension
    ↓
注册 C++ 类 (GameCoreManager, ECSComponentResource 等)
    ↓
启用编辑器插件
    ↓
ECS Editor Plugin._enter_tree()
    ↓
创建事件总线 → 创建实例管理器 → 创建 Dock UI
    ↓
设置预设系统（C++ Inspector Plugin）
    ↓
系统就绪，等待用户操作
```

### 实例管理流程
```
用户点击 "Start ECS Instance"
    ↓
ecs_dock.gd._on_start_pressed()
    ↓
ecs_instance_manager.gd.start_instance()
    ↓
创建 GameCoreManager 实例
    ↓
设置编辑器持久化模式
    ↓
注册到事件总线
    ↓
GameCoreManager._ready() → initialize_core()
    ↓
发出 core_initialized 信号
    ↓
更新 UI 状态 → 显示 "RUNNING"
```

### ECSNode 协调流程
```
场景中的 ECSNode._ready()
    ↓
连接到事件总线
    ↓
event_bus.register_ecs_node(self)
    ↓
检查 GameCore 是否就绪
    ↓
如果就绪：立即创建 ECS 实体
    ↓
如果未就绪：等待 game_core_initialized 信号
    ↓
收到信号后：延迟创建 ECS 实体
```

### 预设系统工作流程
```
用户在 Inspector 中编辑组件资源
    ↓
C++ UniversalPresetInspectorPlugin.can_handle() 检测资源类型
    ↓
如果是 IPresettableResource：创建自定义 UI
    ↓
universal_preset_ui.gd.setup_for_resource() 配置预设界面
    ↓
用户操作预设（保存/加载/自动填充）
    ↓
实时约束检查和警告显示
    ↓
资源属性变化 → 发出 changed 信号 → 更新约束警告
```

## 插件系统优势

### 1. 编辑器集成度
- **无缝集成**：完全融入 Godot 编辑器工作流
- **可视化管理**：通过 Dock 面板直观控制 ECS 系统
- **实时反馈**：状态更新、约束检查、错误提示

### 2. 开发效率提升
- **一键操作**：启动/停止/重启 ECS 系统
- **批量管理**：一次性重置/清除所有 ECSNode
- **预设系统**：快速应用常用配置
- **自动填充**：从现有节点自动提取配置

### 3. 稳定性保障
- **编辑器持久化**：避免编译时意外重启
- **状态机管理**：确保状态转换的可靠性
- **事件总线**：解耦组件间依赖，提高稳定性
- **错误恢复**：提供重启和重置机制

### 4. 扩展性设计
- **多态预设系统**：支持任意自定义组件类型
- **插件化架构**：可以独立开发和部署新功能
- **信号驱动**：松耦合的事件通信机制

## 最佳实践建议

### 1. 插件使用
- 优先使用 Dock 面板控制 ECS 系统
- 在编辑器模式下启用持久化模式
- 定期使用"Reset All ECSNodes"清理状态

### 2. 预设管理
- 为常用组件配置创建预设
- 利用自动填充功能提高配置效率
- 关注约束警告，确保配置正确

### 3. 开发调试
- 使用事件总线监听系统状态变化
- 利用 Dock 面板的信息显示区域监控实例状态
- 遇到问题时使用重启功能快速恢复

### 4. 扩展开发
- 新组件资源继承自 IPresettableResource 自动获得预设功能
- 实现约束检查方法提供实时验证
- 实现自动填充接口支持智能配置

## 总结

Portal Demo 的编辑器插件系统通过精心设计的架构，成功地将复杂的 ECS 系统管理、预设系统和自动填充功能无缝集成到 Godot 编辑器中。这套插件系统不仅提供了强大的功能，还保持了出色的用户体验和开发效率。

通过事件总线的解耦设计、状态机的可靠管理、多态预设系统的扩展性，以及编辑器持久化的稳定性保障，这套插件系统为 ECS 游戏开发提供了完整的编辑器解决方案。
