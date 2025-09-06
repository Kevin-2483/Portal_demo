# ECS组件预设和约束检查系统使用指南

## 🎯 概述

本系统为ECS组件提供了通用的预设管理和实时约束检查功能。任何新的ECS组件只需要简单的几步配置，就能自动获得完整的编辑器集成功能，包括：

- ✅ 预设保存/加载/删除/重置
- ✅ 实时属性约束检查和警告显示  
- ✅ 通用的Inspector UI界面
- ✅ **自动填充功能** (从目标节点自动获取属性)
- ✅ 自动与ECS系统集成
- ✅ 无需编写任何UI代码

## 🏗️ 系统架构

### 核心组件

1. **IPresettableResource** (C++ 接口)
   - 位置: `portal_demo_godot/gdextension/ecs-components/include/ipresettable_resource.h`
   - 作用: 标记接口，声明资源支持预设功能和自动填充

2. **UniversalPresetInspectorPlugin** (C++ 插件)
   - 位置: `portal_demo_godot/gdextension/ecs-components/src/universal_preset_inspector_plugin.cpp`
   - 作用: 自动为所有标记资源添加预设UI和自动填充功能

3. **UniversalPresetUI** (GDScript UI)
   - 位置: `portal_demo_godot/addons/ecs_editor_plugin/preset_ui/`
   - 作用: 通用预设界面，处理所有用户交互，包括自动填充

4. **ECS Editor Plugin** (GDScript 插件)
   - 位置: `portal_demo_godot/addons/ecs_editor_plugin/plugin.gd`
   - 作用: 插件主入口，统一管理所有ECS编辑器功能

### 预设存储结构
```
res://component_presets/
├── PhysicsBodyComponentResource/
│   ├── default_box.tres
│   ├── heavy_ball.tres
│   └── light_static.tres
├── YourComponentResource/
│   ├── preset1.tres
│   └── preset2.tres
└── ...
```

## 🚀 为新组件添加预设和约束功能

### 第一步：继承接口

#### 1. 修改头文件
```cpp
// your_component_resource.h
#include "ipresettable_resource.h"

class YourComponentResource : public Resource, public IPresettableResource {
    GDCLASS(YourComponentResource, Resource)
    
public:
    // 必须实现的接口方法
    virtual String get_preset_display_name() const override;
    virtual String get_constraint_warnings() const override;
    
    // 自动填充支持（可选）
    virtual bool can_auto_fill_from_node(Node* node, const String& capability_name = "") const override;
    virtual Dictionary auto_fill_from_node(Node* node, const String& capability_name = "") override;
    
    // 你的组件属性
    void set_your_property(float value);
    float get_your_property() const;
    
private:
    float your_property = 1.0f;
    
    // 属性变化处理（推荐添加）
    void _on_property_changed();
    
protected:
    static void _bind_methods();
};
```

