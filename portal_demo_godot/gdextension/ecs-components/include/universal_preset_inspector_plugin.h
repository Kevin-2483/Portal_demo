#pragma once

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/control.hpp>
#include "ipresettable_resource.h"

using namespace godot;

/**
 * 通用预设检查器插件
 * 
 * 这个插件会自动为所有继承自 IPresettableResource 的资源
 * 提供预设保存/加载功能，无需为每个组件类型单独创建插件
 */
class UniversalPresetInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(UniversalPresetInspectorPlugin, EditorInspectorPlugin)

private:
    EditorInterface* editor_interface;

protected:
    static void _bind_methods();

public:
    UniversalPresetInspectorPlugin();
    virtual ~UniversalPresetInspectorPlugin() {}

    // 设置EditorInterface引用
    void set_editor_interface(EditorInterface* p_editor_interface);

    virtual bool _can_handle(Object *p_object) const override;
    virtual void _parse_begin(Object *p_object) override;
};
