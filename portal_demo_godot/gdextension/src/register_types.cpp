#include "register_types.h"
#include "game_core_manager.h" // 引入我们自定义节点的头文件
#include "rotating_cube.h" // 添加新的头文件

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// 初始化模块时被调用
void initialize_gdextension_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // 在这里注册我们所有自定义的 C++ 类
    ClassDB::register_class<GameCoreManager>();
    ClassDB::register_class<RotatingCube>(true); // 注册 RotatingCube 类
}

// 卸载模块时被调用
void uninitialize_gdextension_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

// GDExtension 的 C 语言入口点
extern "C" {
// 这个函数名是 Godot 加载库时寻找的入口符号 (entry_symbol)
GDExtensionBool GDE_EXPORT godot_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_gdextension_module);
    init_obj.register_terminator(uninitialize_gdextension_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}