#### 2. 实现CPP文件
```cpp
// your_component_resource.cpp
#include "your_component_resource.h"

// 实现接口方法
String YourComponentResource::get_preset_display_name() const {
    return "Your Component";  // 在UI中显示的名称
}

String YourComponentResource::get_constraint_warnings() const {
    PackedStringArray warnings;
    
    // 添加你的约束检查逻辑
    if (your_property <= 0.0f) {
        warnings.append("Property must be positive");
    }
    if (your_property > 100.0f) {
        warnings.append("Property should not exceed 100");
    }
    
    return String("\n").join(warnings);
}

// 自动填充支持实现
bool YourComponentResource::can_auto_fill_from_node(Node* node, const String& capability_name) const {
    if (!node) return false;
    
    // 检查节点类型是否支持自动填充
    String node_class = node->get_class();
    if (node_class == "MeshInstance3D" || node->is_class("MeshInstance3D")) {
        return true;  // 支持从MeshInstance3D自动填充
    }
    
    return false;
}

Dictionary YourComponentResource::auto_fill_from_node(Node* node, const String& capability_name) {
    Dictionary result;
    result["success"] = false;
    result["error_message"] = "";
    result["property_values"] = Dictionary();
    result["applied_capability"] = "";
    
    if (!can_auto_fill_from_node(node, capability_name)) {
        result["error_message"] = "Node type not supported for auto-fill";
        return result;
    }
    
    // 实现你的自动填充逻辑
    // 例如：从MeshInstance3D获取网格尺寸
    if (node->has_method("get_mesh")) {
        Variant mesh_var = node->call("get_mesh");
        if (mesh_var.get_type() == Variant::OBJECT) {
            Object* mesh_obj = mesh_var;
            if (mesh_obj && mesh_obj->has_method("get_aabb")) {
                Variant aabb_var = mesh_obj->call("get_aabb");
                if (aabb_var.get_type() == Variant::AABB) {
                    AABB aabb = aabb_var;
                    float size = aabb.size.x;  // 使用X轴尺寸
                    
                    // 应用到属性
                    set_your_property(size);
                    
                    Dictionary values;
                    values["your_property"] = size;
                    
                    result["success"] = true;
                    result["property_values"] = values;
                    result["applied_capability"] = "Mesh Size";
                    return result;
                }
            }
        }
    }
    
    result["error_message"] = "Failed to extract data from node";
    return result;
}

// 属性设置器（添加实时检查）
void YourComponentResource::set_your_property(float value) {
    your_property = value;
    _on_property_changed();  // 触发实时约束检查
}

float YourComponentResource::get_your_property() const {
    return your_property;
}

// 属性变化处理
void YourComponentResource::_on_property_changed() {
    emit_changed();  // 触发ECS更新和UI刷新
}

// 绑定方法（必须）
void YourComponentResource::_bind_methods() {
    // 绑定接口方法
    ClassDB::bind_method(D_METHOD("get_constraint_warnings"), &YourComponentResource::get_constraint_warnings);
    ClassDB::bind_method(D_METHOD("get_preset_display_name"), &YourComponentResource::get_preset_display_name);
    
    // 绑定自动填充方法
    ClassDB::bind_method(D_METHOD("can_auto_fill_from_node", "node", "capability_name"), &YourComponentResource::can_auto_fill_from_node);
    ClassDB::bind_method(D_METHOD("auto_fill_from_node", "node", "capability_name"), &YourComponentResource::auto_fill_from_node);
    
    // 绑定属性
    ClassDB::bind_method(D_METHOD("set_your_property", "value"), &YourComponentResource::set_your_property);
    ClassDB::bind_method(D_METHOD("get_your_property"), &YourComponentResource::get_your_property);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "your_property"), "set_your_property", "get_your_property");
}

// 注册组件（推荐）
REGISTER_COMPONENT_RESOURCE(YourComponentResource)
```

### 第二步：创建预设目录和示例

1. **创建预设目录**
   ```
   在 portal_demo_godot/component_presets/ 下创建目录：
   YourComponentResource/
   ```

2. **创建示例预设文件**
   ```tres
   // default.tres
   [gd_resource type="YourComponentResource" format=3]
   
   [resource]
   your_property = 1.0
   ```

### 第三步：编译和测试

1. **编译项目**
   ```bash
   scons
   ```

2. **在Godot编辑器中测试**
   - 创建或选择一个YourComponentResource资源文件
   - 在Inspector中查看预设UI
   - 测试保存/加载/删除预设功能
   - 修改属性值观察实时约束检查

## 🎯 自动填充功能详解

自动填充功能允许组件从场景中的目标节点自动获取属性值，极大简化了组件配置工作。

### 🚀 工作原理

1. **用户选择ECSNode** → 系统自动找到其父节点（通常是MeshInstance3D、CollisionShape3D等）
2. **检查支持能力** → 调用`can_auto_fill_from_node()`检查是否支持该节点类型
3. **执行自动填充** → 调用`auto_fill_from_node()`从节点提取数据并设置到组件属性
4. **更新UI** → 自动刷新Inspector显示新的属性值

