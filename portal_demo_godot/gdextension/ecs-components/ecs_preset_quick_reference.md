# ECS组件预设系统 - 快速参考

## 🚀 新组件添加预设功能 - 3步完成

### 步骤1: 继承接口
```cpp
// your_component.h
#include "ipresettable_resource.h"

class YourComponent : public Resource, public IPresettableResource {
    GDCLASS(YourComponent, Resource)
public:
    virtual String get_preset_display_name() const override { return "Your Component"; }
    virtual String get_constraint_warnings() const override;
    
    // 自动填充支持（可选）
    virtual bool can_auto_fill_from_node(Node* node, const String& capability = "") const override;
    virtual Dictionary auto_fill_from_node(Node* node, const String& capability = "") override;
    
    void set_property(float value) { property = value; emit_changed(); }
    float get_property() const { return property; }
    
private:
    float property = 1.0f;
protected:
    static void _bind_methods();
};
```

### 步骤2: 实现约束检查和自动填充
```cpp
// your_component.cpp
String YourComponent::get_constraint_warnings() const {
    PackedStringArray warnings;
    if (property <= 0.0f) warnings.append("Property must be positive");
    return String("\n").join(warnings);
}

// 自动填充支持（可选）
bool YourComponent::can_auto_fill_from_node(Node* node, const String& capability) const {
    return node && node->is_class("MeshInstance3D");
}

Dictionary YourComponent::auto_fill_from_node(Node* node, const String& capability) {
    Dictionary result;
    result["success"] = false;
    
    if (can_auto_fill_from_node(node, capability)) {
        // 从节点提取数据的逻辑
        float extracted_value = extract_from_node(node);
        set_property(extracted_value);
        
        Dictionary values;
        values["property"] = extracted_value;
        
        result["success"] = true;
        result["property_values"] = values;
        result["applied_capability"] = "Auto Fill";
    }
    
    return result;
}

void YourComponent::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_constraint_warnings"), &YourComponent::get_constraint_warnings);
    ClassDB::bind_method(D_METHOD("get_preset_display_name"), &YourComponent::get_preset_display_name);
    
    // 绑定自动填充方法（如果实现了的话）
    ClassDB::bind_method(D_METHOD("can_auto_fill_from_node", "node", "capability"), &YourComponent::can_auto_fill_from_node);
    ClassDB::bind_method(D_METHOD("auto_fill_from_node", "node", "capability"), &YourComponent::auto_fill_from_node);
    
    ClassDB::bind_method(D_METHOD("set_property", "value"), &YourComponent::set_property);
    ClassDB::bind_method(D_METHOD("get_property"), &YourComponent::get_property);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "property"), "set_property", "get_property");
}

REGISTER_COMPONENT_RESOURCE(YourComponent)
```

### 步骤3: 创建预设目录
```
portal_demo_godot/component_presets/YourComponent/
└── default.tres
```

## ✅ 自动获得的功能
- 预设保存/加载/删除/重置
- 实时约束检查和警告
- **🔄 自动填充功能** (从目标节点获取属性)
- Inspector UI集成
- ECS系统自动同步

## � 自动填充功能使用

### 用户操作流程
1. 选择包含组件的ECSNode
2. 在Inspector中找到组件资源
3. 点击"🔄 Auto Fill"按钮
4. 系统自动从父节点（如MeshInstance3D）提取属性值

### 支持的自动填充类型
- **MeshInstance3D** → 网格形状和尺寸
- **CollisionShape3D** → 碰撞形状数据
- **RigidBody3D/StaticBody3D** → 物理属性
- **其他节点** → 根据组件需求定制

### PhysicsBodyComponentResource 示例
```cpp
// 智能网格识别
SphereMesh → 球体物理形状 (shape_type=1, radius)
BoxMesh → 盒子物理形状 (shape_type=0, size)
CapsuleMesh → 胶囊物理形状 (shape_type=2, radius+height)
CylinderMesh → 盒子物理形状 (shape_type=0, bounding_box)
```

## �📋 必须实现的方法
### 基础功能
- `get_preset_display_name()` - 返回显示名称
- `get_constraint_warnings()` - 返回约束检查结果
- 在 `_bind_methods()` 中绑定这两个方法
- 在属性setter中调用 `emit_changed()`

### 自动填充功能（可选）
- `can_auto_fill_from_node(node, capability)` - 检查是否支持该节点
- `auto_fill_from_node(node, capability)` - 执行自动填充
- 返回Dictionary包含: `success`, `error_message`, `property_values`, `applied_capability`

## 🔧 参考实现
查看 `PhysicsBodyComponentResource` 的完整实现作为参考：
- 支持4种节点类型的自动填充
- 智能网格类型识别
- 完整的错误处理

---
💡 **核心理念**: 一次编写，到处运行 - 任何组件都能自动获得完整的编辑器功能！
