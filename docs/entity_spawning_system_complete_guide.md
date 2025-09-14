# 游戏对象生成系统 - 完整文档

## 概述

本文档描述了一个完整的游戏对象生成系统，实现了从模板文件动态创建游戏实体，并支持深度属性覆写的功能。

## 系统架构

### 三层架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                    数据与表现层 (Godot)                        │
├─────────────────────────────────────────────────────────────┤
│  .tscn模板文件  │  ComponentResource资源  │  EntityManager   │
│  定义"是什么"     │  数据蓝图配置           │  执行"如何做"     │
├─────────────────────────────────────────────────────────────┤
│                      桥接层 (GDExtension)                     │
├─────────────────────────────────────────────────────────────┤
│  GameCoreManager静态方法  │  传递"指令"和"信息"              │
├─────────────────────────────────────────────────────────────┤
│                    逻辑决策层 (C++)                           │
├─────────────────────────────────────────────────────────────┤
│  ECS系统 (AIWaveSystem等)  │  决定"做什么"                   │
└─────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. EntityManager (GDScript单例)

**文件**: `portal_demo_godot/EntityManager.gd`

**职责**:
- 管理所有实体模板(`.tscn`文件)
- 提供实体实例化和属性覆写功能
- 维护活跃实体列表

**主要方法**:
```gdscript
# 基本生成方法
spawn_entity(template_name: String, parent: Node = null, overrides: Dictionary = {}) -> Node

# ECS专用生成方法（支持深度组件覆写）
spawn_entity_with_ecs_override(template_name: String, parent: Node = null, 
                              component_overrides: Dictionary = {}, 
                              general_overrides: Dictionary = {}) -> Node

# 模板管理
get_available_templates() -> Array[String]
has_template(template_name: String) -> bool
reload_templates()

# 实体管理
clear_all_entities()
get_active_entity_count() -> int
```

### 2. GameCoreManager桥接方法 (C++/GDExtension)

**文件**: `portal_demo_godot/gdextension/src/game_core_manager.cpp`

**职责**:
- 提供C++到GDScript的调用桥接
- 让C++系统能够命令Godot生成实体
- 传递信息查询结果

**主要静态方法**:
```cpp
// 基本生成
static Node* spawn_godot_entity(const String& template_name, 
                               Node* parent = nullptr, 
                               const Dictionary& overrides = Dictionary());

// ECS组件覆写生成
static Node* spawn_godot_entity_with_ecs_override(const String& template_name, 
                                                 Node* parent = nullptr,
                                                 const Dictionary& component_overrides = Dictionary(),
                                                 const Dictionary& general_overrides = Dictionary());

// 信息查询
static Array get_available_godot_templates();
static bool has_godot_template(const String& template_name);
```

### 3. 模板文件结构

**位置**: `portal_demo_godot/templates/`

**示例模板** (`ball.tscn`):
```gdscene
[gd_scene load_steps=3 format=3 uid="uid://dpd8bsj6a370t"]

[sub_resource type="SphereMesh" id="SphereMesh_nocpn"]

[sub_resource type="PhysicsBodyComponentResource" id="PhysicsBodyComponentResource_wyiif"]
shape_type = 1
shape_size = Vector3(0.5, 0.5, 0.5)
friction = 0.1
restitution = 0.9
density = 500.0
mass = 0.5

[node name="MeshInstance3D" type="MeshInstance3D"]
mesh = SubResource("SphereMesh_nocpn")

[node name="ECSNode" type="ECSNode" parent="."]
components = Array[Resource]([SubResource("PhysicsBodyComponentResource_wyiif")])
```

## 使用方法

### 从GDScript生成实体