### 🔧 支持的自动填充类型

#### PhysicsBodyComponentResource 示例

```cpp
bool PhysicsBodyComponentResource::can_auto_fill_from_node(Node* node, const String& capability_name) const {
    if (!node) return false;
    
    String node_class = node->get_class();
    
    // 支持多种节点类型
    if (node_class == "MeshInstance3D" || node->is_class("MeshInstance3D")) return true;
    if (node_class == "CollisionShape3D" || node->is_class("CollisionShape3D")) return true;
    if (node_class == "RigidBody3D" || node->is_class("RigidBody3D")) return true;
    if (node_class == "StaticBody3D" || node->is_class("StaticBody3D")) return true;
    
    return false;
}

Dictionary PhysicsBodyComponentResource::auto_fill_from_node(Node* node, const String& capability_name) {
    // 根据节点类型选择不同的填充策略
    String node_class = node->get_class();
    
    if (node_class == "MeshInstance3D") {
        return auto_fill_from_mesh_instance(node);  // 从网格形状提取
    } else if (node_class == "CollisionShape3D") {
        return auto_fill_from_collision_shape(node);  // 从碰撞形状提取
    } else if (node_class == "RigidBody3D") {
        return auto_fill_from_rigid_body(node);  // 从刚体属性提取
    }
    
    // ... 其他类型
}
```

#### 智能网格识别

系统能够智能识别不同类型的网格并设置合适的物理形状：

- **SphereMesh** → 球体物理形状 (shape_type=1)
- **BoxMesh** → 盒子物理形状 (shape_type=0)  
- **CapsuleMesh** → 胶囊物理形状 (shape_type=2)
- **CylinderMesh** → 盒子物理形状 (shape_type=0, 使用包围盒)

```cpp
// 从SphereMesh自动填充
if (mesh_class == "SphereMesh") {
    shape_type = 1; // Sphere
    if (mesh_obj->has_method("get_radius")) {
        float radius = mesh_obj->call("get_radius");
        shape_size = Vector3(radius, radius, radius); // 球体使用半径
    }
}
```

### 📋 返回值结构

`auto_fill_from_node()`必须返回Dictionary，包含以下字段：

```cpp
Dictionary result;
result["success"] = true;                    // 是否成功
result["error_message"] = "";                // 错误信息（失败时）
result["property_values"] = Dictionary();    // 设置的属性值
result["applied_capability"] = "Mesh Shape"; // 应用的能力描述
```

### 🎯 最佳实践

#### 1. 渐进式能力检查
```cpp
bool can_auto_fill_from_node(Node* node, const String& capability_name) const {
    if (!node) return false;
    
    // 优先检查特定能力
    if (!capability_name.is_empty()) {
        if (capability_name == "Mesh Shape" && node->is_class("MeshInstance3D")) return true;
        if (capability_name == "Physics Properties" && node->is_class("RigidBody3D")) return true;
        return false;
    }
    
    // 自动检测最佳匹配
    return node->is_class("MeshInstance3D") || node->is_class("RigidBody3D");
}
```

#### 2. 错误处理和回退
```cpp
Dictionary auto_fill_from_node(Node* node, const String& capability_name) {
    Dictionary result;
    result["success"] = false;
    
    // 验证输入
    if (!node) {
        result["error_message"] = "Target node is null";
        return result;
    }
    
    // 尝试提取数据
    try {
        Vector3 data = extract_data_safely(node);
        apply_data_to_properties(data);
        
        result["success"] = true;
        result["property_values"] = create_values_dict(data);
        
    } catch (const std::exception& e) {
        result["error_message"] = String("Failed to extract data: ") + e.what();
    }
    
    return result;
}
```

