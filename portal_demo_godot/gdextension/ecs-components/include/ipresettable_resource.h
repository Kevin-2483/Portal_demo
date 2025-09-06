#pragma once

#include "ecs_component_resource.h"

using namespace godot;

/**
 * 自动填充结果结构（简化版本，直接使用 Godot 类型）
 */
struct AutoFillResult {
    bool success = false;
    String error_message;
    Dictionary property_values;
    String applied_capability;
    
    AutoFillResult() {}
    AutoFillResult(bool succeeded, const String& error = "", const Dictionary& values = Dictionary(), const String& capability = "")
        : success(succeeded), error_message(error), property_values(values), applied_capability(capability) {}
};

/**
 * 可预设化资源接口
 * 这是一个标记接口，任何继承自它的组件资源都会自动获得预设UI功能
 * 
 * 使用方法：
 * 1. 让您的组件资源继承自 IPresettableResource 而不是 ECSComponentResource
 * 2. 重新编译项目
 * 3. 在编辑器中选择该资源时，会自动显示预设UI
 * 
 * 预设文件将自动保存在 res://component_presets/{ClassName}/ 目录下
 */
class IPresettableResource : public ECSComponentResource
{
    GDCLASS(IPresettableResource, ECSComponentResource)

protected:
    static void _bind_methods();

public:
    IPresettableResource() {}
    virtual ~IPresettableResource() {}

    // 可选：为子类提供自定义预设目录名的机会
    // 默认使用类名作为目录名
    virtual String get_preset_directory_name() const {
        return get_class();
    }

    // 可选：为子类提供自定义预设显示名称的机会
    // 默认使用类名去掉 "Resource" 后缀
    virtual String get_preset_display_name() const {
        String class_name = get_class();
        if (class_name.ends_with("Resource")) {
            return class_name.substr(0, class_name.length() - 8); // 去掉 "Resource"
        }
        return class_name;
    }

    // 可选：约束检查方法，子类可以重写以提供实时验证
    virtual String get_constraint_warnings() const {
        return ""; // 默认无警告
    }

    // 自动填充功能 - 子类如果要支持自动填充，需要重写这些方法
    virtual Array get_auto_fill_capabilities() const {
        return Array(); // 默认不支持自动填充
    }
    
    virtual Dictionary auto_fill_from_node(Node* target_node, const String& capability_name = "") {
        Dictionary result;
        result["success"] = false;
        result["error_message"] = "Auto-fill not implemented for this component type";
        result["property_values"] = Dictionary();
        result["applied_capability"] = "";
        return result;
    }

    // 辅助方法：检查节点是否支持指定的自动填充能力
    virtual bool can_auto_fill_from_node(Node* target_node, const String& capability_name = "") const {
        if (!target_node) {
            return false;
        }
        
        Array capabilities = get_auto_fill_capabilities();
        String node_class = target_node->get_class();
        
        for (int i = 0; i < capabilities.size(); i++) {
            Dictionary cap_dict = capabilities[i];
            String source_type = cap_dict.get("source_node_type", "");
            String cap_name = cap_dict.get("capability_name", "");
            
            // 检查节点类型匹配
            bool type_matches = (node_class == source_type) || target_node->is_class(source_type);
            
            // 如果指定了能力名称，还要检查名称匹配
            bool name_matches = capability_name.is_empty() || (cap_name == capability_name);
            
            if (type_matches && name_matches) {
                return true;
            }
        }
        
        return false;
    }

    // 辅助方法：获取节点支持的能力列表
    virtual Array get_supported_capabilities_for_node(Node* target_node) const {
        Array result;
        
        if (!target_node) {
            return result;
        }
        
        Array capabilities = get_auto_fill_capabilities();
        String node_class = target_node->get_class();
        
        for (int i = 0; i < capabilities.size(); i++) {
            Dictionary cap_dict = capabilities[i];
            String source_type = cap_dict.get("source_node_type", "");
            String cap_name = cap_dict.get("capability_name", "");
            
            // 检查节点类型匹配
            bool type_matches = (node_class == source_type) || target_node->is_class(source_type);
            
            if (type_matches) {
                result.append(cap_name);
            }
        }
        
        return result;
    }
};