```gdscript
# 基本生成
var entity = EntityManager.spawn_entity("ball", self)

# 带属性覆写的生成
var overrides = {
    "position": Vector3(0, 5, 0),
    "rotation": Vector3(0, PI/4, 0)
}
var entity = EntityManager.spawn_entity("ball", self, overrides)

# ECS组件深度覆写
var component_overrides = {
    "PhysicsBodyComponentResource": {
        "mass": 2.0,
        "friction": 0.5,
        "restitution": 0.7
    }
}
var general_overrides = {
    "position": Vector3(-3, 5, 0)
}
var ecs_entity = EntityManager.spawn_entity_with_ecs_override(
    "ball", self, component_overrides, general_overrides
)
```

### 从C++生成实体

```cpp
#include "game_core_manager.h"

// 基本生成
Dictionary overrides;
overrides["position"] = Vector3(0, 8, 3);
Node* entity = GameCoreManager::spawn_godot_entity("ball", nullptr, overrides);

// ECS组件覆写生成
Dictionary component_overrides;
Dictionary physics_props;
physics_props["mass"] = 0.5f;
physics_props["friction"] = 0.2f;
component_overrides["PhysicsBodyComponentResource"] = physics_props;

Dictionary general_overrides;
general_overrides["position"] = Vector3(0, 8, -3);

Node* ecs_entity = GameCoreManager::spawn_godot_entity_with_ecs_override(
    "ball", nullptr, component_overrides, general_overrides
);
```

## 属性覆写机制

### 支持的覆写类型

1. **普通节点属性**: `position`, `rotation`, `scale` 等
2. **ECS组件资源属性**: 通过组件类名访问其内部属性
3. **嵌套路径访问**: 支持 `"node.property"` 或 `"components[0].property"` 格式

### 覆写处理流程

1. **模板实例化**: 从PackedScene创建实例
2. **属性遍历**: 根据覆写字典中的路径查找目标对象
3. **Resource副本**: 对ComponentResource创建副本避免污染原始模板
4. **属性设置**: 使用反射或直接访问设置属性值
5. **场景树添加**: 将配置完成的实例添加到场景树

## 项目集成

### 1. 设置Autoload

在 `project.godot` 中添加:
```ini
[autoload]
EntityManager="*res://EntityManager.gd"
```

### 2. 编译GDExtension

确保GameCoreManager的桥接方法被正确编译到GDExtension中。

### 3. 创建模板

在 `res://templates/` 目录下放置 `.tscn` 文件。模板应包含:
- 基础Mesh节点
- ECSNode节点（如果需要物理或其他ECS功能）
- 配置好的ComponentResource

## 测试场景

**文件**: `EntitySpawnerTest.tscn`

提供了完整的测试功能:
- 自动测试EntityManager和C++桥接
- 交互式实体生成（按空格键）
- 实体清理（按C键）
- 模板重载（按R键）

## 性能考虑

1. **模板缓存**: EntityManager在启动时预加载所有模板
2. **Resource副本**: 只在需要覆写时创建ComponentResource副本
3. **实例池**: 可扩展实现对象池来复用实例
4. **异步加载**: 可扩展支持大型模板的异步加载

## 扩展建议

1. **分类模板**: 支持子文件夹组织（如 `enemies/`, `items/`）
2. **预设配置**: 创建常用属性组合的预设
3. **批量生成**: 支持一次调用生成多个实体
4. **生命周期管理**: 自动清理超出范围的实体
5. **序列化支持**: 保存/加载生成的实体状态

## 故障排除

### 常见问题

1. **模板未找到**: 检查文件路径和拼写
2. **属性覆写失败**: 确认属性名称和类型正确
3. **C++桥接失败**: 确认EntityManager已正确初始化
4. **ECS组件覆写无效**: 检查ComponentResource类名匹配

### 调试技巧

1. 启用详细日志输出
2. 使用测试场景验证功能
3. 检查Godot编辑器的"远程"选项卡查看运行时节点树
4. 使用GDScript调试器单步执行覆写逻辑

## 结论

这个游戏对象生成系统提供了一个强大、灵活且易于扩展的解决方案，完美地将Godot的可视化设计能力与C++的性能和逻辑控制能力结合起来。通过清晰的架构分层，实现了数据驱动的游戏开发模式，让内容创作和逻辑开发可以并行进行。