#### 3. 多能力支持
```cpp
// 支持多种自动填充能力
Dictionary auto_fill_from_node(Node* node, const String& capability_name) {
    if (capability_name == "Mesh Shape") {
        return auto_fill_from_mesh_instance(node);
    } else if (capability_name == "Physics Properties") {
        return auto_fill_from_physics_body(node);
    } else if (capability_name == "Material Properties") {
        return auto_fill_from_material(node);
    } else {
        // 自动选择最佳能力
        return auto_fill_best_match(node);
    }
}
```

### 💡 用户使用流程

1. **选择ECSNode** - 在场景中选择包含你的组件的ECSNode
2. **打开Inspector** - 在Inspector中找到你的组件资源
3. **点击自动填充** - 点击"🔄 Auto Fill"按钮
4. **确认结果** - 查看自动设置的属性值，必要时手动调整

自动填充功能大大减少了手动配置的工作量，特别是对于复杂的物理组件配置！🚀

## 🔧 高级配置选项

### 复杂约束检查
```cpp
String YourComponentResource::get_constraint_warnings() const {
    PackedStringArray warnings;
    
    // 条件约束
    if (enable_feature && required_value <= 0.0f) {
        warnings.append("Feature requires positive value");
    }
    
    // 范围约束
    if (percentage < 0.0f || percentage > 1.0f) {
        warnings.append("Percentage must be between 0 and 1");
    }
    
    // 逻辑约束
    if (auto_mode && manual_override) {
        warnings.append("Cannot enable both auto mode and manual override");
    }
    
    return String("\n").join(warnings);
}
```

### 选择性实时检查
```cpp
// 只在重要属性变化时触发检查
void YourComponentResource::set_critical_property(float value) {
    critical_property = value;
    _on_property_changed();  // 触发检查
}

void YourComponentResource::set_cosmetic_property(Color color) {
    cosmetic_property = color;
    // 不调用 _on_property_changed()，因为这个属性不需要约束检查
}
```

### 自定义显示名称
```cpp
String YourComponentResource::get_preset_display_name() const {
    // 可以是动态的
    return String("Your Component (") + get_mode_name() + ")";
}
```

## 📋 现有参考实现

可以参考 `PhysicsBodyComponentResource` 的完整实现：
- 头文件: `portal_demo_godot/gdextension/ecs-components/include/physics_body_component_resource.h`
- 源文件: `portal_demo_godot/gdextension/ecs-components/src/physics_body_component_resource.cpp`
- 预设目录: `portal_demo_godot/component_presets/PhysicsBodyComponentResource/`

## 🚨 注意事项

### 必须做的
1. ✅ 继承 `IPresettableResource` 接口
2. ✅ 实现 `get_preset_display_name()` 和 `get_constraint_warnings()` 方法
3. ✅ 在 `_bind_methods()` 中绑定这两个方法
4. ✅ 在重要属性的setter中调用 `emit_changed()`

### 推荐做的
1. 🔧 添加 `_on_property_changed()` 辅助方法
2. 🔧 使用 `REGISTER_COMPONENT_RESOURCE` 宏注册组件
3. 🔧 创建有意义的约束检查逻辑
4. 🔧 提供一些常用的预设文件

### 不要做的
1. ❌ 不要修改 `preset_ui` 目录下的代码
2. ❌ 不要在每个属性setter中都调用 `_on_property_changed()`
3. ❌ 不要在 `get_constraint_warnings()` 中执行耗时操作
4. ❌ 不要忘记绑定接口方法到GDScript

## 🎯 总结

这个系统的设计哲学是 **"一次编写，到处运行"**。每个新组件只需要：

1. **最少3行核心代码** - 继承接口 + 实现两个方法
2. **自动获得完整功能** - 预设管理 + 约束检查 + UI集成
3. **零UI开发工作** - 完全通用的预设界面
4. **无缝ECS集成** - 自动与现有ECS系统协作

遵循这个指南，你的新组件将自动获得与 `PhysicsBodyComponentResource` 相同的强大编辑器功能！🚀
