#include "register_types.h"
#include "game_core_manager.h"      // 引入遊戲核心管理器
#include "ecs_node.h"               // 通用 ECS 節點
#include "ecs_component_resource.h" // ECS組件資源基類
#include "component_registrar.h"

// 编辑器插件相关
#include "ipresettable_resource.h"
#include "universal_preset_inspector_plugin.h"

// 调试系统
#include "debug/unified_debug_render_bridge.h"
#include "render/godot_renderer_ui.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// 初始化模組時被調用
void initialize_gdextension_module(ModuleInitializationLevel p_level)
{
  if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
  {
    // 注册核心类
    ClassDB::register_class<GameCoreManager>();
    GDREGISTER_ABSTRACT_CLASS(ECSComponentResource);
    GDREGISTER_ABSTRACT_CLASS(IPresettableResource);
    ClassDB::register_class<ECSNode>();

    // 注册调试系统
    ClassDB::register_class<portal_gdext::debug::UnifiedDebugRenderBridge>();
    ClassDB::register_class<portal_gdext::render::GodotRendererUI>();

    // ✅ 在这里，安全地执行所有延迟的注册！
    for (const auto &func : get_registration_functions())
    {
      func();
    }
  }
  else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
  {
    // 注册编辑器插件相关类 - 只注册Inspector插件，主插件由GDScript管理
    ClassDB::register_class<UniversalPresetInspectorPlugin>();
  }
}

// 卸載模組時被調用
void uninitialize_gdextension_module(ModuleInitializationLevel p_level)
{
  // 编辑器和场景模块都需要处理卸载
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_EDITOR)
  {
    return;
  }
}

// GDExtension 的 C 语言入口点
extern "C"
{
  // 这个函数名是 Godot 加载库时寻找的入口符号 (entry_symbol)
  GDExtensionBool GDE_EXPORT godot_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
  {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_gdextension_module);
    init_obj.register_terminator(uninitialize_gdextension_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_EDITOR);

    return init_obj.init();
  }
}