#include "universal_preset_inspector_plugin.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void UniversalPresetInspectorPlugin::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_editor_interface", "editor_interface"), &UniversalPresetInspectorPlugin::set_editor_interface);
}

UniversalPresetInspectorPlugin::UniversalPresetInspectorPlugin()
{
    editor_interface = nullptr;
}

void UniversalPresetInspectorPlugin::set_editor_interface(EditorInterface* p_editor_interface)
{
    editor_interface = p_editor_interface;
    UtilityFunctions::print("UniversalPresetInspectorPlugin: EditorInterface set successfully");
}

bool UniversalPresetInspectorPlugin::_can_handle(Object *p_object) const
{
    // 检查对象是否继承自 IPresettableResource
    // 这是我们的"魔法"：只需要一行代码就能让任何资源获得预设功能
    IPresettableResource* presettable = Object::cast_to<IPresettableResource>(p_object);
    return presettable != nullptr;
}

void UniversalPresetInspectorPlugin::_parse_begin(Object *p_object)
{
    // 确保这是一个可预设的资源
    IPresettableResource* presettable_resource = Object::cast_to<IPresettableResource>(p_object);
    if (!presettable_resource) {
        return;
    }

    // 加载预设UI场景
    Ref<Resource> scene_res = ResourceLoader::get_singleton()->load("res://addons/ecs_editor_plugin/preset_ui/universal_preset_ui.tscn");
    Ref<PackedScene> ui_scene = scene_res;
    
    if (ui_scene.is_null()) {
        UtilityFunctions::print("Warning: Could not load universal preset UI scene. Make sure res://addons/ecs_editor_plugin/preset_ui/universal_preset_ui.tscn exists.");
        return;
    }

    // 实例化UI
    Control* ui_instance = Object::cast_to<Control>(ui_scene->instantiate());
    if (!ui_instance) {
        UtilityFunctions::print("Error: Failed to instantiate preset UI scene.");
        return;
    }

    // 向UI传递资源信息
    // UI脚本需要知道：
    // 1. 资源实例本身
    // 2. 资源的类名（用于确定预设目录）
    // 3. 资源的显示名称（用于UI显示）
    String class_name = presettable_resource->get_preset_directory_name();
    String display_name = presettable_resource->get_preset_display_name();

    ui_instance->call("setup_for_resource", p_object, class_name, display_name);

    // 传递EditorInterface给UI（如果可用）
    if (editor_interface && ui_instance->has_method("set_editor_interface")) {
        ui_instance->call("set_editor_interface", editor_interface);
        UtilityFunctions::print("UniversalPresetInspectorPlugin: EditorInterface passed to UI");
    }

    // 将UI添加到Inspector的顶部
    add_custom_control(ui_instance);
    
    // 延迟刷新预设列表，确保UI完全初始化
    ui_instance->call_deferred("refresh_presets");
}
