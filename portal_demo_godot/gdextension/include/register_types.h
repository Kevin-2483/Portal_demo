#ifndef GDEXTENSION_REGISTER_TYPES_H
#define GDEXTENSION_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

// 函数声明：用于初始化和卸载我们的模块
void initialize_gdextension_module(godot::ModuleInitializationLevel p_level);
void uninitialize_gdextension_module(godot::ModuleInitializationLevel p_level);

#endif // GDEXTENSION_REGISTER_TYPES_H